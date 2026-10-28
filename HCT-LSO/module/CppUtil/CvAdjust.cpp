#include "pch.h"
#include "CvAdjust.h"

#include <opencv2/core/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace cpp_util;
using namespace cv;
using namespace std;


namespace
{
	constexpr int ADJ_MIN = -100;
	constexpr int ADJ_MAX = 100;
	constexpr float GAMMA_MIN = 0.1f;
	constexpr float GAMMA_MAX = 5.0f;

	int clampAdj(int value)
	{
		return std::clamp(value, ADJ_MIN, ADJ_MAX);
	}

	// Shared by both depth paths so the 8-bit and 16-bit curves cannot drift apart.
	double toneCurve(double norm, double offset, double gain, double invGamma)
	{
		double value = (norm - 0.5) * gain + 0.5 + offset;
		value = std::clamp(value, 0.0, 1.0);

		if (invGamma != 1.0) {
			value = std::pow(value, invGamma);
		}

		return value;
	}

	bool isToneNeutral(const CvAdjustParam& param)
	{
		return param.brightness == 0
			&& param.contrast == 0
			&& std::fabs(param.gamma - 1.0f) < 1e-6f;
	}

	// -100 flattens the image to mid-scale, +100 triples the slope.
	double contrastGain(int contrast)
	{
		const double normalized = clampAdj(contrast) / 100.0;
		return normalized >= 0.0 ? 1.0 + normalized * 2.0 : 1.0 + normalized;
	}

	// Brightness, contrast and gamma are all pointwise, so they collapse into a single
	// lookup - one pass over the image instead of three.
	void applyTone(Mat& image, const CvAdjustParam& param)
	{
		const double offset = clampAdj(param.brightness) / 100.0;
		const double gain = contrastGain(param.contrast);
		const double invGamma = 1.0 / std::clamp(param.gamma, GAMMA_MIN, GAMMA_MAX);

		if (image.depth() == CV_8U) {
			Mat table(1, 256, CV_8UC1);
			uchar* entries = table.ptr<uchar>();
			for (int i = 0; i < 256; ++i) {
				entries[i] = saturate_cast<uchar>(toneCurve(i / 255.0, offset, gain, invGamma) * 255.0);
			}

			LUT(image, table, image);
			return;
		}

		// cv::LUT takes 8-bit input only, so the 16-bit path walks its own table.
		vector<ushort> table(65536);
		for (int i = 0; i < 65536; ++i) {
			table[i] = saturate_cast<ushort>(toneCurve(i / 65535.0, offset, gain, invGamma) * 65535.0);
		}

		const int cols = image.cols * image.channels();
		parallel_for_(Range(0, image.rows), [&](const Range& rows) {
			for (int y = rows.start; y < rows.end; ++y) {
				ushort* row = image.ptr<ushort>(y);
				for (int x = 0; x < cols; ++x) {
					row[x] = table[row[x]];
				}
			}
		});
	}

	// dst = gray + weight * (src - gray). Cheaper than an HSV round trip and it avoids the
	// 8-bit precision loss that converting back and forth would cost.
	void applySaturation(Mat& image, int saturation, CvChannelOrder order)
	{
		const double normalized = clampAdj(saturation) / 100.0;
		const double weight = normalized >= 0.0 ? 1.0 + normalized * 2.0 : 1.0 + normalized;

		Mat gray;
		cvtColor(image, gray, order == CvChannelOrder::RGB ? COLOR_RGB2GRAY : COLOR_BGR2GRAY);

		Mat grayColor;
		cvtColor(gray, grayColor, COLOR_GRAY2BGR);

		addWeighted(image, weight, grayColor, 1.0 - weight, 0.0, image);
	}

	// Unsharp mask. The same blend serves both signs: a negative amount makes the weights
	// sum to 1, which interpolates toward the blur instead of away from it.
	void applySharpness(Mat& image, int sharpness)
	{
		const double amount = clampAdj(sharpness) / 100.0;

		// Sharpening holds the radius fixed so the slider only changes strength; widening it
		// there would boost coarser structure and broaden halos instead. Smoothing does widen
		// with the amount, because that side replaces the auto-denoise brightness used to do.
		const double sigma = (amount > 0.0) ? 1.2 : 1.0 + std::fabs(amount) * 1.5;

		Mat blurred;
		GaussianBlur(image, blurred, Size(0, 0), sigma);
		addWeighted(image, 1.0 + amount, blurred, -amount, 0.0, image);
	}

	void applyChannelView(Mat& image, int channelView, CvChannelOrder order)
	{
		const bool isRgb = (order == CvChannelOrder::RGB);
		int index = -1;

		switch (static_cast<CvChannelView>(channelView))
		{
		case CvChannelView::RED:	index = isRgb ? 0 : 2; break;
		case CvChannelView::GREEN:	index = 1; break;
		case CvChannelView::BLUE:	index = isRgb ? 2 : 0; break;
		default:					return;
		}

		Mat plane;
		extractChannel(image, plane, index);

		Mat planes[3] = { plane, plane, plane };
		merge(planes, 3, image);
	}
}


bool cpp_util::isImageAdjustNeutral(const CvAdjustParam& param)
{
	return isToneNeutral(param)
		&& param.sharpness == 0
		&& param.saturation == 0
		&& param.channelView == static_cast<int>(CvChannelView::ALL);
}


bool cpp_util::applyImageAdjust(const unsigned char* src, unsigned char* dst,
	int width, int height, int srcStride, int dstStride,
	int channels, int depthBytes, CvChannelOrder order, const CvAdjustParam& param)
{
	if (src == nullptr || dst == nullptr) {
		return false;
	}
	if (width <= 0 || height <= 0) {
		return false;
	}
	if (channels != 1 && channels != 3) {
		return false;
	}
	if (depthBytes != 1 && depthBytes != 2) {
		return false;
	}

	const int minStride = width * channels * depthBytes;
	if (srcStride < minStride || dstStride < minStride) {
		return false;
	}

	const int type = (depthBytes == 1) ? CV_8UC(channels) : CV_16UC(channels);
	const Mat srcMat(height, width, type, const_cast<unsigned char*>(src), static_cast<size_t>(srcStride));
	Mat dstMat(height, width, type, dst, static_cast<size_t>(dstStride));

	// Work on a copy: dst is allowed to alias src, so it must only be written once every
	// stage is done, and applyChannelView reallocates its output.
	Mat work = srcMat.clone();

	if (!isToneNeutral(param)) {
		applyTone(work, param);
	}

	// Saturation is moot once a single channel is displayed as grayscale.
	if (channels == 3 && param.saturation != 0
		&& param.channelView == static_cast<int>(CvChannelView::ALL)) {
		applySaturation(work, param.saturation, order);
	}

	if (param.sharpness != 0) {
		applySharpness(work, param.sharpness);
	}

	if (channels == 3) {
		applyChannelView(work, param.channelView, order);
	}

	work.copyTo(dstMat);
	return true;
}

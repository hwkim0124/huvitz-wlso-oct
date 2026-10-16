#include "pch.h"
#include "GrayResampler.h"

#include <algorithm>
#include <cmath>


using namespace eye_align;


void eye_align::GrayResampler::configure(int inWidth, int inHeight, int outWidth, int outHeight)
{
	inWidth_ = inWidth;
	inHeight_ = inHeight;
	outWidth_ = outWidth;
	outHeight_ = outHeight;
	if (!isConfigured()) {
		x_ = Kernel();
		y_ = Kernel();
		rowBuffer_.clear();
		return;
	}
	x_ = buildKernel(inWidth, outWidth);
	y_ = buildKernel(inHeight, outHeight);
	rowBuffer_.assign(static_cast<size_t>(inHeight) * outWidth, 0.0f);
}

bool eye_align::GrayResampler::isConfigured(void) const
{
	return inWidth_ > 0 && inHeight_ > 0 && outWidth_ > 0 && outHeight_ > 0;
}

bool eye_align::GrayResampler::matches(int inWidth, int inHeight, int outWidth, int outHeight) const
{
	return isConfigured() && inWidth_ == inWidth && inHeight_ == inHeight && outWidth_ == outWidth && outHeight_ == outHeight;
}

bool eye_align::GrayResampler::apply(const std::uint8_t* gray, int stride, float* dst, float mean, float stdDev) const
{
	if (!isConfigured() || gray == nullptr || dst == nullptr || stride < inWidth_ || stdDev == 0.0f) {
		return false;
	}

	// Horizontal pass: every input row -> outWidth samples (still in 0..255 units).
	for (int y = 0; y < inHeight_; ++y) {
		const std::uint8_t* row = gray + static_cast<size_t>(y) * stride;
		float* out = rowBuffer_.data() + static_cast<size_t>(y) * outWidth_;
		for (int x = 0; x < outWidth_; ++x) {
			const std::uint8_t* src = row + x_.starts[x];
			const float* w = x_.weights.data() + x_.offsets[x];
			const int taps = x_.offsets[x + 1] - x_.offsets[x];
			float acc = 0.0f;
			for (int k = 0; k < taps; ++k) {
				acc += static_cast<float>(src[k]) * w[k];
			}
			out[x] = acc;
		}
	}

	// Vertical pass + normalisation.
	const float scale = 1.0f / (255.0f * stdDev);
	const float offset = -mean / stdDev;
	for (int y = 0; y < outHeight_; ++y) {
		const float* w = y_.weights.data() + y_.offsets[y];
		const int taps = y_.offsets[y + 1] - y_.offsets[y];
		const float* src = rowBuffer_.data() + static_cast<size_t>(y_.starts[y]) * outWidth_;
		float* out = dst + static_cast<size_t>(y) * outWidth_;
		for (int x = 0; x < outWidth_; ++x) {
			float acc = 0.0f;
			for (int k = 0; k < taps; ++k) {
				acc += src[static_cast<size_t>(k) * outWidth_ + x] * w[k];
			}
			out[x] = acc * scale + offset;
		}
	}
	return true;
}

GrayResampler::Kernel eye_align::GrayResampler::buildKernel(int inSize, int outSize)
{
	// PIL / torchvision triangle filter: support widens with the scale when downscaling.
	Kernel kernel;
	kernel.starts.resize(outSize);
	kernel.offsets.resize(static_cast<size_t>(outSize) + 1);

	const double scale = static_cast<double>(inSize) / outSize;
	const double filterScale = std::max(scale, 1.0);
	const double support = 1.0 * filterScale;		// bilinear filter support = 1.0

	std::vector<double> taps;
	for (int i = 0; i < outSize; ++i) {
		const double center = (i + 0.5) * scale;
		const int xmin = std::max(0, static_cast<int>(center - support + 0.5));
		const int xmax = std::min(inSize, static_cast<int>(center + support + 0.5));

		taps.assign(static_cast<size_t>(std::max(0, xmax - xmin)), 0.0);
		double sum = 0.0;
		for (int x = xmin; x < xmax; ++x) {
			const double t = std::abs((x - center + 0.5) / filterScale);
			const double v = (t < 1.0) ? (1.0 - t) : 0.0;
			taps[static_cast<size_t>(x - xmin)] = v;
			sum += v;
		}

		kernel.starts[i] = xmin;
		kernel.offsets[i] = static_cast<int>(kernel.weights.size());
		for (const double v : taps) {
			kernel.weights.push_back(static_cast<float>(sum > 0.0 ? v / sum : 0.0));
		}
	}
	kernel.offsets[outSize] = static_cast<int>(kernel.weights.size());
	return kernel;
}

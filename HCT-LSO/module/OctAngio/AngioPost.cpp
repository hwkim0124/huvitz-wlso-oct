#include "pch.h"
#include "AngioPost.h"
#include "AngioLayout.h"
#include "AngioLayers.h"

#include "OctAngio2.h"

using namespace oct_angio;


struct AngioPost::AngioPostImpl
{
	int filterOrients = ANGIO_GABOR_FILTER_ORIENTS; // 10;
	float filterSigma = ANGIO_GABOR_FILTER_SIGMA;
	float filterDivider = ANGIO_GABOR_FILTER_DIVIDER; // 1.0f;// 1.50f;
	float filterWeight = ANGIO_GABOR_FILTER_WEIGHT; // 0.50f; // 5f; // 0.5f; // 0.65f; // 0.35f; // 0.50f;

	AngioPostImpl() {
	};
};


AngioPost::AngioPost()
	: d_ptr(make_unique<AngioPostImpl>())
{
}


AngioPost::~AngioPost()
{
}


oct_angio::AngioPost::AngioPost(AngioPost && rhs) = default;
AngioPost & oct_angio::AngioPost::operator=(AngioPost && rhs) = default;


oct_angio::AngioPost::AngioPost(const AngioPost & rhs)
	: d_ptr(make_unique<AngioPostImpl>(*rhs.d_ptr))
{
}


AngioPost & oct_angio::AngioPost::operator=(const AngioPost & rhs)
{
	*d_ptr = *rhs.d_ptr;
	return *this;
}


bool oct_angio::AngioPost::createProjectionMask(const AngioLayout & layout, const std::vector<float>& profile, std::vector<float>& mask, bool kernel)
{
	int width = layout.getWidth();
	int height = layout.getHeight();

	if (profile.empty() || profile.size() != (width * height)) {
		return false;
	}

	CvImage image;
	image.fromFloat32((const unsigned char*)&profile[0], width, height);

	if (kernel) {
		image.applyGaussianBlur(1.0f);
	}

	mask = image.copyDataInFloats();
	return true;
}


bool oct_angio::AngioPost::performPostProcessing(int width, int height, std::vector<float>& profile, bool outFlows)
{
	if (profile.empty() || profile.size() != (width * height)) {
		return false;
	}
	if (all_of(profile.begin(), profile.end(), [](float i) { return i <= 0; })) {
		return true;
	}

	CvImage image;
	image.fromFloat32((const unsigned char*)&profile[0], width, height);
	image.resize(width, height);

	CvImage res1, res2;

	int orients = getImpl().filterOrients;
	float sigma = getImpl().filterSigma;
	float divider = getImpl().filterDivider;
	float weight = getImpl().filterWeight;

	LogD() << "Angio filter, orients: " << orients << ", sigma: " << sigma << ", divider: " << divider << ", weight: " << weight;
	
	if (weight > 0.0f) {
		image.copyTo(&res1);
		res1.applyGaborFilter(5, 50, 10, orients, divider, sigma);
		image.applyWeighted(&res1, (1.0f - weight), weight);

		image.copyTo(&res2);
		res2.applyGaussianBlur(3.0);
		image.applyWeighted(&res2, 1.5, -0.5);
	}

	profile = image.copyDataInFloats();
	for_each(profile.begin(), profile.end(), [&](float &elem) { elem = max(elem, 0.0f); });
	return true;
}


bool oct_angio::AngioPost::performVesselProcessing(int width, int height, std::vector<float>& profile)
{
	if (profile.empty() || profile.size() != (width * height)) {
		return false;
	}
	if (all_of(profile.begin(), profile.end(), [](float i) { return i <= 0; })) {
		return true;
	}

	CvImage image;
	image.fromFloat32((const unsigned char*)&profile[0], width, height);
	image.resize(width, height);

	CvImage res1, res2;

	int orients = 12;
	float weight = 0.5f; // 0.75f;
	float divider = 1.5f;
	float sigma = 3.0f;

	LogD() << "Angio filter, orients: " << orients << ", sigma: " << sigma << ", divider: " << divider << ", weight: " << weight;

	if (weight > 0.0f) {
		// image.applyBilateralFilter(5, 50, 50);

		image.copyTo(&res1);
		res1.applyGaborFilter(5, 50, 10, orients, divider, sigma);
		image.applyWeighted(&res1, (1.0f - weight), weight);

		image.copyTo(&res2);
		res2.applyGaussianBlur(3.0);
		image.applyWeighted(&res2, 1.5, -0.5);
	}

	float mean, stdev;
	image.getMeanStddev(&mean, &stdev);
	float thresh1 = mean * 2.4f;
	float thresh2 = mean * 1.8f;
	LogD() << "Angio vessels, tresholds: " << thresh1 << ", " << thresh2;

	auto v = image.copyDataInFloats();
	detectVessels(width, height, v, thresh1, thresh2);
	image.fromFloat32((const unsigned char*)&v[0], width, height);

	// image.applyMedianBlur(3);

	// Mat mask = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(-1, -1));
	// cv::morphologyEx(image.getCvMat(), image.getCvMat(), cv::MorphTypes::MORPH_OPEN, mask, cv::Point(-1, -1), 1);

	// mask = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(-1, -1));
	// cv::morphologyEx(image.getCvMat(), image.getCvMat(), cv::MorphTypes::MORPH_OPEN, mask, cv::Point(-1, -1), 1);

	// image.applyBilateralFilter(5, 50, 50);

	profile = image.copyDataInFloats();
	return true;
}


void oct_angio::AngioPost::detectVessels(int width, int height, std::vector<float>& profile, float thresh1, float thresh2)
{
	if (profile.empty()) {
		return;
	}

	const float FLOW = 255.0f;
	const float BACK = 0.0f;
	int n_flows = 0;
	int n_iters = 0;

	auto data = profile;

	// Seeds for hysterisis thresholding. 
	transform(begin(data), end(data), begin(data), [&](float e) {
		return (e >= thresh1 ? FLOW : e);
	});

	float* p = &data[0];

	// Remoe the isolated points, works like median filter with kernel size. 
	const int FILT_SIZE = 5;
	const int half = FILT_SIZE / 2;
	int removed = 0;
	int remains = 0;

	for (int y = half; y < (height - half); y++) {
		for (int x = half; x < (width - half); x++) {
			int n = y * width + x;
			if (p[n] >= FLOW) {
				int size = 0;
				int q = n - (width * half);
				for (int m = 0; m < FILT_SIZE; m++) {
					for (int k = -half; k <= half; k++) {
						if (p[q+k] >= FLOW) {
							size++;
						}
					}
					q += width;
				}
				if (size <= half) {
					p[n] = BACK;
					removed++;
				}
				else {
					remains++;
				}
			}
		}
	}
	LogD() << "Angio vessels, points removed: " << removed << ", remains: " << remains;
	// return;

	// Double thresholding until flows size not further changed.
	while (remains > 0) {
		for (int y = 1; y < (height - 1); y++) {
			for (int x = 1; x < (width - 1); x++) {
				int n = y * width + x;
				if (p[n] >= FLOW) {
					if (p[n - 1] >= thresh2) {
						p[n - 1] = FLOW;
					}
					if (p[n + 1] >= thresh2) {
						p[n + 1] = FLOW;
					}
					n -= width;
					if (p[n - 1] >= thresh2) {
						p[n - 1] = FLOW;
					}
					if (p[n] >= thresh2) {
						p[n] = FLOW;
					}
					if (p[n + 1] >= thresh2) {
						p[n + 1] = FLOW;
					}
					n += (width * 2);
					if (p[n - 1] >= thresh2) {
						p[n - 1] = FLOW;
					}
					if (p[n] >= thresh2) {
						p[n] = FLOW;
					}
					if (p[n + 1] >= thresh2) {
						p[n + 1] = FLOW;
					}
				}
			}
		}

		int size = 0;
		for_each(begin(data), end(data), [&](float& e) {
			size += (e == FLOW ? 1 : 0);
		});
		if (n_flows != 0 && n_flows == size) {
			break;
		}
		n_flows = size;
		n_iters += 1;
	}

	transform(begin(profile), end(profile), begin(data), begin(profile), [](float e1, float e2) {
		return e1 * e2;
	});

	// profile = data;

	LogD() << "Angio vessels, iterations: " << n_iters;
	return;
}


bool oct_angio::AngioPost::applyNoiseReduction(const AngioLayout& layout, std::vector<float>& profile, float rate)
{
	if (profile.empty()) {
		return false;
	}
	if (rate <= 0.0f) {
		return false;
	}

	int h = layout.getHeight();
	int w = layout.getWidth();

	const float BACKGROUND = 0.45f;

	auto avgs = vector<float>(h, 0.0f);
	auto fpos = profile.begin();
	auto size = (int)(w * BACKGROUND);
	size = min(max(size, 1), w);

	LogD() << "Noise reduction, rate: " << rate << ", size: " << size;

	for (int i = 0; i < h; i++) {
		auto spos = fpos + i*w;
		auto epos = fpos + (i + 1)*w;
		auto line = vector<float>(spos, epos);
		sort(line.begin(), line.end());
		avgs[i] = accumulate(line.begin(), line.begin() + size, 0.0f) / size;
	}

	bool isFovea = layout.isMacularScan();
	int cx, cy, size1, size2;
	layout.getFoveaCenterInPixel(cx, cy);
	layout.getFoveaRadiusInPixel(size1, size2);
	
	float thresh = 0.0f;
	for (int i = 0; i < h; i++) {
		if (isFovea && (i >= (cy - size2) && i <= (cy + size2))) {
			// Apply the preceding threshold level around fovea region. 
		}
		else {
			thresh = avgs[i];
		}

		auto spos = fpos + i*w;
		auto epos = fpos + (i + 1)*w;
		for_each(spos, epos, [&](float &elem) { 
			// elem = max(elem - thresh * rate, 0.0f);
			if (elem <= thresh) {
				elem = max(elem - thresh*rate, 0.0f);
			}
		});
	}
	return true;
}


bool oct_angio::AngioPost::applyNoiseReduction2(const AngioLayout & layout, std::vector<float>& profile, float rate)
{
	if (profile.empty()) {
		return false;
	}
	if (rate <= 0.0f) {
		return false;
	}

	auto height = layout.getHeight();
	auto width = layout.getWidth();
	
	const int RESIZE = 4;
	const int PATCH_W = width / RESIZE; // 64;
	const int PATCH_H = height / RESIZE;
	const int FILTER_W = (PATCH_W / 4 + 1);
	const int FILTER_H = (PATCH_H / 4 + 1);
	const float BACKGROUND = 0.25f; // 0.25f;

	CvImage image;
	image.fromFloat32((const unsigned char*)&profile[0], width, height);
	image.resize(PATCH_W, PATCH_H);

	auto img_w = image.getWidth();
	auto img_h = image.getHeight();
	auto data = image.copyDataInFloats();
	auto dout = vector<float>(img_w * img_h);

	if (layout.isVerticalScan()) {
		auto filt_size = FILTER_H;
		auto filt_half = FILTER_H / 2;

		for (int x = 0; x < img_w; x++) {
			for (int y = 0; y < img_h; y++) {
				int idx = y * img_w + x;
				int r1 = max(y - filt_half, 0);
				int r2 = min(y + filt_half, img_h - 1);
				int dcnt = 0;

				auto vect = vector<float>();

				for (int k = r1; k <= r2; k++) {
					// dsum += data[k*img_w + x];
					vect.push_back(data[k*img_w + x]);
					dcnt += 1;
				}

				sort(vect.begin(), vect.end());
				auto bidx = (int)(dcnt *  BACKGROUND);
				dout[idx] = vect[bidx];
				// dout[idx] = dsum / dcnt;
			}
		}
	}
	else {
		auto filt_size = FILTER_W;
		auto filt_half = FILTER_W / 2;

		for (int y = 0; y < img_h; y++) {
			for (int x = 0; x < img_w; x++) {
				int idx = y * img_w + x;
				int c1 = max(x - filt_half, 0);
				int c2 = min(x + filt_half, img_w - 1);
				int dcnt = 0;

				auto vect = vector<float>();

				for (int k = c1; k <= c2; k++) {
					// dsum += data[k*img_w + x];
					vect.push_back(data[y*img_w + k]);
					dcnt += 1;
				}

				sort(vect.begin(), vect.end());
				auto bidx = (int)(dcnt *  BACKGROUND);
				dout[idx] = vect[bidx];
				// dout[idx] = dsum / dcnt;
			}
		}
	}

	image.fromFloat32((const unsigned char*)&dout[0], img_w, img_h);
	image.resize(width, height);
	auto backs = image.copyDataInFloats();

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			auto idx = y * width + x;
			auto d_val = profile[idx];
			auto b_val = backs[idx];
			auto thresh = b_val * rate;

			// LogD() << d_val << ", " << b_val << ", " << rate << ", " << thresh;
			profile[idx] = max(d_val - thresh, 0.0f);
		}
	}
	return true;
}


bool oct_angio::AngioPost::removeFoveaRegionNoise(const AngioLayout & layout, const AngioLayers & layers, std::vector<float>& profile)
{
	bool isFovea = layout.isMacularScan();
	if (!isFovea) {
		return false;
	}

	int cx, cy, size1, size2;
	layout.getFoveaCenterInPixel(cx, cy);
	layout.getFoveaRadiusInPixel(size1, size2);

	auto uppers = layers.getUpperLayers(OcularLayerType::ILM);
	auto lowers = layers.getLowerLayers(OcularLayerType::IPL);
	auto outers = layers.getLowerLayers(OcularLayerType::OPL);

	auto vertical = layout.isVerticalScan();
	auto lines = layout.numberOfLines();
	auto points = layout.numberOfPoints();

	auto width = layout.getWidth();
	auto height = layout.getHeight();

	auto fsx = cx - size1 / 2;
	auto fex = fsx + size1;
	auto fsy = cy - size2 / 2;
	auto fey = fsy + size2;

	for (auto r = 0; r < lines; r++) {
		for (auto c = 0; c < points; c++) {
			auto index = (vertical ? (c * lines + r) : (r * points + c));
		}
	}

	return false;
}


int & oct_angio::AngioPost::garborFilerOrients(void)
{
	return getImpl().filterOrients;
}

float & oct_angio::AngioPost::garborFilterSigma(void)
{
	return getImpl().filterSigma;
}

float & oct_angio::AngioPost::garborFilterDivider(void)
{
	return getImpl().filterDivider;
}

float & oct_angio::AngioPost::garborFilterWeight(void)
{
	return getImpl().filterWeight;
}



AngioPost::AngioPostImpl & oct_angio::AngioPost::getImpl(void) const
{
	return *d_ptr;
}


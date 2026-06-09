#include "pch.h"
#include "AngioDecorr.h"
#include "AngioLayers.h"
#include "AngioLayout.h"
#include "AngioData.h"

#include "OctAngio2.h"

using namespace oct_angio;



struct AngioDecorr::AngioDecorrImpl
{
	vector<CvImage> decorrelations;
	vector<CvImage> differentials;
	vector<CvImage> intensities;
	vector<CvImage> differWeights;

	vector<float> decorrProjection;
	vector<float> decorrProjection2;
	vector<float> differProjection;
	vector<float> intensProjection;

	vector<float> decorrAngiogram;
	vector<float> differAngiogram;
	vector<float> outputAngiogram;
	vector<float> offsetProjection;

	vector<float> decorrProjectionMask;
	vector<float> differProjectionMask;

	vector<vector<float>> bscanDecorrLists;
	vector<vector<float>> bscanIntensLists;
	vector<float> bscanDecorrMeans;
	vector<float> bscanDecorrStdevs;
	vector<float> bscanDecorrMaxVals;
	vector<float> bscanIntensMeans;
	vector<float> bscanIntensStdevs;
	vector<float> bscanIntensMaxVals;

	float decorrMean = 0.0f;
	float decorrStdev = 0.0f;
	float decorrMaxVal = 0.0f;
	float intensMean = 0.0f;
	float intensStdev = 0.0f;
	float intensMaxVal = 0.0f;
	float differMean = 0.0f;
	float differStdev = 0.0f;
	float differMaxVal = 0.0f;

	float lowerThreshold = 0.0f;
	float upperThreshold = 2500.0f;
	float backgroundThreshold = 0.0f;
	float backgroundMean = 0.0f;
	float backgroundStdev = 0.0f;

	vector<float> bgThresholds;
	vector<float> bgMeans;
	vector<float> bgStdDevs;

	vector<float> amplitMeans;
	vector<float> amplitStdevs;
	vector<float> amplitThresholds;

	int projectWidth = 0;
	int projectHeight = 0;

	AngioDecorrImpl() {

	};
};

AngioDecorr::AngioDecorr()
	: d_ptr(make_unique<AngioDecorrImpl>())
{
}


AngioDecorr::~AngioDecorr()
{
	// Destructor should be defined for unique_ptr to delete AngioDecorrImpl as an incomplete type.
}


oct_angio::AngioDecorr::AngioDecorr(AngioDecorr && rhs) = default;
AngioDecorr & oct_angio::AngioDecorr::operator=(AngioDecorr && rhs) = default;



auto oct_angio::AngioDecorr::estimateThresholds(const AngioLayout& layout, const AngioData & data, const AngioLayers & layers) -> bool
{
	auto lines = layout.numberOfLines();
	LayerArrays layers1 = layers.getUpperLayers(DECORR_UPPER_LAYER_TYPE, DECORR_UPPER_LAYER_OFFSET);
	LayerArrays layers2 = layers.getLowerLayers(DECORR_LOWER_LAYER_TYPE, DECORR_LOWER_LAYER_OFFSET);

	getImpl().amplitMeans = vector<float>(lines, 0.0f);
	getImpl().amplitStdevs = vector<float>(lines, 0.0f);
	getImpl().amplitThresholds = vector<float>(lines, 0.0f);
	
	cpp_util::StopWatch::start();

	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, &layout, &data, &layers1, &layers2, this]()
			{
				auto width = data.getAmplitudes()[0][0].getWidth();
				auto height = data.getAmplitudes()[0][0].getHeight();

				for (int line_idx : tasks[k]) {
					auto& amplit = data.getAmplitudes()[line_idx][0];
					auto p_buff = (float*)amplit.getBitsData();
					
					/*
					const auto& uppers = layers1[line_idx];
					const auto& lowers = layers2[line_idx];
					const auto w_step = 4;
					const auto h_step = 4;
					auto vals = vector<int>();
					auto size = ((width*height) / (w_step*h_step));

					vals.reserve(size);
					for (int c = 0; c < width; c += w_step) {
						auto y1 = min(max(uppers[line_idx], 0), height-1);
						auto y2 = min(max(lowers[line_idx], y1), height-1);
						for (int r = y1; r <= y2; r += h_step) {
							auto idx = r * width + c;
							vals.push_back((int)p_buff[idx]);
						}
					}

					float mean, sdev, thresh;
					if (vals.size() < (w_step*h_step)) {
						amplit.getMeanStddev(&mean, &sdev);
						thresh = mean;
					}
					else {
						auto vsum = accumulate(vals.begin(), vals.end(), 0, plus<int>());
						mean = (float)vsum / vals.size();
						auto vars = inner_product(vals.begin(), vals.end(), vals.begin(), 0.0,
							[](auto const& x, auto const& y) { return x + y; },
							[mean](auto const& x, auto const& y) { return (x - mean)*(y - mean); });
						sdev = (float)sqrt(vars / vals.size());
						thresh = mean;
					}
					*/

					float mean, sdev, thresh;
					amplit.getMeanStddev(&mean, &sdev);
					thresh = max(mean, BSCAN_IMAGE_MEAN);

					getImpl().amplitMeans[line_idx] = (float)mean;
					getImpl().amplitStdevs[line_idx] = (float)sdev;
					getImpl().amplitThresholds[line_idx] = thresh;

					// LogD() << "Angio line index: " << line_idx << ", mean: " << mean << ", stdev: " << sdev << ", thresh: " << thresh;
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	auto msec = cpp_util::StopWatch::getElapsedMillis();
	// LogD() << "Angio thresholds estimated, elapsed: " << msec;
	return true;
}


auto oct_angio::AngioDecorr::calculateSignals(const AngioLayout& layout, const AngioData & data,
	const AngioLayers & layers, int overlaps, bool pixelAvg, int avgOffset, bool circular) -> bool
{
	cpp_util::StopWatch::start();
	LogD() << "Angio signals calculating, lines: " << layout.numberOfLines() << ", repeats: " << layout.numberOfRepeats() << ", points: " << layout.numberOfPoints();

	auto lines = layout.numberOfLines();
	getImpl().decorrelations = vector<CvImage>(lines);
	getImpl().differentials = vector<CvImage>(lines);
	getImpl().intensities = vector<CvImage>(lines);

	// Retian region in where signals calculated. 
	LayerArrays layers1 = layers.getUpperLayers(DECORR_UPPER_LAYER_TYPE, DECORR_UPPER_LAYER_OFFSET);
	LayerArrays layers2 = layers.getLowerLayers(DECORR_LOWER_LAYER_TYPE, DECORR_LOWER_LAYER_OFFSET);

	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, &layout, &data, &layers1, &layers2, pixelAvg, avgOffset, circular, this]()
			{
				auto width = data.getAmplitudes()[0][0].getWidth();
				auto height = data.getAmplitudes()[0][0].getHeight();
				auto repeats = layout.numberOfRepeats();

				for (int line_idx : tasks[k]) {
					vector<float> decorrs = vector<float>(width*height, 0.0f);
					vector<float> differs = vector<float>(width*height, 0.0f);
					vector<float> intenss = vector<float>(width*height, 0.0f);

					const auto& amplits = data.getAmplitudes()[line_idx];
					const auto& uppers = layers1[line_idx];
					const auto& lowers = layers2[line_idx];

					auto bg_mean = getImpl().amplitMeans[line_idx];
					auto bg_stdv = getImpl().amplitStdevs[line_idx];
					auto thresh1 = (int)((bg_mean + bg_stdv) * 0.5f);
					auto thresh2 = (int)(bg_mean + bg_stdv * 1.5f);

					// thresh1 = max(max(thresh1, (int)bg_mean), (int)bg_stdv);
					LogD() << "Line index: " << line_idx << ", mean: " << bg_mean << ", stdev: " << bg_stdv << ", thresh: " << thresh1 << ", " << thresh2;

					vector<pair<int, int>> list;
					if (circular) {
						for (auto u = 0; u < amplits.size() - 1; u++) {
							for (auto v = u + 1; v < amplits.size(); v++) {
								list.emplace_back(u, v);
							}
						}
					}
					else {
						for (auto u = 1; u < amplits.size(); u++) {
							list.emplace_back(u - 1, u);
						}
					}
					auto lsize = list.size();

					for (auto pair : list) {
						auto img1 = pair.first;
						auto img2 = pair.second;
						auto p1 = (float*)amplits[img1].getBitsData();
						auto p2 = (float*)amplits[img2].getBitsData();

						for (auto c = 0; c < width; c++) {
							auto r1 = uppers[c];
							auto r2 = lowers[c];

							for (auto r = r1; r <= r2; r++) {
								auto idx1 = r * width + c;
								auto y1 = r + (pixelAvg ? avgOffset*-1 : 0);
								auto y2 = r + (pixelAvg ? avgOffset*+1 : 0);

								y1 = max(y1, 0);
								y2 = min(y2, height - 1);

								auto diff_sum = 0.0f;
								auto deco_sum = 0.0f;
								auto ints_sum = 0.0f;
								auto count = 0;

								for (int y = y1; y <= y2; y++) {
									auto idx2 = y * width + c;
									auto a1 = p1[idx2];
									auto a2 = p2[idx2];
									auto diff = 0.0f;
									auto deco = 0.0f;
									auto ints = (a1 + a2) / 2;

									if ((a1 > thresh1 && a2 > thresh1)) { //  && (a1 > thresh2 || a2 > thresh2)) {
										deco = (std::pow((a1 - a2), 2.0f) / (std::pow(a1, 2.0f) + std::pow(a2, 2.0f)));
										diff = fabs(a1 - a2);
									}

									diff_sum += diff;
									deco_sum += deco;
									ints_sum += ints;
									count += 1;
								}

								auto diff_res = count ? (diff_sum / count) : 0.0f;
								auto deco_res = count ? (deco_sum / count) : 0.0f;
								auto ints_res = count ? (ints_sum / count) : 0.0f;

								differs[idx1] += diff_res;
								decorrs[idx1] += deco_res;
								intenss[idx1] += ints_res;
							}
						}
					}

					std::for_each(decorrs.begin(), decorrs.end(), [&](float &elem) { elem = (elem / lsize); });
					std::for_each(differs.begin(), differs.end(), [&](float &elem) { elem = (elem / lsize); });
					std::for_each(intenss.begin(), intenss.end(), [&](float &elem) { elem = (elem / lsize); });

					getImpl().decorrelations[line_idx].fromFloat32((const unsigned char*)&decorrs[0], width, height);
					getImpl().differentials[line_idx].fromFloat32((const unsigned char*)&differs[0], width, height);
					getImpl().intensities[line_idx].fromFloat32((const unsigned char*)&intenss[0], width, height);

					/*
					float* decorr = (float*)getImpl().decorrelations[line_idx].getBitsData();
					float* differ = (float*)getImpl().differentials[line_idx].getBitsData();

					for (auto c = 0; c < width; c++) {
						auto peak_pos = starts[c];
						auto peak_val = 0.0f;
						auto peak_idx = 0;
						for (auto r = starts[c]; r <= closes[c]; r++) {
							auto idx2 = r * width + c;

							if (decorr[idx2] > peak_val) {
								peak_pos = r;
								peak_val = decorr[idx2];
								peak_idx = idx2;
							}
						}

						for (auto r = uppers[c]; r <= lowers[c]; r++) {
							auto idx2 = r * width + c;
							auto dc = decorr[idx2];
							auto df = differ[idx2];

							if (r >= nones1[line_idx][c]) { //  && r <= nones2[line_idx][c]) {
								if (peak_idx > 0) {
									dc = max(dc - decorr[peak_idx], 0.0f);
									df = max(df - differ[peak_idx], 0.0f);
									decorr[idx2] = dc;
									differ[idx2] = df;
								}
							}
						}
					}
					*/
				};
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	auto msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angio signal calculated, elapsed: " << msec;
	return true;
}


auto oct_angio::AngioDecorr::updateProjectionMasks(const AngioLayout & layout, 
							const LayerArrays& uppers, const LayerArrays& lowers, bool outerFlow) -> bool
{
	auto lines = layout.numberOfLines();
	auto points = layout.numberOfPoints();
	auto projSize = lines * points;

	auto vertical = layout.isVerticalScan();

	getImpl().differProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection = vector<float>(projSize, 0.0f);

	LogD() << "Updating projection masks, lines: " << lines << ", points: " << points << ", vertical: " << vertical << ", outerFlow: " << outerFlow;

	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, vertical, outerFlow, &layout, &uppers, &lowers, this]()
			{
				auto lines = layout.numberOfLines();
				auto points = layout.numberOfPoints();
				auto height = DECORR_IMAGE_HEIGHT;

				for (auto line_idx : tasks[k]) {
					const auto &cImage = getImpl().decorrelations[line_idx];
					const auto &dImage = getImpl().differentials[line_idx];

					const float* decorr = (const float*)cImage.getBitsData();
					const float* differ = (const float*)dImage.getBitsData();

					if (uppers.size() < line_idx && lowers.size() < line_idx) {
						continue;
					}
					
					const auto& layer1 = uppers[line_idx];
					const auto& layer2 = lowers[line_idx];
					if (layer1.size() != points || layer2.size() != points) {
						continue;
					}
					
					for (auto c = 0; c < points; c++) {
						auto index = (vertical ? (c * lines + line_idx) : (line_idx * points + c));

						auto dcSum = 0.0f, dcAvg = 0.0f;
						auto dfSum = 0.0f, dfAvg = 0.0f;
						auto depth = 0;

						auto y1 = layer1[c];
						auto y2 = layer2[c];
						y1 = min(max(y1, 0), height - 1);
						y2 = min(max(y1, y2), height - 1);

						for (auto r = y1; r <= y2; r++) {
							auto idx2 = r * points + c;
							auto dc = decorr[idx2];
							auto df = differ[idx2];
							auto coeff = 1.0f;

							if (outerFlow) {
								coeff = 1.0f / (1 + exp(-1.0f * (10.0f*dc - 1.5f)));
							}
							else {
								coeff = 0.75f + (log10(dc + 10.0f) - 1.0f) * 40.0f;
							}

							dcSum += dc;
							dfSum += (df * coeff);
							
							// dcSum += dc;
							// dfSum += df;
							depth += 1;
						}
						
						dcAvg = (depth > 0 ? (dcSum / depth) : 0.0f);
						dfAvg = (depth > 0 ? (dfSum / depth) : 0.0f);

						getImpl().decorrProjection[index] = dcAvg;// dcMax; // dcSum;
						getImpl().differProjection[index] = dfAvg; // dfSum); // (outerFlow ? dfSum : dfAvg);
					}
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	getImpl().projectWidth = layout.getWidth();
	getImpl().projectHeight = layout.getHeight();
	getImpl().differAngiogram = getImpl().differProjection;
	getImpl().decorrAngiogram = getImpl().decorrProjection;
	return true;
}

auto oct_angio::AngioDecorr::updateProjectionProfiles(const AngioLayout & layout, const AngioLayers & layers, 
													bool calcStats, bool applyPAR,
													const std::vector<float>& maskDecorr, const std::vector<float>& maskDiffer) -> bool
{
	auto lines = layout.numberOfLines();
	auto points = layout.numberOfPoints();
	auto projSize = lines * points;

	auto vertical = layout.isVerticalScan();
	auto outerFlow = layers.isOuterRetinaFlows();

	// Retian region in where signals calculated. 
	LayerArrays slabs1 = layers.getUpperLayersOfSlab();
	LayerArrays slabs2 = layers.getLowerLayersOfSlab();

	// OPL ~ (RPE + 25)
	LayerArrays nones1 = layers.getUpperLayersOfNonVascular();
	LayerArrays nones2 = layers.getLowerLayersOfNonVascular();

	// OPL ~ (OPR + 9)
	LayerArrays shade1 = layers.getUpperLayersOfShadowed();
	LayerArrays shade2 = layers.getLowerLayersOfShadowed();

	// NFL ~ (IPL + 15)
	LayerArrays vascs1 = layers.getUpperLayersOfVasculature();
	LayerArrays vascs2 = layers.getLowerLayersOfVasculature();

	// ILM ~ IPL ~ IOS
	LayerArrays super1 = layers.getUpperLayersOfSuperficial();
	LayerArrays super2 = layers.getLowerLayersOfSuperficial();
	LayerArrays super3 = layers.getLowerLayers(OcularLayerType::IOS);

	getImpl().differProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection2 = vector<float>(projSize, 0.0f);
	getImpl().intensProjection = vector<float>(projSize, 0.0f);
	getImpl().offsetProjection = vector<float>(projSize, 0.0f);

	if (calcStats) {
		getImpl().bscanDecorrLists = vector<vector<float>>(lines);
		getImpl().bscanDecorrMeans = vector<float>(lines, 0.0f);
		getImpl().bscanDecorrStdevs = vector<float>(lines, 0.0f);
		getImpl().bscanDecorrMaxVals = vector<float>(lines, 0.0f);

		getImpl().bscanIntensLists = vector<vector<float>>(lines);
		getImpl().bscanIntensMeans = vector<float>(lines, 0.0f);
		getImpl().bscanIntensStdevs = vector<float>(lines, 0.0f);
		getImpl().bscanIntensMaxVals = vector<float>(lines, 0.0f);
	}

	bool projMask = applyPAR;
	if (projMask) {
		if (layout.scanRangeX() <= DECORR_PROJECT_REMOVE_RANGE &&
			maskDecorr.size() == projSize && maskDiffer.size() == projSize) {
			projMask = true;
		}
		else {
			projMask = false;
		}
	}

	LogD() << "Updating projection profiles, lines: " << lines << ", points: " << points << ", vertical: " << vertical << ", outFlow: " << outerFlow << ", projMask: " << projMask;

	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, calcStats, vertical, outerFlow, projMask, &layout, &layers, 
											&slabs1, &slabs2, &nones1, &nones2, &shade1, &shade2, &vascs1, &vascs2,
											&super1, &super2, &super3,
											maskDecorr, maskDiffer, this]()
			{
				auto lines = layout.numberOfLines();
				auto points = layout.numberOfPoints();
				auto height = DECORR_IMAGE_HEIGHT;
				auto angio_w = layout.getWidth();
				auto angio_h = layout.getHeight();

				bool isFovea = false;
				int fov_x1, fov_x2, fov_y1, fov_y2;
				int fov_cx, fov_cy, fov_size1, fov_size2;
				int fov_rad_x1, fov_rad_x2, fov_rad_y1, fov_rad_y2;

				if (layout.isFoveaCenter()) {
					layout.getFoveaCenterInPixel(fov_cx, fov_cy);
					layout.getFoveaRadiusInPixel(fov_size1, fov_size2);

					fov_rad_x1 = fov_size1;
					fov_rad_x2 = fov_size1 * 2;
					fov_rad_y1 = fov_size2;
					fov_rad_y2 = fov_size2 * 2;

					fov_x1 = min(max(fov_cx - fov_rad_x2, 0), angio_w - 1);
					fov_x2 = min(max(fov_cx + fov_rad_x2, 0), angio_w - 1);
					fov_y1 = min(max(fov_cy - fov_rad_y2, 0), angio_h - 1);
					fov_y2 = min(max(fov_cy + fov_rad_y2, 0), angio_h - 1);
					isFovea = true;
				}
				
				for (auto line_idx : tasks[k]) {
					const auto& decorrs = getImpl().decorrelations;
					const auto& differs = getImpl().differentials;
					const auto &cImage = decorrs[line_idx];
					const auto &dImage = differs[line_idx];

					const float* decorr = (const float*)cImage.getBitsData();
					const float* differ = (const float*)dImage.getBitsData();
					const float* intens = nullptr;

					if (calcStats) {
						const auto &tImage = getImpl().intensities[line_idx];
						intens = (const float*)tImage.getBitsData();
					}

					const auto& uppers = slabs1[line_idx];
					const auto& lowers = slabs2[line_idx];

					bool useTailMask = false;
					if (vascs1.size() > line_idx && vascs1[line_idx].size() == points &&
						vascs2.size() > line_idx && vascs2[line_idx].size() == points) {
						useTailMask = true;
					}

					bool fromBase = false;
					if (shade1.size() > line_idx && shade1[line_idx].size() == points &&
						shade2.size() > line_idx && shade2[line_idx].size() == points) {
						fromBase = true;
					}

					for (auto c = 0; c < points; c++) {
						auto index = (vertical ? (c * lines + line_idx) : (line_idx * points + c));
						auto x = index % angio_w;
						auto y = index / angio_w;

						auto dcMax = 0.0f, dcSum = 0.0f, dcAvg = 0.0f;
						auto dfMax = 0.0f, dfSum = 0.0f, dfAvg = 0.0f;
						auto itMax = 0.0f;
						auto depth = 0;
						auto offset = 0.0f;

						auto peak_idx = 0;
						auto peak_pos = 0;
						auto peak_val = 0.0f;

						auto fov_rate = 1.0f;
						auto on_fovea = false;

						const int FOVEA_WIDE_DEPTH = 11;
						const int FOVEA_NEAR_DEPTH = 21;

						auto vasc_y1 = 0;
						auto vasc_y2 = 0;

						auto supf_y1 = super1[line_idx][c];
						auto supf_y2 = super2[line_idx][c];
						auto supf_y3 = super3[line_idx][c];

						supf_y1 = min(max(supf_y1, 0), height - 1);
						supf_y2 = min(max(supf_y1, supf_y2), height - 1);
						supf_y3 = min(max(supf_y2, supf_y3), height - 1);

						if (useTailMask) {
							vasc_y1 = vascs1[line_idx][c];
							vasc_y2 = vascs2[line_idx][c];

							vasc_y1 = min(max(vasc_y1, 0), height - 1);
							vasc_y2 = min(max(vasc_y1, vasc_y2), height - 1);

							for (auto r = vasc_y1; r <= vasc_y2; r++) {
								auto idx2 = r * points + c;
								/*
								if (differ[idx2] > peak_val) {
									peak_pos = r;
									peak_val = differ[idx2];
									peak_idx = idx2;
								}
								*/
								if (decorr[idx2] > peak_val) {
									peak_pos = r;
									peak_val = decorr[idx2];
									peak_idx = idx2;
								}
							}

							if (isFovea) {
								if (x >= fov_x1 && x <= fov_x2 && y >= fov_y1 && y <= fov_y2) {
									auto dx = abs(x - fov_cx);
									auto dy = abs(y - fov_cy);
									auto depth = supf_y2 - supf_y1;

									if (depth < FOVEA_WIDE_DEPTH) {
										if (dx <= fov_rad_x2 && dy <= fov_rad_y2) {
											on_fovea = true;
										}
									}
									else if (depth < FOVEA_NEAR_DEPTH) {
										if (dx <= fov_rad_x1 && dy <= fov_rad_y1) {
											on_fovea = true;
										}
									}
								}
							}
						}

						for (auto r = uppers[c]; r <= lowers[c]; r++) {
							auto idx2 = r * points + c;
							auto dc = decorr[idx2];
							auto df = differ[idx2];
							auto coeff = 1.0f;

							if (useTailMask) {
								if (on_fovea) {
									if (r >= supf_y1 && r <= supf_y3) {
										auto offs = r - supf_y1;
										fov_rate = min(0.01f * offs * offs, 1.0f);
										dc *= fov_rate;
										df *= fov_rate;
									}
								}

								if (projMask && fromBase) {
									auto nones_y1 = nones1[line_idx][c];
									auto nones_y2 = nones2[line_idx][c];
									auto shade_y1 = shade1[line_idx][c];
									auto shade_y2 = shade2[line_idx][c];

									if (r >= nones_y1 && r <= nones_y2) {
										dc = max(dc - decorr[peak_idx], 0.0f);
										df = max(df - differ[peak_idx], 0.0f);
									}
									else if (r >= shade_y1 && r <= shade_y2) {
										dc = max(dc - maskDecorr[index], 0.0f);
										df = max(df - maskDiffer[index], 0.0f);
									}
								}
								else {
									// Without PAR.
									/*
									if (r >= nones1[line_idx][c] && r <= nones2[line_idx][c]) {
										if (peak_idx > 0) {
											dc = max(dc - decorr[peak_idx], 0.0f);
											df = max(df - differ[peak_idx], 0.0f);
										}
									}
									*/
								}
							}

							if (outerFlow) {
								coeff = 1.0f / (1 + exp(-1.0f * (10.0f*dc - 1.5f)));
							}
							else {
								coeff = 0.75f + (log10(dc + 10.0f) - 1.0f) * 40.0f;
							}
							df *= coeff;
							
							dcSum += dc;
							dfSum += df;

							if (dcMax <= dc) {
								dfMax = df;
								dcMax = dc;

								if (fromBase) {
									offset = max((float)(shade2[line_idx][c] - r), 0.0f);
								}
								else {
									offset = max((float)(lowers[c] - r), 0.0f);
								}
							}

							if (calcStats) {
								auto it = intens[idx2];
								if (itMax < it) {
									itMax = it;
								}

								getImpl().bscanDecorrLists[line_idx].push_back(dc);
								getImpl().bscanIntensLists[line_idx].push_back(it);
							}
							depth += 1;
						}

						dcAvg = (depth > 0.0f ? (dcSum / depth) : 0.0f);
						dfAvg = (depth > 0.0f ? (dfSum / depth) : 0.0f);

						getImpl().decorrProjection[index] = (outerFlow || true ? (dcSum / sqrt(depth + 1.0f)) : dcAvg);// dcMax; // dcSum;
						getImpl().differProjection[index] = (outerFlow || true ? (dfSum / sqrt(depth + 1.0f)) : dfAvg); // dfMax); // (outerFlow ? dfSum : dfAvg);
						getImpl().decorrProjection2[index] = dcMax;
						getImpl().intensProjection[index] = itMax;
						getImpl().offsetProjection[index] = offset;
					}
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	getImpl().projectWidth = layout.getWidth();
	getImpl().projectHeight = layout.getHeight();
	getImpl().differAngiogram = getImpl().differProjection;
	getImpl().decorrAngiogram = getImpl().decorrProjection;
	return true;
}


bool oct_angio::AngioDecorr::calculateDecorrelations(Amplitudes & amplitudes, Reals& reals, Imaginaries& imags, const AngioLayers& layers,
										int angioOverlaps, float upperThresh, float lowerThresh, bool pixelAverage, int averageOffset, 
										bool differOutput, bool circular, float decorrThresh, bool reflectCorrect)
{
	if (amplitudes.size() <= 0) {
		return false;
	}

	int lines = (int) amplitudes.size();
	int repeats = angioOverlaps;
	int width = amplitudes[0][0].getWidth();
	int height = amplitudes[0][0].getHeight();

	float threshold1 = getImpl().backgroundThreshold;
	// float threshold2 = getImpl().backgroundThreshold * 9.0f;
	float threshold2 = exp(0.1f*24.0f) * getImpl().backgroundThreshold;

	if (lowerThresh > 0.0f) {
		threshold1 = lowerThresh;
	}
	if (upperThresh > 0.0f) {
		threshold2 = upperThresh;
	}

	cpp_util::StopWatch::start();
	LogD() << "Calculating decorr., lines: " << lines << ", repeats: " << repeats << ", width: " << width << ", height: " << height;

	getImpl().decorrelations = vector<CvImage>(lines);
	getImpl().differentials = vector<CvImage>(lines);
	getImpl().intensities = vector<CvImage>(lines);
	// getImpl().differWeights = vector<CvImage>(lines);

	LayerArrays layers1 = layers.getUpperLayers(DECORR_UPPER_LAYER_TYPE, DECORR_UPPER_LAYER_OFFSET);
	LayerArrays layers2 = layers.getLowerLayers(DECORR_LOWER_LAYER_TYPE, DECORR_LOWER_LAYER_OFFSET);
	LayerArrays shades1 = layers.getUpperLayersOfShadowed();
	LayerArrays shades2 = layers.getLowerLayersOfShadowed();

	vector<float> decorrSums = vector<float>(lines, 0.0f);
	vector<float> decorrCnts = vector<float>(lines, 0.0f);

	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, lines, repeats, width, height, threshold1, threshold2, &decorrSums, &decorrCnts,
				pixelAverage, averageOffset, differOutput, circular, layers1, layers2, shades1, shades2, &amplitudes, &reals, &imags, 
				decorrThresh, reflectCorrect, this]()
			{
				for (int i : tasks[k]) {
					vector<float> decorrs = vector<float>(width*height, 0.0f);
					vector<float> differs = vector<float>(width*height, 0.0f);
					vector<float> intenss = vector<float>(width*height, 0.0f);
					// vector<float> weights = vector<float>(width*height, 0.0f);
					
					int index = i; 
					auto upper = layers1[index];
					auto lower = layers2[index];
					auto shade1 = shades1[index];
					auto shade2 = shades2[index];

					// Adaptive thresholding with the background level of this image. 
					float thresh0 = 0.0f; 
					if (getImpl().bgThresholds.size() > 0) {
						thresh0 = getImpl().bgThresholds[i] * 1.0f;
					}

					float thresh1 = (threshold1 > 0.0f ? threshold1 : thresh0) * 1.0f;
					float thresh2 = (threshold2 > 0.0f ? threshold2 : exp(0.1f*24.0f) * thresh0) * 1.0f;

					thresh1 = getImpl().bgMeans[i];
					thresh2 = (getImpl().bgMeans[i] + getImpl().bgStdDevs[i] * 2.5f);

					/*
					// Complex data.
					vector<float> a_means = vector<float>(width*height, 0.0f);
					vector<float> i_means = vector<float>(width*height, 0.0f);
					vector<float> r_means = vector<float>(width*height, 0.0f);
					vector<float> a_means2 = vector<float>(width*height, 0.0f);

					int M = amplitudes[i].size();
					for (int m = 0; m < M; m++) {
						float *pa = (float*)amplitudes[i][m].getBitsData();
						float *pi = (float*)imags[i][m].getBitsData();
						float *pr = (float*)reals[i][m].getBitsData();
						for (int c = 0; c < width; c++) {
							for (int r = 0; r < height; r++) {
								int idx = r * width + c;
								a_means[idx] += pa[idx];
								i_means[idx] += pi[idx];
								r_means[idx] += pr[idx];
							}
						}
					}

					int N = width * height;
					for (int i = 0; i < N; i++) {
						a_means[i] /= M;
						i_means[i] /= M;
						r_means[i] /= M;

						a_means2[i] = (float)sqrt(pow(i_means[i], 2.0f) + pow(r_means[i], 2.0f));
					}
					*/

					for (int m = 1; m < amplitudes[i].size(); m++) {
						if (m >= repeats && circular == false) {
							break;
						}

						float *p1 = (float*)amplitudes[i][m - 1].getBitsData();
						// float *p1 = (float*)amplitudes[i][m].getBitsData();
						float *p2 = (float*)amplitudes[i][m].getBitsData();

						for (int c = 0; c < width; c++) {
							int r1 = upper[c]; // +(pixelAverage ? averageOffset*+1 : 0);
							int r2 = lower[c]; // +(pixelAverage ? averageOffset*-1 : 0);
							// int c1 = max(c - pixelAverage, 0);
							// int c2 = min(c + pixelAverage, width-1);

							for (int r = r1; r <= r2; r++) {
								int idx = r * width + c;
								int y1 = r + (pixelAverage ? averageOffset*-1 : 0);
								int y2 = r + (pixelAverage ? averageOffset*+1 : 0);
								y1 = max(y1, 0);
								y2 = min(y2, height-1);

								float flows = 0.0f;
								float decor = 0.0f;
								float mdiff = 0.0f;
								float count = 0.0f;
								float power = 0.0f;
								float differ = 0.0f;
								float weight = 1.0f;
							
								for (int y = y1; y <= y2; y++) {
									int idx2 = y * width + c; 
									float a1 = p1[idx2];
									float a2 = p2[idx2];
									// float a2 = a_means2[idx2]; 
									float df = fabs(a1 - a2);

									power += ((a1 + a2) / 2);

									// Mostly all pixels within retina would be over the background threshold and tested, which 
									// are with the purpose of leaving out the non-retina region, rather than in order
									// to identify the vasculatures. 
									if (a1 > thresh1 && a2 > thresh1) {
										/*
										if (reflectCorrect) {
											if (y < shade1[c]) {
												a1 = min(a1, thresh2);
												a2 = min(a2, thresh2);
												df = fabs(a1 - a2);
											}
										}
										*/

										float dc = (std::pow((a1 - a2), 2.0f) / (std::pow(a1, 2.0f) + std::pow(a2, 2.0f)));

										/*
										// If this pixel consists of projection affected region,
										// the variance measured from it should be attenuated.
										if (true) { // reflectCorrect) {
											if (y >= shade1[c] && y <= shade2[c]) {
												dc *= 0.1f;
												df = fabs(min(a1, thresh2) - min(a2, thresh2));
											}
										}
										*/

										decor += dc;
										mdiff += df;
										count += 1.0f;
									}
									else {
										decor += 0.0f;
										mdiff += 0.0f; //  df; // 0.0f;
										count += 1.0f;
									}
								}

								decor = (count > 0.0f ? (decor / count) : 0.0f);
								mdiff = (count > 0.0f ? (mdiff / count) : 0.0f);
								power = (count > 0.0f ? (power / count) : 0.0f);

								if (decor < decorrThresh) {
									mdiff = 0.0f;
									decor = 0.0f;
								}

								// Apply the weight of reflection correction to the averaged decorrelation around the pixel. 
								// The difference of intensities is not affected by the attenuation processing. 
								if (reflectCorrect) {
									if (r >= shade1[c] && r <= shade2[c]) {
										weight = DECORR_REFLECT_WEIGHT;
										decor *= weight;
										// mdiff *= 0.5f; // weight;
									}
								}
								differ = mdiff;

								decorrs[idx] += decor;
								differs[idx] += differ;
								intenss[idx] += power;
								// weights[idx] += weight;

								/*
								decorrSums[i] += decor;
								decorrCnts[i] += 1.0f;
								*/
							}
						}
					}

					/*
					if (i == 192) {
						for (int c = 0; c < width; c++) {
							for (int r = upper[c]; r <= lower[c]; r++) {
								int s = r * width + c;
								*/
								/*
								if (intenss2[s].size() > 0) {
									LogD() << xcoords[s] << ", " << ycoords[s] << ", " << intenss2[s][0] << ", " << intenss2[s][1] << ", " << intenss2[s][2] << ", " << decorrs2[s][0] << ", " << decorrs2[s][1] << ", " << differs2[s][0] << ", " << differs2[s][1];
								}
								*/
								/*
								LogD() << r << ", " << c << " : " << weights[s] << ", " << decorrs[s] << ", " << differs[s];
							}
						}
					}
					*/
				
					int size = repeats + (circular ? 0 : -1);
					std::for_each(decorrs.begin(), decorrs.end(), [&](float &elem) { elem = (elem / size); });
					std::for_each(differs.begin(), differs.end(), [&](float &elem) { elem = (elem / size); });
					std::for_each(intenss.begin(), intenss.end(), [&](float &elem) { elem = (elem / size); });
					// std::for_each(weights.begin(), weights.end(), [&](float &elem) { elem = (elem / size); });

					// std::for_each(decorrs.begin(), decorrs.end(), [&](float &elem) { elem = (elem < getImpl().decorThreshold ? 0.0f : elem); });
					getImpl().decorrelations[i].fromFloat32((const unsigned char*)&decorrs[0], width, height);
					getImpl().differentials[i].fromFloat32((const unsigned char*)&differs[0], width, height);
					getImpl().intensities[i].fromFloat32((const unsigned char*)&intenss[0], width, height);
					// getImpl().differWeights[i].fromFloat32((const unsigned char*)&weights[0], width, height);
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	/*
	for (int i = 0; i < lines; i++) {
		float mean = decorrSums[i] / (decorrCnts[i] + 1.0f);
		LogD() << "Angio line: " << i << ", decorr. mean: " << mean << ", count: " << decorrCnts[i];
	}
	*/

	double msec = cpp_util::StopWatch::getElapsedMillis();
	LogD() << "Angio decorr. calculated, elapsed: " << msec;
	return true;
}


bool oct_angio::AngioDecorr::updateProjectionProfiles(const AngioLayout& layout, const LayerArrays& uppers, const LayerArrays& lowers,
								bool calcStats, bool calcMask, bool reflectCorrect,
								const AngioLayers& maskLayers, 
								const std::vector<float>& maskDecorr, 
								const std::vector<float>& maskDiffer)
{
	int lines = layout.numberOfLines();
	int points = layout.numberOfPoints();
	int width = points;
	int height = DECORR_IMAGE_HEIGHT;
	bool vertical = layout.isVerticalScan();

	LayerArrays layers1 = uppers;
	LayerArrays layers2 = lowers;

	int projSize = lines * points;
	getImpl().differProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection2 = vector<float>(projSize, 0.0f);
	getImpl().intensProjection = vector<float>(projSize, 0.0f);
	getImpl().offsetProjection = vector<float>(projSize, 0.0f);

	if (calcStats) {
		getImpl().bscanDecorrLists = vector<vector<float>>(lines);
		getImpl().bscanDecorrMeans = vector<float>(lines, 0.0f);
		getImpl().bscanDecorrStdevs = vector<float>(lines, 0.0f);
		getImpl().bscanDecorrMaxVals = vector<float>(lines, 0.0f);

		getImpl().bscanIntensLists = vector<vector<float>>(lines);
		getImpl().bscanIntensMeans = vector<float>(lines, 0.0f);
		getImpl().bscanIntensStdevs = vector<float>(lines, 0.0f);
		getImpl().bscanIntensMaxVals = vector<float>(lines, 0.0f);
	}

	bool projMask = false;

	// OPL ~ OPR + 9
	LayerArrays layersM1 = maskLayers.getUpperLayersOfProjectMask();
	LayerArrays layersM2 = maskLayers.getLowerLayersOfProjectMask();

	// ILM ~ IPL
	LayerArrays layersV1 = maskLayers.getUpperLayersOfSuperficial();
	LayerArrays layersV2 = maskLayers.getLowerLayersOfSuperficial();
	bool outerFlow = maskLayers.isOuterRetinaFlows();

	if (outerFlow && layout.scanRangeX() <= DECORR_PROJECT_REMOVE_RANGE &&
		maskDecorr.size() == projSize && maskDiffer.size() == projSize) {
		projMask = true;
	}

	LogD() << "Updating projection, lines: " << lines << ", points: " << points << ", projMask: " << projMask << ", outFlow: " << outerFlow;


	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, width, height, lines, points, layers1, layers2, vertical, 
				calcStats, calcMask, reflectCorrect,
				projMask, layersM1, layersM2, layersV1, layersV2, maskDecorr, maskDiffer, outerFlow, this]()
			{
				for (int i : tasks[k]) {
					auto upper = layers1[i];
					auto lower = layers2[i];

					auto &cImage = getImpl().decorrelations[i];
					auto &dImage = getImpl().differentials[i];
					auto &tImage = getImpl().intensities[i];
					// auto &wImage = getImpl().differWeights[i];

					const float* decorr = (const float*)cImage.getBitsData();
					const float* differ = (const float*)dImage.getBitsData();
					const float* intens = (const float*)tImage.getBitsData();
					// const float* weights = (const float*)wImage.getBitsData();

					bool fromBase = false;
					if (layersM2.size() > i && layersM2[i].size() == width) {
						fromBase = true;
					}

					int index = 0;
					for (int c = 0; c < width; c++) {
						if (vertical) {
							index = c * lines + i;
						}
						else {
							index = i * width + c;
						}

						float dcMax = 0.0f, dcSum = 0.0f, dcAvg = 0.0f;
						float dfMax = 0.0f, dfSum = 0.0f, dfAvg = 0.0f;
						float itMax = 0.0f;
						float offset = 0.0f;
						float depth = 0.0f;
						float coeff = 0.0f;

						for (int r = upper[c]; r < lower[c]; r++) {
							int idx = r * width + c;
							float dc = decorr[idx];
							float df = differ[idx];
							float it = intens[idx];

							if (projMask) {
								/*
								if (r >= layersM1[i][c] && r <= layersM2[i][c]) {
								dc = max(dc - maskDecorr[index], 0.0f);
								df = max(df - maskDiffer[index], 0.0f);
								}
								*/

								// It assumes that the all pixels in inner retina till photoreceptors are perturbated by projection artifacts, 
								// on the other hand, the choroidal region would be barely affected since its intensity is attentuated significantly
								// right after the photoreceptors. 
								if (r <= layersM2[i][c]) {
									if (r >= layersM1[i][c] && r <= layersM2[i][c]) {
										if (reflectCorrect) {
											// The decorrelation level of projection mask should be attenuated before subtracted
											// from the corrected region. 
											dc = max(dc - maskDecorr[index] * DECORR_REFLECT_WEIGHT, 0.0f);
											df = max(df - maskDiffer[index], 0.0f);
										}
										else {
											dc = max(dc - maskDecorr[index], 0.0f);
											df = max(df - maskDiffer[index], 0.0f);
										}
									}
									else {
										dc = max(dc - maskDecorr[index], 0.0f);
										df = max(df - maskDiffer[index], 0.0f);
									}
								}
							}
							else {
								/*
								if (!calcMask) {
									if (layersV1.size() > i && layersV1[i].size() > c) {
										int span = layersV2[i][c] - layersV1[i][c];
										if (span < 25) {
											if (r >= layersV1[i][c] && r <= layersV2[i][c]) {
												float d = abs(layersV1[i][c] - r) + 1.0f;
												float r = min(d * d * 0.005f, 1.0f);
												df = df * r;
												// dc = dc * r;
											}
										}
									}
								}
								*/
							}
							
							if (outerFlow) {
								coeff = 1.0f / (1 + exp(-1.0f * (10.0f*dc - 1.5f)));
							}
							else {
								coeff = 0.75f + (log10(dc + 10.0f) - 1.0f) * 40.0f;
							}

							dcSum += dc;
							dfSum += (df * coeff);

							if (dcMax <= dc) {
								dfMax = df;
								dcMax = dc;
								itMax = it;
								
								if (fromBase) {
									offset = max((float)(layersM2[i][c] - r), 0.0f);
								}
								else {
									offset = max((float)(lower[c] - r), 0.0f);
								}
							}

							if (calcStats) {
								getImpl().bscanDecorrLists[i].push_back(dc);
								getImpl().bscanIntensLists[i].push_back(it);
							}
							depth += 1.0f;
						}

						dcAvg = (depth > 0.0f ? (dcSum / depth) : 0.0f);
						dfAvg = (depth > 0.0f ? (dfSum / depth) : 0.0f);

						getImpl().decorrProjection[index] = ((calcMask || !outerFlow) ? dcAvg : dcSum);// dcMax; // dcSum;
						getImpl().differProjection[index] = ((calcMask || !outerFlow) ? dfAvg : dfSum); // (outerFlow ? dfSum : dfAvg);
						getImpl().decorrProjection2[index] = dcMax;
						getImpl().intensProjection[index] = itMax;
						getImpl().offsetProjection[index] = offset;
					}
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	getImpl().projectWidth = layout.getWidth();
	getImpl().projectHeight = layout.getHeight();
	getImpl().differAngiogram = getImpl().differProjection;
	getImpl().decorrAngiogram = getImpl().decorrProjection;
	return true;
}


bool oct_angio::AngioDecorr::updateProjectionProfiles2(const AngioLayout & layout, const LayerArrays & uppers, const LayerArrays & lowers, 
										bool calcStats, bool calcMask, bool reflectCorrect, 
										const AngioLayers & maskLayers, const std::vector<float>& maskDecorr, const std::vector<float>& maskDiffer)
{
	int lines = layout.numberOfLines();
	int points = layout.numberOfPoints();
	int width = points;
	int height = DECORR_IMAGE_HEIGHT;
	bool vertical = layout.isVerticalScan();

	LayerArrays layers1 = uppers; // layers.getUpperLayers();
	LayerArrays layers2 = lowers; // layers.getLowerLayers();

	int projSize = lines * points;
	getImpl().differProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection = vector<float>(projSize, 0.0f);
	getImpl().decorrProjection2 = vector<float>(projSize, 0.0f);
	getImpl().intensProjection = vector<float>(projSize, 0.0f);
	getImpl().offsetProjection = vector<float>(projSize, 0.0f);

	if (calcStats) {
		getImpl().bscanDecorrLists = vector<vector<float>>(lines);
		getImpl().bscanDecorrMeans = vector<float>(lines, 0.0f);
		getImpl().bscanDecorrStdevs = vector<float>(lines, 0.0f);
		getImpl().bscanDecorrMaxVals = vector<float>(lines, 0.0f);

		getImpl().bscanIntensLists = vector<vector<float>>(lines);
		getImpl().bscanIntensMeans = vector<float>(lines, 0.0f);
		getImpl().bscanIntensStdevs = vector<float>(lines, 0.0f);
		getImpl().bscanIntensMaxVals = vector<float>(lines, 0.0f);
	}

	bool projMask = false;
	LayerArrays layersM1 = maskLayers.getUpperLayersOfProjectMask();
	LayerArrays layersM2 = maskLayers.getLowerLayersOfProjectMask();
	LayerArrays layersV1 = maskLayers.getUpperLayersOfSuperficial();
	LayerArrays layersV2 = maskLayers.getLowerLayersOfSuperficial();
	bool outerFlow = maskLayers.isOuterRetinaFlows();

	if (outerFlow && layout.scanRangeX() <= DECORR_PROJECT_REMOVE_RANGE &&
		maskDecorr.size() == projSize && maskDiffer.size() == projSize) {
		projMask = true;
	}

	LogD() << "Updating projection, lines: " << lines << ", points: " << points << ", projMask: " << projMask << ", outFlow: " << outerFlow;

	auto n_workers = std::thread::hardware_concurrency();
	std::vector<std::vector<int>> tasks(n_workers);

	for (int i = 0; i < lines; i++) {
		int k = i % n_workers;
		tasks[k].push_back(i);
	}

	std::vector<std::thread> workers;
	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k, width, height, lines, points, layers1, layers2, vertical, 
				calcStats, calcMask, reflectCorrect,
				projMask, layersM1, layersM2, layersV1, layersV2, maskDecorr, maskDiffer, outerFlow, this]()
			{
				for (int i : tasks[k]) {
					auto upper = layers1[i];
					auto lower = layers2[i];

					auto &cImage = getImpl().decorrelations[i];
					auto &dImage = getImpl().differentials[i];
					// auto &wImage = getImpl().differWeights[i];

					const float* decorr = (const float*)cImage.getBitsData();
					const float* differ = (const float*)dImage.getBitsData();
					// const float* weights = (const float*)wImage.getBitsData();

					bool fromBase = false;
					if (layersM2.size() > i && layersM2[i].size() == width) {
						fromBase = true;
					}

					int index = 0;
					for (int c = 0; c < width; c++) {
						if (vertical) {
							index = c * lines + i;
						}
						else {
							index = i * width + c;
						}

						float dcMax = 0.0f, dcSum = 0.0f, dcAvg = 0.0f;
						float dfMax = 0.0f, dfSum = 0.0f, dfAvg = 0.0f;
						float itMax = 0.0f;
						float offset = 0.0f;
						float depth = 0.0f;
						float coeff = 0.0f;
						float w = 0.0f;

						for (int r = upper[c]; r < lower[c]; r++) {
							int idx = r * width + c;
							float dc = decorr[idx];
							float df = differ[idx];


							if (projMask) {
								/*
								if (r >= layersM1[i][c] && r <= layersM2[i][c]) {
								dc = max(dc - maskDecorr[index], 0.0f);
								df = max(df - maskDiffer[index], 0.0f);
								}
								*/

								// It assumes that the all pixels in inner retina till photoreceptors are perturbated by projection artifacts, 
								// on the other hand, the choroidal region would be barely affected since its intensity is attentuated significantly
								// right after the photoreceptors. 
								if (r <= layersM2[i][c]) {
									if (r >= layersM1[i][c] && r <= layersM2[i][c]) {
										if (reflectCorrect) {
											// The decorrelation level of projection mask should be attenuated before subtracted
											// from the corrected region. 
											dc = max(dc - maskDecorr[index] * DECORR_REFLECT_WEIGHT, 0.0f);
											df = max(df - maskDiffer[index], 0.0f);
										}
										else {
											dc = max(dc - maskDecorr[index], 0.0f);
											df = max(df - maskDiffer[index], 0.0f);
										}
									}
									else {
										dc = max(dc - maskDecorr[index], 0.0f);
										df = max(df - maskDiffer[index], 0.0f);
									}
								}
							}
							else {
								if (!calcMask) {
									if (layersV1.size() > i && layersV1[i].size() > c) {
										int span = layersV2[i][c] - layersV1[i][c];
										if (span < 25) {
											if (r >= layersV1[i][c] && r <= layersV2[i][c]) {
												float d = abs(layersV1[i][c] - r) + 1.0f;
												float r = min(d * d * 0.005f, 1.0f);
												df = df * r;
												// dc = dc * r;
											}
										}
									}
								}
							}

							if (outerFlow) {
								coeff = 1.0f / (1 + exp(-1.0f * (10.0f*dc - 1.5f)));
							}
							else {
								coeff = 0.75f + (log10(dc + 10.0f) - 1.0f) * 40.0f;
							}

							dcSum += dc;
							dfSum += (df * coeff);

							if (dcMax <= dc) {
								dfMax = df;
								dcMax = dc;

								if (fromBase) {
									offset = max((float)(layersM2[i][c] - r), 0.0f);
								}
								else {
									offset = max((float)(lower[c] - r), 0.0f);
								}
							}

							if (calcStats) {
								getImpl().bscanDecorrLists[i].push_back(dc);
							}
							depth += 1.0f;
						}

						dcAvg = (depth > 0.0f ? (dcSum / depth) : 0.0f);
						dfAvg = (depth > 0.0f ? (dfSum / depth) : 0.0f);

						getImpl().decorrProjection[index] = ((calcMask || !outerFlow) ? dcAvg : dcSum);// dcMax; // dcSum;
						getImpl().differProjection[index] = ((calcMask || !outerFlow) ? dfAvg : dfSum); // (outerFlow ? dfSum : dfAvg);
						getImpl().decorrProjection2[index] = dcMax;
						getImpl().intensProjection[index] = itMax;
						getImpl().offsetProjection[index] = offset;
					}
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t) {
		t.join();
	});

	getImpl().projectWidth = layout.getWidth();
	getImpl().projectHeight = layout.getHeight();
	getImpl().differAngiogram = getImpl().differProjection;
	getImpl().decorrAngiogram = getImpl().decorrProjection;
	return true;
}


bool oct_angio::AngioDecorr::checkIfDecorrelationsLoaded(int lines, int points, int repeats)
{
	if (getImpl().decorrelations.size() != lines || getImpl().differentials.size() != lines) {
		return false; 
	}

	if (getImpl().decorrelations.empty() || getImpl().differentials.empty()) {
		return false;
	}

	if (getImpl().decorrelations[0].getWidth() != points ||
		getImpl().differentials[0].getWidth() != points) {
		return false;
	}
	return true;
}


bool oct_angio::AngioDecorr::calculateBscanImageStats(const AngioLayout & layout)
{
	int lines = layout.numberOfLines();
	if (getImpl().bscanDecorrLists.size() != lines ||
		getImpl().bscanIntensLists.size() != lines) {
		return false;
	}


	for (int i = 0; i < lines; i++) {
		const auto& decors = getImpl().bscanDecorrLists[i];
		const auto& intens = getImpl().bscanIntensLists[i];

		if (decors.size() > 100) {
			auto sorted = decors;
			sort(sorted.begin(), sorted.end(), greater<float>());
			auto quater = (int)(sorted.size() * 0.05f);
			auto value = sorted[quater];

			auto dvect = vector<float>(quater);
			auto ivect = vector<float>(quater);
			int count = 0;

			for (int i = 0; i < decors.size(); i++) {
				if (decors[i] >= value) {
					dvect[count] = decors[i];
					ivect[count] = intens[i];
					if (++count >= quater) {
						break;
					}
				}
			}

			if (dvect.size() > 0)
			{
				auto first = dvect.begin();
				auto last = dvect.end();
				auto sum = accumulate(first, last, 0.0f);
				auto mean = sum / quater;
				auto vmax = *max_element(dvect.begin(), dvect.end());

				vector<float> diff(quater);
				std::transform(first, last, diff.begin(), [mean](float x) { return x - mean; });
				auto sq_sum = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0f);
				auto stdev = (float)sqrt(sq_sum / (quater - 1.0f));

				getImpl().bscanDecorrMeans[i] = mean;
				getImpl().bscanDecorrStdevs[i] = stdev;
				getImpl().bscanDecorrMaxVals[i] = vmax;
			}
			else {
				getImpl().bscanDecorrMeans[i] = 0.0f;
				getImpl().bscanDecorrStdevs[i] = 0.0f;
				getImpl().bscanDecorrMaxVals[i] = 0.0f;
			}

			if (ivect.size() > 0)
			{
				auto first = ivect.begin();
				auto last = ivect.end();
				auto sum = accumulate(first, last, 0.0f);
				auto mean = sum / quater;
				auto vmax = *max_element(ivect.begin(), ivect.end());

				vector<float> diff(quater);
				std::transform(first, last, diff.begin(), [mean](float x) { return x - mean; });
				auto sq_sum = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0f);
				auto stdev = (float)sqrt(sq_sum / (quater - 1.0f));

				getImpl().bscanIntensMeans[i] = mean;
				getImpl().bscanIntensStdevs[i] = stdev;
				getImpl().bscanIntensMaxVals[i] = vmax;
			}
			else {
				getImpl().bscanIntensMeans[i] = 0.0f;
				getImpl().bscanIntensStdevs[i] = 0.0f;
				getImpl().bscanIntensMaxVals[i] = 0.0f;
			}
		}
		else {
			getImpl().bscanDecorrMeans[i] = 0.0f;
			getImpl().bscanDecorrStdevs[i] = 0.0f;
			getImpl().bscanDecorrMaxVals[i] = 0.0f;
			getImpl().bscanIntensMeans[i] = 0.0f;
			getImpl().bscanIntensStdevs[i] = 0.0f;
			getImpl().bscanIntensMaxVals[i] = 0.0f;
		}
	}
	return true;
}


bool oct_angio::AngioDecorr::calculateProjectionStats(void)
{
	auto intens = getImpl().intensProjection;
	auto decors = getImpl().decorrProjection2;
	auto differs = getImpl().differProjection;

	auto sorted = decors;
	sort(sorted.begin(), sorted.end(), greater<float>());
	auto quater = (int)(sorted.size() * 0.05f);
	auto value = sorted[quater];

	auto dvect = vector<float>(quater);
	auto ivect = vector<float>(quater);
	auto fvect = vector<float>(quater);
	int count = 0;

	for (int i = 0; i < decors.size(); i++) {
		if (decors[i] >= value) {
			dvect[count] = decors[i];
			ivect[count] = intens[i];
			fvect[count] = differs[i];
			if (++count >= quater) {
				break;
			}
		}
	}

	{
		auto first = dvect.begin();
		auto last = dvect.end();
		auto sum = accumulate(first, last, 0.0f);
		auto mean = sum / quater;
		auto vmax = *max_element(dvect.begin(), dvect.end());

		vector<float> diff(quater);
		std::transform(first, last, diff.begin(), [mean](float x) { return x - mean; });
		auto sq_sum = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0f);
		auto stdev = (float)sqrt(sq_sum / (quater - 1.0f));

		getImpl().decorrMean = mean;
		getImpl().decorrStdev = stdev;
		getImpl().decorrMaxVal = vmax;

		LogD() << "Angio projection Decorrelations (5%), avg: " << mean << ", stdev: " << stdev << ", max: " << vmax;
	}

	{
		auto first = ivect.begin();
		auto last = ivect.end();
		auto sum = accumulate(first, last, 0.0f);
		auto mean = sum / quater;
		auto vmax = *max_element(ivect.begin(), ivect.end());

		vector<float> diff(quater);
		std::transform(first, last, diff.begin(), [mean](float x) { return x - mean; });
		auto sq_sum = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0f);
		auto stdev = (float)sqrt(sq_sum / (quater - 1.0f));

		getImpl().intensMean = mean;
		getImpl().intensStdev = stdev;
		getImpl().intensMaxVal = vmax;

		LogD() << "Angio projection Intensities (5%), avg: " << mean << ", stdev: " << stdev << ", max: " << vmax;
	}

	{
		auto first = fvect.begin();
		auto last = fvect.end();
		auto sum = accumulate(first, last, 0.0f);
		auto mean = sum / quater;
		auto vmax = *max_element(fvect.begin(), fvect.end());

		vector<float> diff(quater);
		std::transform(first, last, diff.begin(), [mean](float x) { return x - mean; });
		auto sq_sum = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0f);
		auto stdev = (float)sqrt(sq_sum / (quater - 1.0f));

		getImpl().differMean = mean;
		getImpl().differStdev = stdev;
		getImpl().differMaxVal = vmax;

		LogD() << "Angio projection Differentials (5%), avg: " << mean << ", stdev: " << stdev << ", max: " << vmax;
	}

	return true;
}


bool oct_angio::AngioDecorr::normalizeProjectionProfiles(const AngioLayout& layout, const AngioLayers& layers, float decorrMin, float decorrMax, float differMin, float differMax, float dropoff)
{
	normalizeDifferAngiogram(layout, layers, differMin, differMax, dropoff);
	normalizeDecorrAngiogram(layout, layers, decorrMin, decorrMax, dropoff);
	
	return true;
}


bool oct_angio::AngioDecorr::normalizeDifferAngiogram(const AngioLayout & layout, const AngioLayers & layers, float rangeMin, float rangeMax, float dropoff)
{
	int w = layout.getWidth();
	int h = layout.getHeight();

	auto& projs = getImpl().differAngiogram;
	if (projs.size() <= 0) {
		return false;
	}

	auto fpos = projs.begin();
	auto epos = projs.end();
	auto max_v = *max_element(fpos, epos);
	auto min_v = *min_element(fpos, epos);

	float mean, stdev;
	CvImage image;
	image.fromFloat32((const unsigned char*)&projs[0], w, h);
	image.getMeanStddev(&mean, &stdev);

	float valueMax = min(mean + stdev * 4.5f, max_v);
	float valueMin = max(mean, stdev) * 0.5f;

	if (layers.isOuterRetinaFlows()) {
		valueMin = max(mean, stdev) * 0.25f;
		valueMax = max(valueMax, min_v + 550.0f);
	}
	else {
		valueMin = max(mean, stdev) * 0.25f;
		valueMax = max(valueMax, min_v + 350.0f);
	}
	
	if (rangeMin > 0.0f) {
		valueMin = rangeMin;
	}
	if (rangeMax > 0.0f) {
		valueMax = rangeMax;
	}

	std::for_each(fpos, epos, [&](float &elem) { elem = min(elem, valueMax); });
	std::for_each(fpos, epos, [&](float &elem) { elem = max(elem, valueMin); });

	LogD() << "Angio normalization (Differ.), min: " << min_v << ", max: " << max_v << ", mean: " << mean << ", stdev: " << stdev;
	LogD() << "Differ. Normalization range: " << valueMin << ", " << valueMax;

	if (valueMax > 0.0f) { 
		if (valueMax > valueMin) {
			const float range = valueMax - valueMin;
			std::transform(fpos, epos, fpos, [=](float e) -> float { return e - valueMin; });
			std::transform(fpos, epos, fpos, [=](float e) -> float { return e / range; });

			if (dropoff > 0.0f) {
				std::for_each(fpos, epos, [&](float &elem) { elem = (elem < dropoff ? 0 : elem); });
			}
		}
	}
	return true;
}


bool oct_angio::AngioDecorr::normalizeDecorrAngiogram(const AngioLayout & layout, const AngioLayers & layers, float rangeMin, float rangeMax, float dropoff)
{
	int w = layout.getWidth();
	int h = layout.getHeight();

	auto& projs = getImpl().decorrAngiogram;
	if (projs.size() <= 0) {
		return false;
	}

	auto fpos = projs.begin();
	auto epos = projs.end();
	auto max_v = *max_element(fpos, epos);
	auto min_v = *min_element(fpos, epos);

	float mean, stdev;
	CvImage image;
	image.fromFloat32((const unsigned char*)&projs[0], w, h);
	image.getMeanStddev(&mean, &stdev);

	float valueMax = min(mean + stdev * 4.5f, max_v);
	float valueMin = min(mean, stdev) * 1.0f;

	if (layers.isOuterRetinaFlows()) {
		valueMin = max(mean, stdev) * 0.25f;
		valueMax = max(valueMax, min_v + 0.35f);
	}
	else {
		valueMin = max(mean, stdev) * 0.25f;
		valueMax = max(valueMax, min_v + 0.25f);
	}

	if (rangeMin > 0.0f) {
		valueMin = rangeMin;
	}
	if (rangeMax > 0.0f) {
		valueMax = rangeMax;
	}

	std::for_each(fpos, epos, [&](float &elem) { elem = min(elem, valueMax); });
	std::for_each(fpos, epos, [&](float &elem) { elem = max(elem, valueMin); });

	LogD() << "Angio normalization (Decorr.), min: " << min_v << ", max: " << max_v << ", mean: " << mean << ", stdev: " << stdev;
	LogD() << "Decorr. Normalization range: " << valueMin << ", " << valueMax;

	if (valueMax > 0.0f) {
		if (valueMax > valueMin) {
			const float range = valueMax - valueMin;
			std::transform(fpos, epos, fpos, [=](float e) -> float { return e - valueMin; });
			std::transform(fpos, epos, fpos, [=](float e) -> float { return e / range; });

			if (dropoff > 0.0f) {
				std::for_each(fpos, epos, [&](float &elem) { elem = (elem < dropoff ? 0 : elem); });
			}
		}
	}
	return true;
}


bool oct_angio::AngioDecorr::denoiseProjectionProfiles(int lines, int points, float decorrMin, float decorrMax)
{
	{
		auto& projs = getImpl().decorrAngiogram;
		if (projs.size() <= 0) {
			return false;
		}

		auto fpos = projs.begin();
		auto epos = projs.end();
		auto max_v = *max_element(fpos, epos);
		auto min_v = *min_element(fpos, epos);

		float mean, stdev;
		CvImage image;
		image.fromFloat32((const unsigned char*)&projs[0], points, lines);
		image.getMeanStddev(&mean, &stdev);

		float valueMax = min(mean + stdev * 3.5f, max_v);
		float valueMin = mean * 0.5f;
		valueMax = min(max(valueMax, valueMin + 0.35f), 1.0f);

		if (decorrMin > 0.0f) {
			valueMin = decorrMin;
		}
		if (decorrMax > 0.0f) {
			valueMax = decorrMax;
		}

		LogD() << "Angio normalization (Decorr.), min: " << min_v << ", max: " << max_v << ", mean: " << mean << ", stdev: " << stdev;
		LogD() << "Decorr. Normalization range: " << valueMin << ", " << valueMax;

		std::for_each(fpos, epos, [&](float &elem) { elem = (elem < valueMin ? 0.0f : elem); });
		std::for_each(fpos, epos, [&](float &elem) { elem = (elem > valueMax ? 0.0f : elem); });
	}
	return true;
}

std::vector<CvImage>& oct_angio::AngioDecorr::decorrelations(void) const
{
	return getImpl().decorrelations;
}

std::vector<CvImage>& oct_angio::AngioDecorr::differentials(void) const
{
	return getImpl().differentials;
}


vector<float>& oct_angio::AngioDecorr::decorrAngiogram(void) const
{
	return getImpl().decorrAngiogram;
}

vector<float>& oct_angio::AngioDecorr::differAngiogram(void) const
{
	return getImpl().differAngiogram;
}

vector<float>& oct_angio::AngioDecorr::outputAngiogram(void) const
{
	return getImpl().outputAngiogram;
}

std::vector<float>& oct_angio::AngioDecorr::decorrProjectionMax(void) const
{
	return getImpl().decorrProjection2;
}

std::vector<float>& oct_angio::AngioDecorr::decorrProjectionMask(void) const
{
	return getImpl().decorrProjectionMask;
}

std::vector<float>& oct_angio::AngioDecorr::differProjectionMask(void) const
{
	return getImpl().differProjectionMask;
}

std::vector<float>& oct_angio::AngioDecorr::decorrAxialOffsets(void) const
{
	return getImpl().offsetProjection;
}


cpp_util::CvImage oct_angio::AngioDecorr::decorrAngiogramImage(bool axialMax) const
{
	int w = getImpl().projectWidth;
	int h = getImpl().projectHeight;

	auto& decors = (axialMax ? decorrProjectionMax() : decorrAngiogram());
	if (decors.empty() || decors.size() != w * h) {
		return cpp_util::CvImage();
	}

	CvImage image;
	image.fromFloat32((const unsigned char*)&decors[0], w, h);
	return image;
}


cpp_util::CvImage oct_angio::AngioDecorr::differAngiogramImage(void) const
{
	int w = getImpl().projectWidth;
	int h = getImpl().projectHeight;

	auto& differs = differAngiogram();
	if (differs.empty() || differs.size() != w * h) {
		return cpp_util::CvImage();
	}

	CvImage image;
	image.fromFloat32((const unsigned char*)&differs[0], w, h);
	return image;
}


float * oct_angio::AngioDecorr::getDecorrelationData(int index)
{
	if (getImpl().decorrelations.size() < index) {
		return nullptr;
	}

	auto& image = getImpl().decorrelations[index];
	if (image.getWidth() < 0 || image.getHeight() < 0) {
		return nullptr;
	}

	return (float*)image.getBitsData();
}


float * oct_angio::AngioDecorr::getDifferentialsData(int index)
{
	if (getImpl().differentials.size() < index) {
		return nullptr;
	}

	auto& image = getImpl().differentials[index];
	if (image.getWidth() < 0 || image.getHeight() < 0) {
		return nullptr;
	}

	return (float*)image.getBitsData();
}


cpp_util::CvImage oct_angio::AngioDecorr::getDecorrelationImage(int index)
{
	if (getImpl().decorrelations.size() < index) {
		return cpp_util::CvImage();
	}

	auto& image = getImpl().decorrelations[index];
	if (image.getWidth() < 0 || image.getHeight() < 0) {
		return cpp_util::CvImage();
	}

	return image;
}


cpp_util::CvImage oct_angio::AngioDecorr::getDifferentialsImage(int index)
{
	if (getImpl().differentials.size() < index) {
		return cpp_util::CvImage();
	}

	auto& image = getImpl().differentials[index];
	if (image.getWidth() < 0 || image.getHeight() < 0) {
		return cpp_util::CvImage();
	}

	return image;
}


float & oct_angio::AngioDecorr::backgroundThreshold(void)
{
	return getImpl().backgroundThreshold;
}


void oct_angio::AngioDecorr::getDecorrelationStat(float & mean, float & stdev, float & maxval)
{
	mean = getImpl().decorrMean;
	stdev = getImpl().decorrStdev;
	maxval = getImpl().decorrMaxVal;
	return;
}


void oct_angio::AngioDecorr::getIntensityStat(float & mean, float & stdev, float & maxval)
{
	mean = getImpl().intensMean;
	stdev = getImpl().intensStdev;
	maxval = getImpl().intensMaxVal;
	return;
}


void oct_angio::AngioDecorr::getDecorrelationStatOfBscan(int index, float & mean, float & stdev, float & maxval)
{
	if (index >= 0 && index < getImpl().bscanDecorrMeans.size()) {
		mean = getImpl().bscanDecorrMeans[index];
		stdev = getImpl().bscanDecorrStdevs[index];
		maxval = getImpl().bscanDecorrMaxVals[index];
	}
	else {
		mean = stdev = maxval = 0.0f;
	}
	return;
}


void oct_angio::AngioDecorr::getIntensityStatOfBscan(int index, float & mean, float & stdev, float & maxval)
{
	if (index >= 0 && index < getImpl().bscanIntensMeans.size()) {
		mean = getImpl().bscanIntensMeans[index];
		stdev = getImpl().bscanIntensStdevs[index];
		maxval = getImpl().bscanIntensMaxVals[index];
	}
	else {
		mean = stdev = maxval = 0.0f;
	}
	return;
}


AngioDecorr::AngioDecorrImpl & oct_angio::AngioDecorr::getImpl(void) const
{
	return *d_ptr;
}

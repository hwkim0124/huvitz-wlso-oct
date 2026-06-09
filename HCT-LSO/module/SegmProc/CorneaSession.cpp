#include "pch.h"
#include "CorneaSession.h"
#include "SegmProc2.h"

using namespace segm_proc;


struct CorneaSession::CorneaSessionImpl
{
	bool isDewarping = false;

	CorneaSessionImpl() {

	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<CorneaSession::CorneaSessionImpl> CorneaSession::d_ptr(new CorneaSessionImpl());


CorneaSession::CorneaSession()
{
}


CorneaSession::~CorneaSession()
{
}


bool segm_proc::CorneaSession::processDewarping(void)
{
	getImpl().isDewarping = true;

	if (!initialize()) {
		return false;
	}

	if (!executeDewarping()) {
		return false;
	}

	if (!finalize()) {
		return false;
	}

	if (!employDewarping()) {
		return false;
	}
	return true;
}


bool segm_proc::CorneaSession::employDewarping(void)
{
	int size = getPatternBsegmCount();
	for (int i = 0; i < size; i++) {
		auto data = getScanDataSource()->getPatternContent(i);
		data->employBsegmImage();
	}

	size = getPreviewBsegmCount();
	for (int i = 0; i < size; i++) {
		auto data = getScanDataSource()->getPreviewContent(i);
		data->employBsegmImage();
	}

	return true;
}



bool segm_proc::CorneaSession::initialize(void)
{
	auto& desc = getScanDataSource()->getScanPattern();
	int n_patterns = getScanDataSource()->getPatternContentCount();
	int n_previews = getScanDataSource()->getPreviewContentCount();

	clearAllPatternBsegms();
	for (int i = 0; i < n_patterns; i++) {
		auto bscan = getScanDataSource()->getPatternContent(i);
		if (bscan == nullptr || bscan->isEmpty()) {
			LogD() << "Cornea pattern bscan data is empty!, index: " << i;
			return false;
		}

		auto bsegm = make_unique<CorneaBsegm>();
		bsegm->loadSource(bscan->getImageBuffer(), bscan->getImageWidth(), bscan->getImageHeight());
		bsegm->setPatternDescript(desc);
		addPatternBsegm(std::move(bsegm));
	}
	LogD() << "Cornea session, pattern bscan data loaded, size: " << n_patterns;

	clearAllPreviewBsegms();
	for (int i = 0; i < n_previews; i++) {
		auto bscan = getScanDataSource()->getPreviewContent(i);
		if (bscan == nullptr || bscan->isEmpty()) {
			LogD() << "Cornea preview bscan data is empty!, index: " << i;
			return false;
		}

		auto bsegm = make_unique<CorneaBsegm>();
		bsegm->loadSource(bscan->getImageBuffer(), bscan->getImageWidth(), bscan->getImageHeight());
		bsegm->setPatternDescript(desc);
		addPreviewBsegm(std::move(bsegm));
	}
	LogD() << "Cornea session, preview bscan data loaded, size: " << n_previews;
	return true;
}


bool segm_proc::CorneaSession::executeDewarping(void)
{
	auto n_workers = std::thread::hardware_concurrency();

	std::vector<std::vector<OcularBsegm*>> tasks(n_workers);
	int size = getPatternBsegmCount();

	for (int i = 0; i < size; i++) {
		auto bsegm = getPatternBsegm(i);
		LogD() << "Perform dewarping, pattern image: " << i << " / " << size;
		LogD() << "name: " << wtoa(getScanDataSource()->getPatternContent(i)->getImageName());

		if (bsegm != nullptr) {
			bsegm->setImageIndex(i);
			int k = i % n_workers;
			tasks[k].push_back(bsegm);
		}
	}

	std::vector<std::thread> workers;

	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k]() {
				for (auto p : tasks[k]) {
					bool result = ((CorneaBsegm*)p)->performDewarping();
				}
			}));
		}
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread& t)
	{
		t.join();
	});


	std::vector<std::vector<OcularBsegm*>> tasks2(n_workers);
	size = getPreviewBsegmCount();

	for (int i = 0; i < size; i++) {
		auto bsegm = getPreviewBsegm(i);

		LogD() << "Perform dewarping, preview image: " << i << " / " << size;
		LogD() << "name: " << wtoa(getScanDataSource()->getPreviewContent(i)->getImageName());

		if (bsegm != nullptr) {
			int k = i % n_workers;
			tasks2[k].push_back(bsegm);
		}
	}

	std::vector<std::thread> workers2;

	for (unsigned int k = 0; k < n_workers; k++) {
		if (tasks2[k].size() > 0) {
			workers2.push_back(std::thread([tasks2, k]() {
				for (auto p : tasks2[k]) {
					bool result = ((CorneaBsegm*)p)->performDewarping();
				}
			}));
		}
	}

	std::for_each(workers2.begin(), workers2.end(), [](std::thread& t)
	{
		t.join();
	});

	return true;
}


CorneaSession::CorneaSessionImpl & segm_proc::CorneaSession::getImpl(void)
{
	return *d_ptr;
}

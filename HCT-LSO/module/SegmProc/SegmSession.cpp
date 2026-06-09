#include "pch.h"
#include "SegmSession.h"
#include "SegmProc2.h"

using namespace segm_proc;


struct SegmSession::SegmSessionImpl
{
	ProtocolSource* data;
	vector<unique_ptr<OcularBsegm>> patterns;
	vector<unique_ptr<OcularBsegm>> previews;

	SegmSessionImpl() : data(nullptr) {

	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<SegmSession::SegmSessionImpl> SegmSession::d_ptr(new SegmSessionImpl());


SegmSession::SegmSession()
{
}


SegmSession::~SegmSession()
{
}


bool segm_proc::SegmSession::loadScanData(oct_report::ProtocolSource* data)
{
	if (data) {
		if (data->getPatternContentCount() > 0) {
			getImpl().data = data;
			return true;
		}
	}
	return false;
}


bool segm_proc::SegmSession::processScanData(void)
{
	if (!initialize()) {
		return false;
	}

	if (!execute()) {
		return false;
	}

	if (!rectify()) {
		return false;
	}

	if (!finalize()) {
		return false;
	}

	return true;
}


bool segm_proc::SegmSession::initialize(void)
{
	int overlaps = getScanDataSource()->getScanPattern().getScanOverlaps();

	clearAllPatternBsegms();
	for (int i = 0; i < getScanDataSource()->getPatternContentCount(); i++) {
		auto bscan = getScanDataSource()->getPatternContent(i);
		if (bscan == nullptr || bscan->isEmpty()) {
			LogW() << i << "th bscan data is empty!";
			return false;
		}

		auto bsegm = make_unique<OcularBsegm>();
		// if (!(i % overlaps)) {
		bsegm->loadSource(bscan->getImageBuffer(), bscan->getImageWidth(), bscan->getImageHeight());
		bsegm->setPatternDescript(getScanDataSource()->getScanPattern());
		// }
		addPatternBsegm(std::move(bsegm));
	}
	LogD() << "SegmSession measure bscans: " << getScanDataSource()->getPatternContentCount();

	clearAllPreviewBsegms();
	for (int i = 0; i < getScanDataSource()->getPreviewContentCount(); i++) {
		auto bscan = getScanDataSource()->getPreviewContent(i);
		if (bscan == nullptr || bscan->isEmpty()) {
			LogW() << i << "th preview data is empty!";
			return false;
		}

		auto bsegm = make_unique<OcularBsegm>();
		bsegm->loadSource(bscan->getImageBuffer(), bscan->getImageWidth(), bscan->getImageHeight());
		bsegm->setPatternDescript(getScanDataSource()->getScanPattern());
		addPreviewBsegm(std::move(bsegm));
	}
	LogD() << "SegmSession preview bscans: " << getScanDataSource()->getPreviewContentCount();
	return true;
}


bool segm_proc::SegmSession::execute(void)
{
	auto n_workers = std::thread::hardware_concurrency();

	std::vector<std::vector<OcularBsegm*>> tasks(n_workers);
	auto size = getPatternBsegmCount();

	for (int i = 0; i < size; i++) {
		auto bsegm = d_ptr->patterns[i].get();
		auto bname = wtoa(getScanDataSource()->getPatternContent(i)->getImageName());
		LogD() << "Segmentating pattern image: " << i << " / " << size << ", name: " << bname;
		if (bsegm != nullptr) {
			bsegm->setImageIndex(i);
			int k = i % n_workers;
			tasks[k].push_back(bsegm);
		}
	}

	std::vector<std::thread> workers;
	auto n_threads = min((unsigned int)size, n_workers);

	for (unsigned int k = 0; k < n_threads; k++) {
		if (tasks[k].size() > 0) {
			workers.push_back(std::thread([tasks, k]() {
				for (auto p : tasks[k]) {
					bool result = p->performAnalysis();
					if (!result) {
						LogD() << "Layers not identified in pattern image, index: " << p->getImageIndex();
					}
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
		auto bsegm = d_ptr->previews[i].get();
		auto bname = wtoa(getScanDataSource()->getPreviewContent(i)->getImageName());

		LogD() << "Segmentating preview image: " << i << " / " << size << ", name: " << bname;
		if (bsegm != nullptr) {
			int k = i % n_workers;
			tasks2[k].push_back(bsegm);
		}
	}

	std::vector<std::thread> workers2;
	n_threads = min((unsigned int)size, n_workers);

	for (unsigned int k = 0; k < n_threads; k++) {
		if (tasks2[k].size() > 0) {
			workers2.push_back(std::thread([tasks2, k]() {
				for (auto p : tasks2[k]) {
					bool result = p->performAnalysis();
					if (!result) {
						LogD() << "Layers not identified in preview image, index: " << p->getImageIndex();
					}
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


bool segm_proc::SegmSession::rectify(void)
{
	return true;
}


bool segm_proc::SegmSession::finalize(void)
{
	for (int i = 0; i < getScanDataSource()->getPatternContentCount(); i++) {
		auto bscan = getScanDataSource()->getPatternContent(i);
		bscan->setOcularBsegm(std::move(d_ptr->patterns[i]));
	}

	for (int i = 0; i < getScanDataSource()->getPreviewContentCount(); i++) {
		auto bscan = getScanDataSource()->getPreviewContent(i);
		bscan->setOcularBsegm(std::move(d_ptr->previews[i]));
	}
	return true;
}


ProtocolSource* segm_proc::SegmSession::getScanDataSource(void) const
{
	return d_ptr->data;
}


int segm_proc::SegmSession::getPatternBsegmCount(void) const
{
	return (int)d_ptr->patterns.size();
}


void segm_proc::SegmSession::clearAllPatternBsegms(void)
{
	d_ptr->patterns.clear();
	return;
}


void segm_proc::SegmSession::addPatternBsegm(std::unique_ptr<OcularBsegm> bsegm)
{
	d_ptr->patterns.push_back(std::move(bsegm));
	return;
}


segm_scan::OcularBsegm* segm_proc::SegmSession::getPatternBsegm(int index) const
{
	if (index >= 0 && index < getPatternBsegmCount()) {
		return d_ptr->patterns[index].get();
	}
	return nullptr;
}

int segm_proc::SegmSession::getPreviewBsegmCount(void) const
{
	return (int)d_ptr->previews.size();
}

void segm_proc::SegmSession::clearAllPreviewBsegms(void)
{
	d_ptr->previews.clear();
	return;
}

void segm_proc::SegmSession::addPreviewBsegm(std::unique_ptr<segm_scan::OcularBsegm> bsegm)
{
	d_ptr->previews.push_back(std::move(bsegm));
	return;
}

segm_scan::OcularBsegm* segm_proc::SegmSession::getPreviewBsegm(int index) const
{
	if (index >= 0 && index < getPreviewBsegmCount()) {
		return d_ptr->previews[index].get();
	}
	return nullptr;
}


SegmSession::SegmSessionImpl& segm_proc::SegmSession::getImpl(void)
{
	return *d_ptr;
}

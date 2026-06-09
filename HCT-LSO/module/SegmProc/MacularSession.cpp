#include "pch.h"
#include "MacularSession.h"
#include "SegmProc2.h"

using namespace segm_proc;


struct MacularSession::MacularSessionImpl
{
	MacularSessionImpl() {

	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<MacularSession::MacularSessionImpl> MacularSession::d_ptr(new MacularSessionImpl());


MacularSession::MacularSession()
{
}


MacularSession::~MacularSession()
{
}


bool segm_proc::MacularSession::initialize(void)
{
	auto& desc = getScanDataSource()->getScanPattern();
	int overlaps = desc.getScanOverlaps();
	int n_patterns = getScanDataSource()->getPatternContentCount();
	int n_previews = getScanDataSource()->getPreviewContentCount();

	clearAllPatternBsegms();
	for (int i = 0; i < n_patterns; i++) {
		auto bscan = getScanDataSource()->getPatternContent(i);
		if (bscan == nullptr || bscan->isEmpty()) {
			LogD() << "Macular pattern bscan data is empty!, index: " << i;
			return false;
		}

		// LogD() << i << " : " << bscan->getImageBuffer() << ", " << bscan->getImageWidth() << ", " << bscan->getImageHeight();
		auto bsegm = make_unique<MacularBsegm>();
		// if (!(i % overlaps)) {
			bsegm->loadSource(bscan->getImageBuffer(), bscan->getImageWidth(), bscan->getImageHeight());
		// }
		bsegm->setPatternDescript(desc);
		addPatternBsegm(std::move(bsegm));
	}

	LogD() << "Macular session, pattern bscan data loaded, size: " << n_patterns;

	clearAllPreviewBsegms();
	for (int i = 0; i < n_previews; i++) {
		auto bscan = getScanDataSource()->getPreviewContent(i);
		if (bscan == nullptr || bscan->isEmpty()) {
			LogD() << "Macular preview bscan data is empty!, index: " << i;
			return false;
		}

		auto bsegm = make_unique<MacularBsegm>();
		bsegm->loadSource(bscan->getImageBuffer(), bscan->getImageWidth(), bscan->getImageHeight());
		bsegm->setPatternDescript(desc);
		addPreviewBsegm(std::move(bsegm));
	}

	LogD() << "Macular session, preview bscan data loaded, size: " << n_previews;
	return true;
}


MacularSession::MacularSessionImpl & segm_proc::MacularSession::getImpl(void)
{
	return *d_ptr;
}

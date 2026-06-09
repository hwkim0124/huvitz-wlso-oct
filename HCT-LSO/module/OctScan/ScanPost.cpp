#include "pch.h"
#include "ScanPost.h"

#include <mutex>
#include <atomic>
#include <functional>

#include "oct_grab.h"
#include "oct_pattern.h"
#include "sig_chain.h"

using namespace oct_scan;
using namespace oct_grab;
using namespace oct_pattern;
using namespace sig_chain;


std::mutex ScanPost::singleMutex_;


struct ScanPost::ScanPostImpl
{
	MainBoard* board;
	OctProcess* process;

	bool initiated;
	bool measureDone;

	mutex mutexPost;
	atomic<bool> posterRun;
	atomic<bool> postComplete;
	thread threadPost;

	ScanPostImpl() : initiated(false), posterRun(false), postComplete(false), process(nullptr), 
		board(nullptr), measureDone(false)
	{
	}
};



ScanPost::ScanPost() :
	d_ptr(std::make_unique<ScanPostImpl>())
{
}


ScanPost::~ScanPost()
{
	if (getImpl().threadPost.joinable()) {
		// getImpl().threadPost.join();
		getImpl().threadPost.detach();
	}
}


ScanPost* oct_scan::ScanPost::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static ScanPost instance;
	return &instance;
}


void oct_scan::ScanPost::initiateScanPost(wso_device::MainBoard* board, sig_chain::OctProcess* process)
{
	getImpl().board = board;
	getImpl().process = process;
	getImpl().initiated = true;
	getImpl().measureDone = false;
	return;
}

void oct_scan::ScanPost::releaseScanPost(void)
{
	closeScanPost();

	getImpl().initiated = false;
	return;
}


bool oct_scan::ScanPost::postMeasure(bool result)
{
	getImpl().measureDone = result;
	return startScanPost();
}


bool oct_scan::ScanPost::isInitiated(void) const
{
	return getImpl().initiated;
}


bool oct_scan::ScanPost::startScanPost(void) 
{
	if (!isInitiated()) {
		return false;
	}

	closeScanPost();

	if (getImpl().threadPost.joinable()) {
		getImpl().threadPost.join();
	}

	getImpl().threadPost = thread{ &ScanPost::threadPostFunction, this };
	return true;
}


bool oct_scan::ScanPost::closeScanPost(void)
{
	if (isPosting()) {
		getImpl().posterRun = false;

		if (getImpl().threadPost.joinable()) {
			getImpl().threadPost.join();
		}
	}
	return true;
}


bool oct_scan::ScanPost::isPosting(void) const
{
	return getImpl().posterRun;
}


bool oct_scan::ScanPost::isProcessingCompleted(void) const
{
	return getImpl().postComplete;
}


void oct_scan::ScanPost::threadPostFunction(void)
{
	getImpl().posterRun = true;
	getImpl().postComplete = false;
	bool init = true;

	StopWatch::start(1);
	LogD() << "ScanPost thread started...";

	while (true)
	{
		if (isPosting()) {
			if (!waitForMeasureProcessed(init)) {
				break;
			}
			else {
				init = false;
				if (checkIfMeasureProcessed()) {
					getImpl().postComplete = true;
					break;
				}
			}
		}
		else {
			break;
		}
	}

	double msec = StopWatch::getElapsedMillis(1);
	LogD() << "Measure pattern processed, completed: " << getImpl().postComplete << ", elapsed: " << msec;

	getImpl().posterRun = false;
	bool result = completeMeasureResult();

	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->runOctScanProtocolCompleted(result);
	}

	LogD() << "ScanPost thread exiting";
	return;
}


bool oct_scan::ScanPost::waitForMeasureProcessed(bool init)
{
	static int waitCnt = 0;

	if (init) {
		waitCnt = 0;
	}

	if (waitCnt < SCANNER_WAIT_MEASURE_PROCESSED_COUNT_MAX) {
		this_thread::sleep_for(chrono::milliseconds(SCANNER_WAIT_MEASURE_PROCESSED_DELAY));
		return true;
	}
	LogD() << "Measure pattern processing timeout";
	return false;
}


bool oct_scan::ScanPost::checkIfMeasureProcessed(void)
{
	if (!getProcess()->isBusying()) {
		this_thread::sleep_for(chrono::milliseconds(SCANNER_WAIT_MEASURE_PROCESSED_DELAY_NEXT));
		return true;
	}
	return false;
}


bool oct_scan::ScanPost::completeMeasureResult(void)
{
	bool result = getImpl().measureDone && isProcessingCompleted();

	if (auto* p = EventRegistry::getInstance(); p) {
		p->runOctProtocolMeasureCompleted(result);
	}
	
	LogD() << "Measure pattern completed, result: " << result;
	// Resume Ir camera threads. 
	// d_ptr->board->getCorneaCamera()->play();
	// d_ptr->board->getRetinaCamera()->play();
	return result;
}


sig_chain::OctProcess * oct_scan::ScanPost::getProcess(void)
{
	return getImpl().process;
}


ScanPost::ScanPostImpl & oct_scan::ScanPost::getImpl(void) const
{
	return *d_ptr;
}

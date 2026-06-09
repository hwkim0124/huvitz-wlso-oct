#include "pch.h"
#include "ScanOptimizer.h"
#include "ScanAutoFocus.h"
#include "ScanAutoPolar.h"
#include "ScanAutoRefer.h"
#include "ScanGrab.h"

#include <mutex>
#include <atomic>

#include "sig_chain.h"

using namespace oct_scan;
using namespace sig_chain;

std::mutex ScanOptimizer::singleMutex_;


struct ScanOptimizer::ScanOptimizerImpl
{
	bool initiated;
	bool noFundus;
	bool measureRetina;
	bool measureCornea;
	bool dioptFocus;
	bool topoMode;
	bool isOnCornea;

	int measureCount;
	bool keepOnError;
	bool referFixed;

	MainBoard* mainboard;

	atomic<bool> stopping;
	atomic<bool> running;
	atomic<bool> completed;
	condition_variable condVarWork;
	thread threadWork;
	mutex mutexWork;

	ScanOptimizerImpl() : initiated(false), mainboard(nullptr), 
		running(false), stopping(false), completed(false), measureRetina(false), measureCornea(false),
		referFixed(false), dioptFocus(false), topoMode(false), isOnCornea(false)
	{
	}
};


oct_scan::ScanOptimizer::ScanOptimizer() :
	d_ptr(std::make_unique<ScanOptimizerImpl>())
{
}


oct_scan::ScanOptimizer::~ScanOptimizer()
{
	if (impl().threadWork.joinable()) {
		// impl().threadWork.join();
		impl().threadWork.detach();
	}
}


ScanOptimizer* oct_scan::ScanOptimizer::getInstance(void)
{
	// Instance is constructed by public static method.
	// Static local variable initialization is thread-safe 
	// and will be initailized only once. 
	static ScanOptimizer instance;
	return &instance;
}

bool oct_scan::ScanOptimizer::initializeScanOptimizer(wso_device::MainBoard * board)
{
	impl().mainboard = board;
	impl().initiated = true;
	return true;
}


bool oct_scan::ScanOptimizer::isInitialized(void)
{
	return impl().initiated;
}


bool oct_scan::ScanOptimizer::initiateAutoScanOptimize(bool on_cornea)
{
	if (!isInitialized()) {
		return false;
	}
	ceaseOptimizing();

	impl().isOnCornea = on_cornea;

	d_ptr->dioptFocus = false;
	d_ptr->completed = false;
	impl().threadWork = thread{ &ScanOptimizer::threadAutoOptimizeFunction, this };
	return true;
}


bool oct_scan::ScanOptimizer::initiateAutoPosition(void)
{
	if (!isInitialized()) {
		return false;
	}
	ceaseOptimizing();

	d_ptr->completed = false;
	impl().threadWork = thread{ &ScanOptimizer::threadAutoPositionFunction, this };
	return true;
}



bool oct_scan::ScanOptimizer::initiateAutoDiopterFocus(OctAutoFocusOptimized callback)
{
	if (!isInitialized()) {
		return false;
	}

	if (!ScanAutoFocus::initializeScanAutoFocus(getMainBoard(), callback)) {
		return false;
	}

	if (!ScanAutoFocus::startOptimizing()) {
		return false;
	}
	return true;
}


bool oct_scan::ScanOptimizer::initiateAutoPolarization(OctAutoPolarOptimized callback)
{
	if (!isInitialized()) {
		return false;
	}
	if (!ScanAutoPolar::initializeScanAutoPolar(getMainBoard(), callback)) {
		return false;
	}

	if (!ScanAutoPolar::startOptimizing()) {
		return false;
	}
	return true;
}


bool oct_scan::ScanOptimizer::initiateAutoReference(OctAutoReferOptimized callback, bool on_cornea)
{
	if (!isInitialized()) {
		return false;
	}

	if (!ScanAutoRefer::initializeScanAutoRefer(getMainBoard(), callback)) {
		return false;
	}

	impl().isOnCornea = on_cornea;

	bool narrowTarget = false; // true;
	bool smallMoveStep = false;// true;
	bool autoCenter = false;
	bool measureMode = false;
	bool upperTarget = false;

	bool isAnterior = on_cornea;
	bool motorInRange = (on_cornea ? false : true);
	if (!ScanAutoRefer::startOptimizing(isAnterior, autoCenter, motorInRange, narrowTarget, smallMoveStep, measureMode, upperTarget)) {
		return false;
	}
	return true;
}


void oct_scan::ScanOptimizer::ceaseAutoReference(bool wait)
{
	ScanAutoRefer::cancelOptimizing(wait);
	return;
}


void oct_scan::ScanOptimizer::ceaseAutoDiopterFocus(bool wait)
{
	ScanAutoFocus::cancelOptimizing(wait);
	return;
}


void oct_scan::ScanOptimizer::ceaseAutoPolarization(bool wait)
{
	ScanAutoPolar::cancelOptimizing(wait);
	return;
}



bool oct_scan::ScanOptimizer::isAutoDiopterFocusing(void)
{
	return ScanAutoFocus::isRunning();
}


bool oct_scan::ScanOptimizer::isAutoPolarizing(void)
{
	return ScanAutoPolar::isRunning();
}


bool oct_scan::ScanOptimizer::isAutoReferencing(void)
{
	return ScanAutoRefer::isRunning();
}


bool oct_scan::ScanOptimizer::isOptimizing(void)
{
	return (impl().running || ScanAutoFocus::isRunning() || ScanAutoPolar::isRunning() || ScanAutoRefer::isRunning());
}


bool oct_scan::ScanOptimizer::isCancelling(void)
{
	return (impl().stopping || ScanAutoFocus::isCancelling() || ScanAutoPolar::isCancelling() || ScanAutoRefer::isCancelling());
}


bool oct_scan::ScanOptimizer::isCompleted(void)
{
	return (impl().completed || ScanAutoFocus::isCompleted() || ScanAutoPolar::isCompleted() || ScanAutoRefer::isCompleted());
}


void oct_scan::ScanOptimizer::ceaseOptimizing(void)
{
	/*
	if (!isOptimizing()) {
		return;
	}
	*/
	if (impl().running) {
		LogD() << "Auto scan optimizer cancelling...";
		impl().stopping = true;
		d_ptr->measureRetina = false;
		d_ptr->measureCornea = false;
		/*
		// Caution:
		// If this routine owning the lock falls into waiting status by calling join,
		// the worker thread can't wake up until the lock released.
		// However, the lock should be held to be the waiting status of the worker thread
		// before notification, RAII fashion within a bracket scope could resolve this problem.
		{
			unique_lock<mutex> lock(impl().mutexWork);
			impl().condVarWork.notify_all();
		}
		*/
	}
	
	// Thread object should be joined before being reused. 
	if (impl().threadWork.joinable()) {
		impl().threadWork.join();
	}

	LogD() << "Auto scan optimizer cancelled";
	impl().stopping = false;

	/*
	if (ScanAutoFocus::isRunning()) {
		LogD() << "Auto focus optimizer cancelling...";
		ceaseAutoDiopterFocus(true);
	}
	if (ScanAutoRefer::isRunning()) {
		LogD() << "Auto reference optimizer cancelling...";
		ceaseAutoReference(true);
	}
	if (ScanAutoPolar::isRunning()) {
		LogD() << "Auto polarization optimizer cancelling...";
		ceaseAutoPolarization(true);
	}
	*/

	ceaseAutoDiopterFocus(true);
	ceaseAutoReference(true);
	ceaseAutoPolarization(true);
	return;
}


ScanOptimizer::ScanOptimizerImpl & oct_scan::ScanOptimizer::impl(void)
{
	return *d_ptr;
}


wso_device::MainBoard * oct_scan::ScanOptimizer::getMainBoard(void)
{
	return impl().mainboard;
}


void oct_scan::ScanOptimizer::threadAutoOptimizeFunction(void)
{
	bool result = false;
	d_ptr->running = true;

	if (!impl().isOnCornea && !d_ptr->dioptFocus) {
		if (initiateAutoReference())
		{
			LogD() << "Auto reference started";

			while (true)
			{
				this_thread::sleep_for(chrono::milliseconds(50));
				if (!ScanAutoRefer::isRunning()) {
					result = ScanAutoRefer::isCompleted();
					LogD() << "Auto reference completed, result: " << result;
					break;
				}
				if (isCancelling()) {
					ScanAutoRefer::cancelOptimizing(true);
					LogD() << "Auto reference canceled";
					goto exit_optim;
				}
			}
		}
	}

	if (!d_ptr->topoMode) {
		if (initiateAutoDiopterFocus())
		{
			LogD() << "Auto diopter focus started";
			while (true)
			{
				this_thread::sleep_for(chrono::milliseconds(50));
				if (!ScanAutoFocus::isRunning()) {
					result = ScanAutoFocus::isCompleted();
					LogD() << "Auto diopter focus completed, result: " << result;
					break;
				}
				if (isCancelling()) {
					ScanAutoFocus::cancelOptimizing(true);
					LogD() << "Auto diopter focus canceled";
					goto exit_optim;
				}
			}
		}
	}

	if (initiateAutoPolarization() && !d_ptr->dioptFocus)
	{
		LogD() << "Auto polarization started";
		while (true)
		{
			this_thread::sleep_for(chrono::milliseconds(50));
			if (!ScanAutoPolar::isRunning()) {
				result = ScanAutoPolar::isCompleted();
				LogD() << "Auto polarization completed, result: " << result;
				break;
			}
			if (isCancelling()) {
				ScanAutoPolar::cancelOptimizing(true);
				LogD() << "Auto polarization canceled";
				goto exit_optim;
			}
		}
	}

exit_optim:
	d_ptr->completed = result;
	d_ptr->running = false;

	LogD() << "Auto optimize completed, result: " << result;
	if (auto* p = CallbackRegistry::getInstance(); p) {
		p->runOctAutoScanOptimized(result);
	}
	return;
}


void oct_scan::ScanOptimizer::threadAutoPositionFunction(void)
{
	/*
	threadAutoReferFunction();

	bool result = isCompleted();
	GlobalRegister::runAutoPositionCompletedCallback(result);
	*/

	bool result = false;
	d_ptr->running = true;

	if (initiateAutoReference())
	{
		while (true)
		{
			this_thread::sleep_for(chrono::milliseconds(50));

			if (!ScanAutoRefer::isRunning()) {
				result = isCompleted();
				break;
			}

			if (isCancelling()) {
				ScanAutoRefer::cancelOptimizing(true);
				break;
			}
		}
	}

	d_ptr->completed = result;
	d_ptr->running = false;

	if (auto* p = CallbackRegistry::getInstance(); p) {
		// p->runOctAutoPositionCompleted(result);
	}
	return;
}

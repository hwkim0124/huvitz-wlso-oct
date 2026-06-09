#include "pch.h"
#include "ScanAutoRefer.h"

#include <mutex>
#include <atomic>

#include "sig_chain.h"

using namespace oct_scan;
using namespace sig_chain;


struct ScanAutoRefer::ScanAutoReferImpl
{
	MainBoard* board;
	bool initiated;
	bool autoCenter;
	bool isAnterior;
	bool motorInRange;
	bool narrowTarget;
	bool smallStep;
	bool measureMode;
	bool upperTarget;

	AutoReferPhase phase;
	float quality;
	float qualityPeak;
	float signalRatio;
	int refPoint;

	int retryCnt;
	int trackCnt;
	int centerCnt;
	int trackDir;
	int moments;
	int centerOffset;

	int mStep;
	int mPosition;
	int mDirection;
	int mPositionPhase;
	int mPositionStart;
	int mPositionPeak;

	OctAutoReferOptimized cbAutoRefer;

	atomic<bool> running;
	atomic<bool> stopping;
	atomic<bool> completed;
	thread threadWork;

	ScanAutoReferImpl() : initiated(false), board(nullptr), cbAutoRefer(nullptr), motorInRange(true), 
							narrowTarget(false), smallStep(false), centerOffset(0), running(false), 
							stopping(false), completed(false), upperTarget(false)
	{
	}
};


// Direct initialization of static smart pointer.
std::unique_ptr<ScanAutoRefer::ScanAutoReferImpl> ScanAutoRefer::d_ptr(new ScanAutoReferImpl());


ScanAutoRefer::ScanAutoRefer()
{
}


ScanAutoRefer::~ScanAutoRefer()
{
	/*
	if (getImpl().threadWork.joinable()) {
		// getImpl().threadWork.join();
		getImpl().threadWork.detach();
	}
	*/
}


bool oct_scan::ScanAutoRefer::initializeScanAutoRefer(wso_device::MainBoard * board, OctAutoReferOptimized callback)
{
	d_ptr->board = board;
	d_ptr->initiated = true;
	d_ptr->autoCenter = false;
	d_ptr->isAnterior = false;
	d_ptr->cbAutoRefer = callback;
	d_ptr->narrowTarget = false;
	d_ptr->smallStep = false;
	d_ptr->centerOffset = 0;
	d_ptr->measureMode = false;
	d_ptr->upperTarget = false;
	return true;
}


bool oct_scan::ScanAutoRefer::isInitialized(void)
{
	return d_ptr->initiated;
}


bool oct_scan::ScanAutoRefer::startOptimizing(bool isAnterior, bool autoCenter, bool motorInRange, bool narrowTarget, bool smallStep, bool measureMode, bool upperTarget)
{
	if (!isInitialized()) {
		return false;
	}

	cancelOptimizing(true);
	resetReferStatus();

	d_ptr->autoCenter = autoCenter;
	d_ptr->isAnterior = isAnterior;
	d_ptr->motorInRange = motorInRange;
	d_ptr->narrowTarget = narrowTarget;
	d_ptr->smallStep = smallStep;
	d_ptr->measureMode = measureMode;
	d_ptr->upperTarget = upperTarget;

	LogD() << "Auto Reference started, anterior: " << isAnterior << ", autoCenter: " << autoCenter << ", motorInRange: " << motorInRange << ", narrowTarget: " << narrowTarget << ", smallStep: " << smallStep << ", measureMode: " << measureMode << ", upperTarget: " << upperTarget; 

	// Thread object should be joined before being reused. 
	if (getImpl().threadWork.joinable()) {
		getImpl().threadWork.join();
	}
	getImpl().threadWork = thread{ &ScanAutoRefer::threadFunction };
	return true;
}


void oct_scan::ScanAutoRefer::cancelOptimizing(bool wait)
{
	if (isRunning() && !isCancelling()) {
		d_ptr->stopping = true;
	}

	if (wait) {
		// Thread object should be joined before being reused. 
		if (getImpl().threadWork.joinable()) {
			getImpl().threadWork.join();
		}
	}
	LogD() << "Auto reference cancelled";

	d_ptr->stopping = false;
	// d_ptr->cbAutoRefer = nullptr;
	return;
}


bool oct_scan::ScanAutoRefer::isRunning(void)
{
	return d_ptr->running;
}


bool oct_scan::ScanAutoRefer::isCancelling(void)
{
	return d_ptr->stopping;
}


bool oct_scan::ScanAutoRefer::isCompleted(void)
{
	return (d_ptr->phase == AutoReferPhase::COMPLETE);
}


bool oct_scan::ScanAutoRefer::isCancelled(void)
{
	return (d_ptr->phase == AutoReferPhase::CANCELED);
}


void oct_scan::ScanAutoRefer::resetReferStatus(void)
{
	d_ptr->phase = AutoReferPhase::INIT;

	d_ptr->running = false;
	d_ptr->stopping = false;
	d_ptr->completed = false;

	d_ptr->quality = 0.0f;
	d_ptr->qualityPeak = 0.0f;
	d_ptr->refPoint = 0;

	resetReferCounts();

	updateMotorPosition();
	d_ptr->mPositionStart = d_ptr->mPosition;
	d_ptr->mPositionPhase = d_ptr->mPosition;
	d_ptr->mPositionPeak = 0;
	return;
}


void oct_scan::ScanAutoRefer::threadFunction(void)
{
	d_ptr->running = true;
	onScanOptimizingStarted();

	while (true)
	{
		if (!processAutoReferPhase()) {
			break;
		}
		if (isCompleted()) {
			LogD() << "Auto reference completed";
			break;
		}
		if (isCancelled()) {
			LogD() << "Auto reference cancelled";
			break;
		}
		if (isCancelling()) {
			LogD() << "Auto reference stopping";
			break;
		}
	}

	d_ptr->running = false;
	onScanOptimizingClosed();
	return;
}


bool oct_scan::ScanAutoRefer::processAutoReferPhase(void)
{
	bool res = false;

	switch (getImpl().phase) {
	case AutoReferPhase::INIT:
		res = phaseAutoRefer_Initiate();
		break;
	case AutoReferPhase::NO_SIGNAL:
	case AutoReferPhase::NO_SIGNAL_REVERSE:
		res = phaseAutoRefer_NoSignal();
		break;
	case AutoReferPhase::CAPTURE:
		res = phaseAutoRefer_Capture();
		break;
	case AutoReferPhase::FORWARD:
		res = phaseAutoRefer_Forward();
		break;
	case AutoReferPhase::RETRACE:
		res = phaseAutoRefer_Retrace();
		break;
	case AutoReferPhase::CENTER:
		res = phaseAutoRefer_Center();
		break;
	}

	if (res) {
		if (getImpl().phase == AutoReferPhase::COMPLETE) {
			phaseAutoRefer_Complete();
		}
		if (getImpl().phase == AutoReferPhase::CANCELED) {
			phaseAutoRefer_Canceled();
		}
		return true;
	}
	else {
		phaseAutoRefer_Canceled();
		return false;
	}
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_Initiate(void)
{
	if (!renewQualityIndex()) {
		return false;
	}

	if (!renewReferencePoint()) {
		return false;
	}

	LogD() << "Auto Refer: start, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();
	checkIfTargetExist();

	if (isTargetFound()) {
		if (d_ptr->autoCenter) {
			changeReferPhase(AutoReferPhase::CENTER);
			LogD() << "Auto Refer: auto position => center, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint;
		}
		else {
			changeReferPhase(AutoReferPhase::CAPTURE);
			LogD() << "Auto Refer: target found => capture, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint;
		}
	}
	else {
		if (d_ptr->autoCenter) {
			changeReferPhase(AutoReferPhase::CANCELED);
			LogD() << "Auto Refer: auto position => target not found";
		}
		else {
			changeReferPhase(AutoReferPhase::NO_SIGNAL);
			LogD() << "Auto Refer: target not found => no_signal";
		}
	}
	return true;
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_NoSignal(void)
{
	if (!moveReferenceMotor()) {
		return false;
	}

	if (!renewQualityIndex()) {
		return false;
	}

	if (isTargetFound())
	{
		if (!renewReferencePoint()) {
			return false;
		}

		if (d_ptr->phase == AutoReferPhase::NO_SIGNAL) {
			changeReferPhase(AutoReferPhase::CAPTURE);
			LogD() << "Auto Refer: target found on forward => capture, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();
		}
		else {
			changeReferPhase(AutoReferPhase::CENTER);
			LogD() << "Auto Refer: target found on reverse => center, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();
		}
	}
	else
	{
		if (d_ptr->phase == AutoReferPhase::NO_SIGNAL) {
			// if (isAtUpperSideEndByOrigin()) {
			if (isAtLowerSideEndByOrigin()) {
				changeReferPhase(AutoReferPhase::NO_SIGNAL_REVERSE, true);
				LogD() << "Auto Refer: forward end => reverse, return to start";
			}
		}
		else {
			// if (isAtLowerSideEndByOrigin()) {
			if (isAtUpperSideEndByOrigin()) {
				changeReferPhase(AutoReferPhase::CANCELED, true);
				LogD() << "Auto Refer: backward end => canceled, return to start";
			}
		}
	}
	return true;
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_Capture(void)
{
	int offset = getTargetOffsetFromCenter();
	int moveDist = (int)(AUTO_REFER_MOVE_DISTANCE_CAPTURE + AUTO_REFER_MOVE_RATIO_TO_OFFSET * offset);

	// Moves the target toward the forward direction to make it the not reverse image.  
	moveDist *= AUTO_REFER_TO_FORWARD;
	LogD() << "Auto Refer: move to capture, offset: " << offset << ", dist: " << moveDist;

	if (!moveReferenceMotor(moveDist)) {
		return false;
	}

	// Whether the target is still on the image, then now it is the not reverse image.
	checkIfTargetExist();

	if (isTargetFound()) {
		if (!renewReferencePoint(true)) {
			return false;
		}

		if (isTargetBelowCenter()) {
			offset = getTargetOffsetFromCenter();
			moveDist = (int)(AUTO_REFER_MOVE_RATIO_TO_OFFSET * offset * 0.5f);
			moveDist *= AUTO_REFER_TO_BACKWARD;
			if (!moveReferenceMotor(moveDist)) {
				return false;
			}
		}

		changeReferPhase(AutoReferPhase::CENTER);
		LogD() << "Auto Refer: target found on capture => center, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();
	}
	else {
		// After a big step toward to eye side, if target is not found, then retreat it to be on image.
		// The initial image must be not reverse.
		moveDist = (int)(AUTO_REFER_MOVE_DISTANCE_CAPTURE + AUTO_REFER_MOVE_RATIO_TO_OFFSET * offset);
		moveDist += (int)(AUTO_REFER_MOVE_RATIO_TO_OFFSET * offset);
		moveDist *= AUTO_REFER_TO_BACKWARD;
		if (!moveReferenceMotor(moveDist)) {
			return false;
		}

		changeReferPhase(AutoReferPhase::CENTER);
		LogD() << "Auto Refer: target empty on capture => center, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();
	}
	return true;
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_Center(void)
{
	if (!renewQualityIndex()) {
		return false;
	}

	if (isTargetFound())
	{
		if (!moveReferenceMotor()) {
			return false;
		}
		else {
			if (isAtLowerSideEndByOrigin()) {
				changeReferPhase(AutoReferPhase::CANCELED);
				LogD() << "Auto Refer: forward end => canceled";
				return true;
			}

			if (isAtUpperSideEndByOrigin()) {
				changeReferPhase(AutoReferPhase::CANCELED);
				LogD() << "Auto Refer: backward end => canceled";
				return true;
			}
		}

		d_ptr->trackCnt++;
		d_ptr->retryCnt = 0;

		if (!renewReferencePoint(true)) {
			return false;
		}

		LogD() << "Auto Refer: center, qidx: " << d_ptr->quality << ", refPoint : " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition() << ", count: " << d_ptr->trackCnt;

		if (isTargetAtCenter()) {
			if (d_ptr->centerCnt++ > 2) {
				changeReferPhase(AutoReferPhase::COMPLETE);
				LogD() << "Auto Refer: center aligned => complete";
				return true;
			}
		}
		else {
			d_ptr->centerCnt = 0;
		}

		if (checkTrackOverToCenter()) {
			changeReferPhase(AutoReferPhase::CANCELED);
			LogD() << "Auto Refer: time over center => cancelled, mpos: " << getReferMotorPosition();
		}
	}
	else
	{
		d_ptr->trackCnt = 0;
		d_ptr->centerCnt = 0;
		d_ptr->retryCnt++;

		if (checkRetryOverToCenter()) {
			changeReferPhase(AutoReferPhase::CANCELED);
			LogD() << "Auto Refer: retry over center => cancelled, mpos: " << getReferMotorPosition();
		}
	}
	return true;
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_Forward(void)
{
	if (!isRetryCount()) {
		if (!moveReferenceMotor()) {
			return false;
		}
		else {
			// if (isAtUpperSideEndByOrigin()) {
			if (isAtLowerSideEndByOrigin()) {
				changeReferPhase(AutoReferPhase::CANCELED);
				LogD() << "Auto Refer: forward end => canceled";
				return true;
			}
		}
	}

	if (!renewQualityIndex()) {
		return false;
	}

	if (isTargetFound())
	{
		d_ptr->trackCnt++;
		d_ptr->retryCnt = 0;

		int prevPos = d_ptr->refPoint;
		if (!renewReferencePoint(false)) {
			return false;
		}

		LogD() << "Auto Refer: track forward, qidx: " << d_ptr->quality << ", refPoint: " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();

		if (d_ptr->trackCnt > 1) {
			int currPos = d_ptr->refPoint;
			int trackVec = currPos - prevPos;
			
			if (trackVec != 0) {
				if (trackVec > 0) {
					d_ptr->moments++;
				}
				else {
					d_ptr->moments = 0;
				}
			}

			if (isTargetBelowCenter()) {
				if (d_ptr->moments >= AUTO_REFER_MOMENTS_FORWARD) {
					changeReferPhase(AutoReferPhase::CENTER);
					LogD() << "Auto Refer: track forward => center";
				}
			}
		}
	}
	else
	{
		d_ptr->trackCnt = 0;
		d_ptr->retryCnt++;

		if (checkRetryOverToForward()) {
			changeReferPhase(AutoReferPhase::RETRACE);
			LogD() << "Auto Refer: retry over forward => retrace";
		}
	}
	return true;
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_Retrace(void)
{
	if (!moveReferenceMotor()) {
		return false;
	}
	else {
		// if (isAtLowerSideEndByOrigin()) {
		if (isAtUpperSideEndByOrigin()) {
			changeReferPhase(AutoReferPhase::CANCELED);
			LogD() << "Auto Refer: backward end => canceled";
			return true;
		}
	}

	if (!renewQualityIndex()) {
		return false;
	}

	if (isTargetFound())
	{
		d_ptr->trackCnt++;
		d_ptr->retryCnt = 0;

		int prevPos = d_ptr->refPoint;
		if (!renewReferencePoint(false)) {
			return false;
		}

		LogD() << "Auto Refer: track retrace, qidx: " << d_ptr->quality << ", refPoint : " << d_ptr->refPoint << ", mpos: " << getReferMotorPosition();

		if (d_ptr->trackCnt > 1) {
			int currPos = d_ptr->refPoint;
			int trackVec = currPos - prevPos;

			if (trackVec != 0) {
				if (trackVec > 0) {
					if (d_ptr->trackDir == AUTO_REFER_TO_FORWARD) {
						d_ptr->moments++;
					}
					else {
						d_ptr->trackDir = AUTO_REFER_TO_FORWARD;
						d_ptr->moments = 1;
					}
				}
				else {
					if (d_ptr->trackDir == AUTO_REFER_TO_BACKWARD) {
						d_ptr->moments++;
					}
					else {
						d_ptr->trackDir = AUTO_REFER_TO_BACKWARD;
						d_ptr->moments = 1;
					}
				}
			}

			if (isTargetInValidRange()) {
				if (d_ptr->trackDir == AUTO_REFER_TO_BACKWARD && d_ptr->moments >= AUTO_REFER_MOMENTS_RETRACE) {
					changeReferPhase(AutoReferPhase::CENTER);
					LogD() << "Auto Refer: track retrace => center";
				}
				if (d_ptr->trackDir == AUTO_REFER_TO_FORWARD && d_ptr->moments >= AUTO_REFER_MOMENTS_FORWARD) {
					changeReferPhase(AutoReferPhase::FORWARD);
					LogD() << "Auto Refer: track retrace => forward";
				}
			}
		}
	}
	else
	{
		d_ptr->trackCnt = 0;
		d_ptr->retryCnt++;

		if (checkRetryOverToRetrace()) {
			changeReferPhase(AutoReferPhase::CANCELED, false);
			LogD() << "Auto Refer: retry over retrace => cancelled";
		}
	}
	return true;
}



bool oct_scan::ScanAutoRefer::phaseAutoRefer_Complete(void)
{
	d_ptr->completed = true;

	if (getImpl().cbAutoRefer != nullptr) {
		(*getImpl().cbAutoRefer)(d_ptr->completed, d_ptr->quality, d_ptr->refPoint, d_ptr->mPosition);
	}
	return true;
}


bool oct_scan::ScanAutoRefer::phaseAutoRefer_Canceled(void)
{
	d_ptr->completed = false;

	if (getImpl().cbAutoRefer != nullptr) {
		(*getImpl().cbAutoRefer)(d_ptr->completed, d_ptr->quality, d_ptr->refPoint, d_ptr->mPosition);
	}
	return true;
}


bool oct_scan::ScanAutoRefer::changeReferPhase(AutoReferPhase phase, bool resetPos)
{
	d_ptr->phase = phase;
	resetReferCounts();

	if (resetPos) {
		if (!returnToStartPosition()) {
			LogD() << "Auto Refer: return to init failed";
			return false;
		}
	}
	else {
		updateMotorPosition();
	}

	d_ptr->mPositionPhase = d_ptr->mPosition;
	return true;
}


void oct_scan::ScanAutoRefer::resetReferCounts(void)
{
	d_ptr->trackCnt = 0;
	d_ptr->trackDir = 0;
	d_ptr->centerCnt = 0;
	d_ptr->moments = 0;
	d_ptr->retryCnt = 0;
	return;
}


bool oct_scan::ScanAutoRefer::isRetryCount(void)
{
	return (d_ptr->retryCnt > 0);
}


bool oct_scan::ScanAutoRefer::checkIfTargetExist(void)
{
	int countMax = AUTO_REFER_RETRY_MAX_CAPTURE;
	bool found = false;

	// Image update would be not fast enough to check target existence.
	// Normal speed : 3 x 2 x 50ms = 300ms
	// Faster speed : 3 x 1 x 50ms = 150ms
	// + 250ms pre-emptive delay.
	if (ChainSetup::getScanPattern().isSpeedNormal()) {
		countMax *= 2;
	}
	else if (ChainSetup::getScanPattern().isSpeedFaster()) {
		countMax *= 1;
	}
	else {
		countMax *= 3;
	}

	// Delay to stabilize image.
	this_thread::sleep_for(chrono::milliseconds(250));

	for (int i = 0; i < countMax; i++) {
		if (!renewQualityIndex()) {
			return false;
		}

		if (isTargetFound()) {
			found = true;
			break;
		}

		this_thread::sleep_for(chrono::milliseconds(50));
	}
	return found;
}


bool oct_scan::ScanAutoRefer::checkTrackOverToCenter(void)
{
	int countMax = AUTO_REFER_CENTER_TRACK_MAX;

	if (ChainSetup::getScanPattern().isSpeedNormal()) {
		countMax *= 1;
	}
	else if (ChainSetup::getScanPattern().isSpeedFaster()) {
		countMax *= 2;
	}
	else {
		countMax *= 1;
	}

	if (isNarrowTarget()) {
		countMax = (int)(countMax * 1.5f);
	}

	if (d_ptr->trackCnt > countMax) {
		return true;
	}
	return false;
}


bool oct_scan::ScanAutoRefer::checkRetryOverToForward(void)
{
	int countMax = AUTO_REFER_RETRY_MAX_FORWARD;

	if (ChainSetup::getScanPattern().isSpeedNormal()) {
		countMax *= 2;
	}
	else if (ChainSetup::getScanPattern().isSpeedFaster()) {
		countMax *= 1;
	}
	else {
		countMax *= 2;
	}

	if (d_ptr->retryCnt > countMax) {
		return true;
	}
	return false;
}


bool oct_scan::ScanAutoRefer::checkRetryOverToRetrace(void)
{
	int countMax = AUTO_REFER_RETRY_MAX_RETRACE;

	if (ChainSetup::getScanPattern().isSpeedNormal()) {
		countMax *= 2;
	}
	else if (ChainSetup::getScanPattern().isSpeedFaster()) {
		countMax *= 1;
	}
	else {
		countMax *= 2;
	}

	if (d_ptr->retryCnt > countMax) {
		return true;
	}
	return false;
}


bool oct_scan::ScanAutoRefer::checkRetryOverToCenter(void)
{
	int countMax = AUTO_REFER_RETRY_MAX_CENTER;

	if (ChainSetup::getScanPattern().isSpeedNormal()) {
		countMax *= 2;
	}
	else if (ChainSetup::getScanPattern().isSpeedFaster()) {
		countMax *= 1;
	}
	else {
		countMax *= 2;
	}

	if (d_ptr->retryCnt > countMax) {
		return true;
	}
	return false;
}


bool oct_scan::ScanAutoRefer::renewQualityIndex(bool next)
{
	float qidx, ratio;
	if (!obtainQualityIndexFromPreview(qidx, ratio, next)) {
		return false;
	}
	getImpl().quality = qidx;
	getImpl().signalRatio = ratio;

	if (getImpl().qualityPeak < qidx) {
		getImpl().qualityPeak = qidx;
		getImpl().mPositionPeak = getMainBoard()->getOctReferMotor()->getPosition();
	}
	return true;
}


bool oct_scan::ScanAutoRefer::renewReferencePoint(bool next)
{
	int refPoint;
	if (!obtainReferencePointFromPreview(refPoint, next)) {
		return false;
	}

	getImpl().refPoint = refPoint;
	return true;
}


bool oct_scan::ScanAutoRefer::isTargetFound(void)
{
	return isQualityToSignal(d_ptr->quality);
}


bool oct_scan::ScanAutoRefer::isTargetAtCenter(void)
{
	int offset = getTargetOffsetFromCenter();
	int centerMin = (isNarrowTarget() ? -35 : AUTO_REFER_CENTER_OFFSET_MIN);
	int centerMax = (isNarrowTarget() ? +35 : AUTO_REFER_CENTER_OFFSET_MAX);
	bool center = false;
	if (offset >= centerMin && offset <= centerMax) {
		center = true;
	}
	d_ptr->centerOffset = offset;
	LogD() << "Auto Refer: target offset: " << offset << ", center: " << centerMin << ", " << centerMax << " => " << center;
	return center;
}


bool oct_scan::ScanAutoRefer::isTargetOutOfCenter(void)
{
	int offset = getTargetOffsetFromCenter();
	int centerMin = AUTO_REFER_CENTER_OFFSET_MIN * 3;
	int centerMax = AUTO_REFER_CENTER_OFFSET_MAX * 3;
	if (offset < centerMin || offset > centerMax) {
		return true;
	}
	return false;
}


bool oct_scan::ScanAutoRefer::isTargetAboveCenter(bool shift)
{
	return (getTargetOffsetFromCenter(shift) < 0);
}


bool oct_scan::ScanAutoRefer::isTargetBelowCenter(bool shift)
{
	return (getTargetOffsetFromCenter(shift) > 0);
}


bool oct_scan::ScanAutoRefer::isTargetInValidRange(void)
{
	int refPoint = d_ptr->refPoint;
	if (refPoint >= AUTO_REFER_VALID_POINT_MIN && refPoint <= AUTO_REFER_VALID_POINT_MAX) {
		return true;
	}
	return false;
}


int oct_scan::ScanAutoRefer::getTargetOffsetFromCenter(bool shift)
{
	int offset; 
	
	if (d_ptr->upperTarget) {
		offset = (d_ptr->refPoint - AUTO_REFER_CENTER_UPPER_LINE);
	}
	else if (!shift) {
		offset = (d_ptr->refPoint - AUTO_REFER_CENTER_LINE);
	}
	else {
		offset = (d_ptr->refPoint - AUTO_REFER_CENTER_LINE_SHIFT);
	}
	return offset;
}

bool oct_scan::ScanAutoRefer::isAnteriorMode(void)
{
	return d_ptr->isAnterior;
}

bool oct_scan::ScanAutoRefer::isMeasureMode(void)
{
	return d_ptr->measureMode;
}

bool oct_scan::ScanAutoRefer::isNarrowTarget(void)
{
	return d_ptr->narrowTarget;
}


bool oct_scan::ScanAutoRefer::useSmallMoveStep(void)
{
	return d_ptr->smallStep;
}


int oct_scan::ScanAutoRefer::updateMotorStep(void)
{
	int mstep;

	switch (d_ptr->phase) {
	case AutoReferPhase::NO_SIGNAL:
	case AutoReferPhase::NO_SIGNAL_REVERSE:
		// if (isAtLowerSideByOrigin() && !useSmallMoveStep()) {
		if (isAtLowerSideByOrigin() && !useSmallMoveStep()) {
			mstep = AUTO_REFER_MOVE_STEP_SEARCH_FAST;
		}
		else {
			mstep = AUTO_REFER_MOVE_STEP_SEARCH;
		}
		break;
	case AutoReferPhase::FORWARD:
	case AutoReferPhase::RETRACE:
		mstep = AUTO_REFER_MOVE_STEP_ALIGN;
		break;
	case AutoReferPhase::CENTER:
	default:
		if (isTargetFound() && !isTargetOutOfCenter()) {
			mstep = AUTO_REFER_MOVE_STEP_CENTER;
		}
		else {
			mstep = AUTO_REFER_MOVE_STEP_ALIGN;
		}
		break;
	}

	mstep = (int)(mstep * (useSmallMoveStep() ? 0.5f : 1.0f));
	d_ptr->mStep = mstep;
	return mstep;
}


int oct_scan::ScanAutoRefer::updateMotorPosition(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	int mpos = motor->getPosition();
	d_ptr->mPosition = mpos;
	return mpos;
}


int oct_scan::ScanAutoRefer::updateMotorDirection(void)
{
	int mdir;

	switch (d_ptr->phase) {
	case AutoReferPhase::NO_SIGNAL:
	case AutoReferPhase::FORWARD:
		mdir = AUTO_REFER_TO_FORWARD;
		break;
	case AutoReferPhase::NO_SIGNAL_REVERSE:
	case AutoReferPhase::RETRACE:
		mdir = AUTO_REFER_TO_BACKWARD;
		break;
	case AutoReferPhase::CENTER:
		mdir = (isTargetAboveCenter() ? AUTO_REFER_TO_FORWARD : AUTO_REFER_TO_BACKWARD);
		break;
	default:
		mdir = d_ptr->mDirection;
		break;
	}

	d_ptr->mDirection = mdir;
	return mdir;
}


int oct_scan::ScanAutoRefer::getMotorOffsetToMove(void)
{
	updateMotorDirection();
	updateMotorStep();

	int offset = d_ptr->mStep * d_ptr->mDirection;
	return offset;
}


int oct_scan::ScanAutoRefer::getReferMotorPosition(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	return motor->getPosition();
}


bool oct_scan::ScanAutoRefer::moveReferenceMotor(void)
{
	int offset = getMotorOffsetToMove();
	return moveReferenceMotor(offset);
}


bool oct_scan::ScanAutoRefer::moveReferenceMotor(int dist)
{
	auto* motor = getMainBoard()->getOctReferMotor();

	if (!motor->updatePositionByOffset(dist)) {
		return false;
	}
	return true;
}


bool oct_scan::ScanAutoRefer::returnToStartPosition(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	int mpos = d_ptr->mPositionStart; //  d_ptr->mPosition;
	if (!motor->updatePosition(mpos)) {
		return false;
	}
	updateMotorPosition();
	return true;
}


bool oct_scan::ScanAutoRefer::returnToPeakPosition(void)
{
	if (d_ptr->qualityPeak > 0.0f) {
		auto* motor = getMainBoard()->getOctReferMotor();
		int mpos = d_ptr->mPositionPeak; //  d_ptr->mPosition;
		if (!motor->updatePosition(mpos)) {
			return false;
		}
		updateMotorPosition();
		return true;
	}
	return false;
}


bool oct_scan::ScanAutoRefer::isMovingForward(void)
{
	return (d_ptr->mDirection == AUTO_REFER_TO_FORWARD);
}


bool oct_scan::ScanAutoRefer::isMovingBackward(void)
{
	return (d_ptr->mDirection == AUTO_REFER_TO_BACKWARD);
}


bool oct_scan::ScanAutoRefer::isAtForwardEnd(void)
{
	if (isMovingForward()) {
		auto* motor = getMainBoard()->getOctReferMotor();
		/*
		if (d_ptr->isAnterior) {
			return motor->isAtUpperSide();
		}
		else {
			return motor->isEndOfUpperPosition();
		}
		*/
		return motor->isEndOfUpperPosition();
	}
	return false;
}


bool oct_scan::ScanAutoRefer::isAtBackwardEnd(void)
{
	if (isMovingBackward()) {
		auto* motor = getMainBoard()->getOctReferMotor();
		/*
		if (d_ptr->isAnterior) {
			return motor->isEndOfLowerPosition();
		}
		else {
			return motor->isAtLowerSide();
		}
		*/
		return motor->isEndOfLowerPosition();
	}
	return false;
}


bool oct_scan::ScanAutoRefer::isAtUpperSideEndByOrigin(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	bool inRange = d_ptr->motorInRange;
	bool onLimit = false;

	if (isMeasureMode()) {
		if (isAnteriorMode()) {
			auto curPos = motor->getPosition();
			auto endPos = motor->getRangeMax();
			if (curPos >= endPos) {
				onLimit = true;
			}
		}
		else {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfRetinaOrigin();
			auto maxPos = motor->getRangeMax();
			auto endPos = orgPos + (maxPos - orgPos) / 2;
			if (curPos >= endPos) {
				onLimit = true;
			}
		}
		inRange = false;
	}
	else {
		if (isAnteriorMode()) {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfCorneaOrigin();
			auto maxPos = motor->getRangeMax();
			auto endPos = orgPos + (maxPos - orgPos) / 4;
			if (curPos >= endPos) {
				onLimit = true;
			}
		}
		else {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfRetinaOrigin();
			auto maxPos = motor->getRangeMax();
			auto endPos = maxPos; 
			if (curPos >= endPos) {
				onLimit = true;
			}
		}
	}

	bool arrived = motor->isEndOfUpperPosition(inRange);
	// bool closing = isMovingForward();
	bool closing = isMovingBackward();
	bool result = ((arrived || onLimit) && closing);
	return result;
}


bool oct_scan::ScanAutoRefer::isAtLowerSideEndByOrigin(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	bool inRange = d_ptr->motorInRange;
	bool onLimit = false;

	if (isMeasureMode()) {
		if (isAnteriorMode()) {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfRetinaOrigin();
			auto minPos = motor->getRangeMin();
			auto endPos = orgPos + (orgPos - minPos) / 2;
			if (curPos <= endPos) {
				onLimit = true;
			}
		}
		else {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfRetinaOrigin();
			auto minPos = motor->getRangeMin();
			auto endPos = minPos + (orgPos - minPos) / 3;
			if (curPos <= endPos) {
				onLimit = true;
			}
		}
		inRange = false;
	}
	else {
		if (isAnteriorMode()) {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfCorneaOrigin();
			auto minPos = motor->getRangeMin();
			auto endPos = orgPos - (orgPos - minPos) / 2;
			if (curPos <= endPos) {
				onLimit = true;
			}
		}
		else {
			auto curPos = motor->getPosition();
			auto orgPos = motor->getPositionOfRetinaOrigin();
			auto minPos = motor->getRangeMin();
			auto endPos = minPos; 
			if (curPos <= endPos) {
				onLimit = true;
			}
		}
	}

	bool arrived = motor->isEndOfLowerPosition(inRange);
	// bool closing = isMovingBackward();
	bool closing = isMovingForward();
	bool result = ((arrived || onLimit) && closing);
	return result;
}


bool oct_scan::ScanAutoRefer::isAtUpperSideByOrigin(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	return (motor->isAtUpperSideByOrigin());
}


bool oct_scan::ScanAutoRefer::isAtLowerSideByOrigin(void)
{
	auto* motor = getMainBoard()->getOctReferMotor();
	return (motor->isAtLowerSideByOrigin());
}


ScanAutoRefer::ScanAutoReferImpl & oct_scan::ScanAutoRefer::getImpl(void)
{
	return *d_ptr;
}


wso_device::MainBoard * oct_scan::ScanAutoRefer::getMainBoard(void)
{
	return d_ptr->board;
}

#include "pch.h"
#include "PupilTracker.h"


using namespace eye_align;


PupilTracker::PupilTracker() :
	config_(), last_(), locked_(false), everLocked_(false), justLocked_(false), justUnlocked_(false),
	consecutiveHits_(0), consecutiveMisses_(0), centerX_(0.0f), centerY_(0.0f), radius_(0.0f)
{}


PupilTracker::PupilTracker(const PupilTrackerConfig& config) :
	PupilTracker()
{
	config_ = config;
}


PupilTracker::~PupilTracker()
{}


void eye_align::PupilTracker::configure(const PupilTrackerConfig& config)
{
	config_ = config;
	if (config_.lockAfterConsecutiveHits < 1) config_.lockAfterConsecutiveHits = 1;
	if (config_.unlockAfterConsecutiveMisses < 1) config_.unlockAfterConsecutiveMisses = 1;
	if (config_.smoothingAlpha <= 0.0f || config_.smoothingAlpha > 1.0f) config_.smoothingAlpha = 1.0f;
}

void eye_align::PupilTracker::reset(void)
{
	last_ = PupilDetectResult();
	locked_ = false;
	everLocked_ = false;
	justLocked_ = false;
	justUnlocked_ = false;
	consecutiveHits_ = 0;
	consecutiveMisses_ = 0;
	centerX_ = centerY_ = radius_ = 0.0f;
}

bool eye_align::PupilTracker::update(const PupilDetectResult& result)
{
	justLocked_ = false;
	justUnlocked_ = false;
	last_ = result;

	if (!result.found) {
		consecutiveHits_ = 0;
		consecutiveMisses_++;
		if (locked_ && consecutiveMisses_ >= config_.unlockAfterConsecutiveMisses) {
			locked_ = false;
			justUnlocked_ = true;
		}
		return false;
	}

	// A detection far away from a fresh lock is treated as an outlier (a miss). If the
	// eye really moved that far, the lock drops after unlockAfterConsecutiveMisses frames and the
	// tracker re-locks on the new position.
	if (locked_ && consecutiveMisses_ == 0) {
		const float deltaX = result.cx - centerX_;
		const float deltaY = result.cy - centerY_;
		if (std::sqrt(deltaX * deltaX + deltaY * deltaY) > config_.maxCenterJumpPixels) {
			consecutiveHits_ = 0;
			consecutiveMisses_++;
			if (consecutiveMisses_ >= config_.unlockAfterConsecutiveMisses) {
				locked_ = false;
				justUnlocked_ = true;
			}
			return false;
		}
	}

	consecutiveMisses_ = 0;
	if (!locked_) {
		consecutiveHits_++;
		if (consecutiveHits_ == 1) {
			centerX_ = result.cx;
			centerY_ = result.cy;
			radius_ = result.radius;
		}
		else {
			const float alpha = config_.smoothingAlpha;
			centerX_ += alpha * (result.cx - centerX_);
			centerY_ += alpha * (result.cy - centerY_);
			radius_ += alpha * (result.radius - radius_);
		}
		if (consecutiveHits_ >= config_.lockAfterConsecutiveHits) {
			locked_ = true;
			everLocked_ = true;
			justLocked_ = true;
		}
	}
	else {
		const float alpha = config_.smoothingAlpha;
		centerX_ += alpha * (result.cx - centerX_);
		centerY_ += alpha * (result.cy - centerY_);
		radius_ += alpha * (result.radius - radius_);
	}
	return justLocked_;
}

bool eye_align::PupilTracker::isLocked(void) const
{
	return locked_;
}

bool eye_align::PupilTracker::hasFix(void) const
{
	return locked_ && consecutiveMisses_ == 0;
}

bool eye_align::PupilTracker::justLocked(void) const
{
	return justLocked_;
}

bool eye_align::PupilTracker::justUnlocked(void) const
{
	return justUnlocked_;
}

bool eye_align::PupilTracker::everLocked(void) const
{
	return everLocked_;
}

int eye_align::PupilTracker::consecutiveHits(void) const
{
	return consecutiveHits_;
}

int eye_align::PupilTracker::lostFrames(void) const
{
	return consecutiveMisses_;
}

float eye_align::PupilTracker::centerX(void) const
{
	return centerX_;
}

float eye_align::PupilTracker::centerY(void) const
{
	return centerY_;
}

float eye_align::PupilTracker::radius(void) const
{
	return radius_;
}

const PupilDetectResult& eye_align::PupilTracker::lastResult(void) const
{
	return last_;
}

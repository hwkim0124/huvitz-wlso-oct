#include "pch.h"
#include "PupilTracker.h"


using namespace eye_align;


PupilTracker::PupilTracker() :
	cfg_(), last_(), locked_(false), everLocked_(false), justLocked_(false), justUnlocked_(false),
	hits_(0), misses_(0), cx_(0.0f), cy_(0.0f), radius_(0.0f)
{}


PupilTracker::PupilTracker(const PupilTrackerConfig& config) :
	PupilTracker()
{
	cfg_ = config;
}


PupilTracker::~PupilTracker()
{}


void eye_align::PupilTracker::configure(const PupilTrackerConfig& config)
{
	cfg_ = config;
	if (cfg_.detectConsecutive < 1) cfg_.detectConsecutive = 1;
	if (cfg_.unlockAfterMisses < 1) cfg_.unlockAfterMisses = 1;
	if (cfg_.smoothingAlpha <= 0.0f || cfg_.smoothingAlpha > 1.0f) cfg_.smoothingAlpha = 1.0f;
}

void eye_align::PupilTracker::reset(void)
{
	last_ = PupilDetectResult();
	locked_ = false;
	everLocked_ = false;
	justLocked_ = false;
	justUnlocked_ = false;
	hits_ = 0;
	misses_ = 0;
	cx_ = cy_ = radius_ = 0.0f;
}

bool eye_align::PupilTracker::update(const PupilDetectResult& result)
{
	justLocked_ = false;
	justUnlocked_ = false;
	last_ = result;

	if (!result.found) {
		hits_ = 0;
		misses_++;
		if (locked_ && misses_ >= cfg_.unlockAfterMisses) {
			locked_ = false;
			justUnlocked_ = true;
		}
		return false;
	}

	// A detection far away from a fresh lock is treated as an outlier (a miss). If the
	// eye really moved that far, the lock drops after unlockAfterMisses frames and the
	// tracker re-locks on the new position.
	if (locked_ && misses_ == 0) {
		const float dx = result.cx - cx_;
		const float dy = result.cy - cy_;
		if (std::sqrt(dx * dx + dy * dy) > cfg_.maxJumpPx) {
			hits_ = 0;
			misses_++;
			if (misses_ >= cfg_.unlockAfterMisses) {
				locked_ = false;
				justUnlocked_ = true;
			}
			return false;
		}
	}

	misses_ = 0;
	if (!locked_) {
		hits_++;
		if (hits_ == 1) {
			cx_ = result.cx;
			cy_ = result.cy;
			radius_ = result.radius;
		}
		else {
			const float a = cfg_.smoothingAlpha;
			cx_ += a * (result.cx - cx_);
			cy_ += a * (result.cy - cy_);
			radius_ += a * (result.radius - radius_);
		}
		if (hits_ >= cfg_.detectConsecutive) {
			locked_ = true;
			everLocked_ = true;
			justLocked_ = true;
		}
	}
	else {
		const float a = cfg_.smoothingAlpha;
		cx_ += a * (result.cx - cx_);
		cy_ += a * (result.cy - cy_);
		radius_ += a * (result.radius - radius_);
	}
	return justLocked_;
}

bool eye_align::PupilTracker::isLocked(void) const
{
	return locked_;
}

bool eye_align::PupilTracker::hasFix(void) const
{
	return locked_ && misses_ == 0;
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
	return hits_;
}

int eye_align::PupilTracker::lostFrames(void) const
{
	return misses_;
}

float eye_align::PupilTracker::cx(void) const
{
	return cx_;
}

float eye_align::PupilTracker::cy(void) const
{
	return cy_;
}

float eye_align::PupilTracker::radius(void) const
{
	return radius_;
}

const PupilDetectResult& eye_align::PupilTracker::lastResult(void) const
{
	return last_;
}

#pragma once

#include "EyeAlign2.h"


namespace eye_align
{
	struct PupilTrackerConfig
	{
		int detectConsecutive = 3;		// consecutive detections needed to lock
		int unlockAfterMisses = 5;		// consecutive misses that drop the lock
		float maxJumpPx = 60.0f;		// a centre jump larger than this counts as a miss
		float smoothingAlpha = 0.5f;	// EMA weight of the newest detection (1 = no smoothing)
	};

	// Temporal filter over per-frame detector results of one camera. Counts frames,
	// not time: update() is called exactly once per camera frame.
	class EYEALIGN_DLL_API PupilTracker
	{
	public:
		PupilTracker();
		explicit PupilTracker(const PupilTrackerConfig& config);
		virtual ~PupilTracker();

	public:
		void configure(const PupilTrackerConfig& config);
		void reset(void);

		// Feeds one frame result. Returns true when this call established the lock.
		bool update(const PupilDetectResult& result);

		bool isLocked(void) const;			// enough consecutive detections seen, not yet dropped
		bool hasFix(void) const;			// locked and the latest frame was a detection
		bool justLocked(void) const;		// lock established by the last update()
		bool justUnlocked(void) const;		// lock dropped by the last update()
		bool everLocked(void) const;		// locked at least once since reset()

		int consecutiveHits(void) const;
		int lostFrames(void) const;			// consecutive frames without a detection

		float cx(void) const;				// smoothed centre / radius of the lock
		float cy(void) const;
		float radius(void) const;
		const PupilDetectResult& lastResult(void) const;

	private:
		PupilTrackerConfig cfg_;
		PupilDetectResult last_;
		bool locked_;
		bool everLocked_;
		bool justLocked_;
		bool justUnlocked_;
		int hits_;
		int misses_;
		float cx_;
		float cy_;
		float radius_;
	};
}

#pragma once

#include "EyeAlign2.h"


namespace eye_align
{
	// Latest-frame mailbox between the camera worker thread and the aligner thread.
	// One slot per IrCameraId; push() copies the frame (a 640x480 memcpy) and wakes the
	// consumer, takeLatest() hands the newest frame out by vector swap. Older frames
	// are dropped, never queued: the control loop always works on the freshest image.
	class EYEALIGN_DLL_API CorneaFrameStore
	{
	public:
		CorneaFrameStore();
		virtual ~CorneaFrameStore();

		CorneaFrameStore(const CorneaFrameStore& rhs) = delete;
		CorneaFrameStore& operator=(const CorneaFrameStore& rhs) = delete;

	public:
		void setAccepting(bool flag);		// false: push() drops frames without copying
		bool isAccepting(void) const;
		void clear(void);

		// Cameras whose frames are kept; push() drops every other camera without copying.
		// A run consumes exactly two of them (the side camera of the eye plus the lower
		// camera), so an idle slot would otherwise fill up and wake the consumer for nothing.
		void setEnabledCameras(std::initializer_list<IrCameraId> cameras);
		bool isCameraEnabled(IrCameraId cameraId) const;

		// Camera thread. Copies the frame into the slot and notifies the consumer.
		void push(IrCameraId cameraId, const std::uint8_t* pixels, int width, int height);

		// Consumer. Blocks until any slot holds a frame newer than the last one taken,
		// stopFlag becomes true, or maxWaitMs elapses. Returns true when a new frame is
		// available.
		bool waitForNewFrame(const std::atomic<bool>& stopFlag, int maxWaitMs);

		// Consumer. Moves the newest frame of cameraId into outFrame (swap). Returns false when
		// that camera has produced nothing newer than the previous take.
		bool takeLatest(IrCameraId cameraId, std::vector<std::uint8_t>& outFrame, int& width, int& height, std::uint64_t& sequence);

		// Wakes a waiting consumer without a frame (used by cancel / hold / resume).
		void wake(void);

	private:
		struct Slot
		{
			std::vector<std::uint8_t> data;
			int width = 0;
			int height = 0;
			std::uint64_t sequence = 0;		// incremented on every push
			std::uint64_t takenSequence = 0;	// sequence of the frame last handed out
		};

		std::array<Slot, IR_CAMERA_COUNT> slots_;
		mutable std::mutex mutex_;
		std::condition_variable frameCondition_;
		std::atomic<bool> accepting_;
		std::atomic<unsigned int> enabledMask_;	// bit per IrCameraId
		std::atomic<bool> wakeupRequested_;

		bool hasNewFrameLocked(void) const;
	};
}

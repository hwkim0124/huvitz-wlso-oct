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

		// Camera thread. Copies the frame into the slot and notifies the consumer.
		void push(IrCameraId camId, const std::uint8_t* bits, int width, int height);

		// Consumer. Blocks until any slot holds a frame newer than the last one taken,
		// stopFlag becomes true, or maxWaitMs elapses. Returns true when a new frame is
		// available.
		bool waitForNewFrame(const std::atomic<bool>& stopFlag, int maxWaitMs);

		// Consumer. Moves the newest frame of camId into out (swap). Returns false when
		// that camera has produced nothing newer than the previous take.
		bool takeLatest(IrCameraId camId, std::vector<std::uint8_t>& out, int& width, int& height, std::uint64_t& seq);

		// Wakes a waiting consumer without a frame (used by cancel / hold / resume).
		void wake(void);

	private:
		struct Slot
		{
			std::vector<std::uint8_t> data;
			int width = 0;
			int height = 0;
			std::uint64_t seq = 0;		// incremented on every push
			std::uint64_t taken = 0;	// seq of the frame last handed out
		};

		std::array<Slot, NUM_IR_CAMERAS> slots_;
		mutable std::mutex mutex_;
		std::condition_variable condVar_;
		std::atomic<bool> accepting_;
		std::atomic<bool> wakeup_;

		bool anyNewFrameLocked(void) const;
	};
}

#include "pch.h"
#include "CorneaFrameStore.h"


using namespace eye_align;


CorneaFrameStore::CorneaFrameStore() :
	slots_(), accepting_(false), enabledMask_((1u << IR_CAMERA_COUNT) - 1u), wakeupRequested_(false)
{}


CorneaFrameStore::~CorneaFrameStore()
{}


void eye_align::CorneaFrameStore::setAccepting(bool flag)
{
	accepting_ = flag;
}

bool eye_align::CorneaFrameStore::isAccepting(void) const
{
	return accepting_;
}

void eye_align::CorneaFrameStore::setEnabledCameras(std::initializer_list<IrCameraId> cameras)
{
	unsigned int mask = 0;
	for (const auto cameraId : cameras) {
		const int index = static_cast<int>(cameraId);
		if (index >= 0 && index < IR_CAMERA_COUNT) {
			mask |= (1u << index);
		}
	}
	enabledMask_ = mask;
}

bool eye_align::CorneaFrameStore::isCameraEnabled(IrCameraId cameraId) const
{
	const int index = static_cast<int>(cameraId);
	if (index < 0 || index >= IR_CAMERA_COUNT) {
		return false;
	}
	return (enabledMask_.load() & (1u << index)) != 0;
}

void eye_align::CorneaFrameStore::clear(void)
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto& slot : slots_) {
		slot.width = 0;
		slot.height = 0;
		slot.sequence = 0;
		slot.takenSequence = 0;
	}
}

void eye_align::CorneaFrameStore::push(IrCameraId cameraId, const std::uint8_t* pixels, int width, int height)
{
	// Cheap early-out on the camera thread when nobody is aligning.
	if (!accepting_ || pixels == nullptr || width <= 0 || height <= 0) {
		return;
	}
	const int index = static_cast<int>(cameraId);
	if (index < 0 || index >= IR_CAMERA_COUNT || (enabledMask_.load() & (1u << index)) == 0) {
		return;
	}

	{
		std::lock_guard<std::mutex> lock(mutex_);
		Slot& slot = slots_[index];
		const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height);
		slot.data.assign(pixels, pixels + size);
		slot.width = width;
		slot.height = height;
		slot.sequence++;
	}
	frameCondition_.notify_one();
}

bool eye_align::CorneaFrameStore::hasNewFrameLocked(void) const
{
	for (const auto& slot : slots_) {
		if (slot.sequence > slot.takenSequence) {
			return true;
		}
	}
	return false;
}

bool eye_align::CorneaFrameStore::waitForNewFrame(const std::atomic<bool>& stopFlag, int maxWaitMs)
{
	std::unique_lock<std::mutex> lock(mutex_);
	frameCondition_.wait_for(lock, std::chrono::milliseconds(maxWaitMs), [&] {
		return stopFlag.load() || wakeupRequested_.load() || hasNewFrameLocked();
	});
	wakeupRequested_ = false;
	return hasNewFrameLocked();
}

bool eye_align::CorneaFrameStore::takeLatest(IrCameraId cameraId, std::vector<std::uint8_t>& outFrame, int& width, int& height, std::uint64_t& sequence)
{
	const int index = static_cast<int>(cameraId);
	if (index < 0 || index >= IR_CAMERA_COUNT) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	Slot& slot = slots_[index];
	if (slot.sequence == slot.takenSequence || slot.width <= 0) {
		return false;
	}
	// Swap buffers: the consumer's previous buffer becomes the slot's next target and
	// gets overwritten by assign() on the next push. No second copy.
	outFrame.swap(slot.data);
	width = slot.width;
	height = slot.height;
	sequence = slot.sequence;
	slot.takenSequence = slot.sequence;
	return true;
}

void eye_align::CorneaFrameStore::wake(void)
{
	wakeupRequested_ = true;
	frameCondition_.notify_all();
}

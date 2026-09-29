#include "pch.h"
#include "CorneaFrameStore.h"


using namespace eye_align;


CorneaFrameStore::CorneaFrameStore() :
	slots_(), accepting_(false), wakeup_(false)
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

void eye_align::CorneaFrameStore::clear(void)
{
	lock_guard<mutex> lock(mutex_);
	for (auto& slot : slots_) {
		slot.width = 0;
		slot.height = 0;
		slot.seq = 0;
		slot.taken = 0;
	}
}

void eye_align::CorneaFrameStore::push(IrCameraId camId, const std::uint8_t* bits, int width, int height)
{
	// Cheap early-out on the camera thread when nobody is aligning.
	if (!accepting_ || bits == nullptr || width <= 0 || height <= 0) {
		return;
	}
	const int index = static_cast<int>(camId);
	if (index < 0 || index >= NUM_IR_CAMERAS) {
		return;
	}

	{
		lock_guard<mutex> lock(mutex_);
		Slot& slot = slots_[index];
		const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height);
		slot.data.assign(bits, bits + size);
		slot.width = width;
		slot.height = height;
		slot.seq++;
	}
	condVar_.notify_one();
}

bool eye_align::CorneaFrameStore::anyNewFrameLocked(void) const
{
	for (const auto& slot : slots_) {
		if (slot.seq > slot.taken) {
			return true;
		}
	}
	return false;
}

bool eye_align::CorneaFrameStore::waitForNewFrame(const std::atomic<bool>& stopFlag, int maxWaitMs)
{
	unique_lock<mutex> lock(mutex_);
	condVar_.wait_for(lock, chrono::milliseconds(maxWaitMs), [&] {
		return stopFlag.load() || wakeup_.load() || anyNewFrameLocked();
	});
	wakeup_ = false;
	return anyNewFrameLocked();
}

bool eye_align::CorneaFrameStore::takeLatest(IrCameraId camId, std::vector<std::uint8_t>& out, int& width, int& height, std::uint64_t& seq)
{
	const int index = static_cast<int>(camId);
	if (index < 0 || index >= NUM_IR_CAMERAS) {
		return false;
	}

	lock_guard<mutex> lock(mutex_);
	Slot& slot = slots_[index];
	if (slot.seq == slot.taken || slot.width <= 0) {
		return false;
	}
	// Swap buffers: the consumer's previous buffer becomes the slot's next target and
	// gets overwritten by assign() on the next push. No second copy.
	out.swap(slot.data);
	width = slot.width;
	height = slot.height;
	seq = slot.seq;
	slot.taken = slot.seq;
	return true;
}

void eye_align::CorneaFrameStore::wake(void)
{
	wakeup_ = true;
	condVar_.notify_all();
}

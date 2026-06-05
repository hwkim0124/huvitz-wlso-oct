#include "pch.h"
#include "FrameImage.h"


#include <memory>
#include <atomic>

using namespace sig_chain;
using namespace std;


struct FrameImage::FrameImageImpl
{
	unique_ptr<unsigned char[]> buffer;
	unique_ptr<float[]> intensity;

	uint8_t lateralBuffer[FRAME_DATA_LINES_MAX];

	int width;
	int height;
	int indexOfImage;

	float qualityIndex;
	float signalRatio;
	int referPoint;

	atomic<int> status;
	atomic<int> type;

	FrameImageImpl() : width(0), height(0), indexOfImage(0),
					qualityIndex(0.0f), signalRatio(0.0f), referPoint(0), 
					status(FrameImage::EMPTY), type(PREVIEW), 
					buffer(nullptr), intensity(nullptr), 
					lateralBuffer{0}
	{
		buffer = make_unique<unsigned char[]>(FRAME_IMAGE_BUFFER_SIZE);
		// intensity = nullptr;
	}
};


FrameImage::FrameImage() :
	d_ptr(make_unique<FrameImageImpl>())
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
sig_chain::FrameImage::~FrameImage() = default;
sig_chain::FrameImage::FrameImage(FrameImage && rhs) = default;
FrameImage & sig_chain::FrameImage::operator=(FrameImage && rhs) = default;


unsigned char * sig_chain::FrameImage::getBuffer(void) const
{
	return &getImpl().buffer[0];
}


float * sig_chain::FrameImage::getIntensity(void) const
{
	if (getImpl().intensity == nullptr) {
		getImpl().intensity = make_unique<float[]>(FFT_OUTPUT_BUFFER_SIZE);
	}
	return &getImpl().intensity[0];
}


void sig_chain::FrameImage::setDimension(int width, int height)
{
	getImpl().width = width;
	getImpl().height = height;
	return;
}


int sig_chain::FrameImage::getWidth(void) const
{
	return getImpl().width;
}


int sig_chain::FrameImage::getHeight(void) const
{
	return getImpl().height;
}


int sig_chain::FrameImage::getIndexOfImage(void)
{
	return getImpl().indexOfImage;
}


void sig_chain::FrameImage::setIndexOfImage(int index)
{
	getImpl().indexOfImage = index;
	return;
}


float sig_chain::FrameImage::getQualityIndex(void) const
{
	return getImpl().qualityIndex;
}


float sig_chain::FrameImage::getSignalRatio(void) const
{
	return getImpl().signalRatio;
}


int sig_chain::FrameImage::getReferencePoint(void) const
{
	return getImpl().referPoint;
}


uint8_t * sig_chain::FrameImage::getLateralBuffer(void) const
{
	return getImpl().lateralBuffer;
}


void sig_chain::FrameImage::setQualityIndex(float value)
{
	getImpl().qualityIndex = value;
	return;
}


void sig_chain::FrameImage::setSignalRatio(float value)
{
	getImpl().signalRatio = value;
	return;
}


void sig_chain::FrameImage::setReferencePoint(int value)
{
	getImpl().referPoint = value;
	return;
}


void sig_chain::FrameImage::setStatus(int status)
{
	getImpl().status = status;
	return;
}


void sig_chain::FrameImage::setCompleted(void)
{
	setStatus(FrameImage::COMPLETED);
	return;
}


void sig_chain::FrameImage::setEmpty(void)
{
	setStatus(FrameImage::EMPTY);
	return;
}


bool sig_chain::FrameImage::isEmpty(void) const
{
	return (getImpl().status == FrameImage::EMPTY);
}


bool sig_chain::FrameImage::isReceiving(void) const
{
	return (getImpl().status == FrameImage::RECEIVING);
}


bool sig_chain::FrameImage::isCompleted(void) const
{
	return (getImpl().status == FrameImage::COMPLETED);
}


bool sig_chain::FrameImage::isProcessing(void) const
{
	return (getImpl().status == FrameImage::PROCESSING);
}


bool sig_chain::FrameImage::isProcessed(void) const
{
	return (getImpl().status == FrameImage::PROCESSED);
}


void sig_chain::FrameImage::setType(int type)
{
	getImpl().type = type;
	return;
}


bool sig_chain::FrameImage::isPreview(void) const
{
	return (getImpl().type == FrameImage::PREVIEW);
}


bool sig_chain::FrameImage::isEnface(void) const
{
	return (getImpl().type == FrameImage::ENFACE);
}


bool sig_chain::FrameImage::isMeasure(void) const
{
	return (getImpl().type == FrameImage::MEASURE);
}


FrameImage::FrameImageImpl & sig_chain::FrameImage::getImpl(void) const
{
	return *d_ptr;
}



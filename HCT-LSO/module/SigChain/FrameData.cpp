#include "pch.h"
#include "FrameData.h"

#include <memory>
#include <atomic>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>

#include <boost/format.hpp>

using namespace sig_chain;


struct FrameData::FrameDataImpl
{
	unique_ptr<unsigned short[]> buffer;
	unique_ptr<float[]> sample;

	unsigned short* rawBuffer;

	int frameWidth;
	int frameHeight;
	int lineCount;

	int dataWidth;
	int dataHeight;
	int indexOfImage;
	int numLines;

	atomic<int> status;
	atomic<int> type;

	FrameDataImpl() : lineCount(0), indexOfImage(0), status(EMPTY), type(PREVIEW), 
					frameWidth(0), frameHeight(0), dataWidth(0), dataHeight(0), 
					numLines(0),
					rawBuffer(nullptr)
	{
		// Initialize buffer with data size of the largest bscan image.
		// buffer = make_unique<unsigned short[]>(FRAME_DATA_BUFFER_SIZE);
		// sample = make_unique<float[]>(FRAME_DATA_BUFFER_SIZE);
	}
};


FrameData::FrameData() :
	d_ptr(make_unique<FrameDataImpl>())
{
	// setFrameLayout(FRAME_DATA_LINE_SIZE, FRAME_DATA_LINES_MAX);
}


sig_chain::FrameData::FrameData(int width, int height, bool alloc) :
	d_ptr(make_unique<FrameDataImpl>())
{
	setFrameLayout(width, height, alloc);
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
sig_chain::FrameData::~FrameData() = default;
sig_chain::FrameData::FrameData(FrameData && rhs) = default;
FrameData & sig_chain::FrameData::operator=(FrameData && rhs) = default;


unsigned short * sig_chain::FrameData::getBuffer(void) const
{
	// return &getImpl().buffer[0];
	return getImpl().rawBuffer;
}


unsigned short * sig_chain::FrameData::getDataEnd(void) const
{
	return (getBuffer() + getLineCount() * getDataWidth()) ;
}


float * sig_chain::FrameData::getSample(void) const
{
	if (getImpl().sample == nullptr) {
		int bufferSize = getFrameSize();
		getImpl().sample = make_unique<float[]>(bufferSize);
	}
	return &getImpl().sample[0];
}


void sig_chain::FrameData::setBuffer(unsigned short * buff) 
{
	getImpl().buffer.reset(nullptr);
	getImpl().rawBuffer = buff;
	return;
}


int sig_chain::FrameData::getFrameWidth(void) const
{
	return getImpl().frameWidth;
}


int sig_chain::FrameData::getFrameHeight(void) const
{
	return getImpl().frameHeight;
}


int sig_chain::FrameData::getFrameSize(void) const
{
	return getFrameWidth() * getFrameHeight();
}


int sig_chain::FrameData::getDataWidth(void) const
{
	return getImpl().dataWidth;
}


int sig_chain::FrameData::getDataHeight(void) const
{
	return getImpl().dataHeight;
}


int sig_chain::FrameData::getDataSize(void) const
{
	return getDataWidth() * getDataHeight();
}


int sig_chain::FrameData::getDataCount(void) const
{
	return getLineCount() * getDataWidth();
}


int sig_chain::FrameData::getLineCount(void) const
{
	return getImpl().lineCount;
}


bool sig_chain::FrameData::isProcessed(void) const
{
	return (getImpl().status == FrameData::PROCESSED);
}


bool sig_chain::FrameData::isCompleted(void) const
{
	return (getImpl().status == FrameData::COMPLETED);
}


bool sig_chain::FrameData::isReceiving(void) const
{
	return (getImpl().status == FrameData::RECEIVING);
}


bool sig_chain::FrameData::isProcessing(void) const
{
	return (getImpl().status == FrameData::PROCESSING);
}


void sig_chain::FrameData::setType(int type)
{
	getImpl().type = type;
	return;
}


bool sig_chain::FrameData::isPreview(void) const
{
	return (getImpl().type == FrameData::PREVIEW);
}


bool sig_chain::FrameData::isEnface(void) const
{
	return (getImpl().type == FrameData::ENFACE);
}


bool sig_chain::FrameData::isMeasure(void) const
{
	return (getImpl().type == FrameData::MEASURE);
}


bool sig_chain::FrameData::isEmpty(void) const
{
	return (getImpl().status == FrameData::EMPTY);
}


void sig_chain::FrameData::setEmpty(void)
{
	getImpl().lineCount = 0;
	setStatus(FrameData::EMPTY);
	return;
}


bool sig_chain::FrameData::isReceivedFull(void)
{
	return (getLineCount() == getDataHeight());
}


void sig_chain::FrameData::setStatus(int status)
{
	getImpl().status = status;
	return;
}


void sig_chain::FrameData::setIndexOfImage(int index)
{
	getImpl().indexOfImage = index;
	return;
}


int sig_chain::FrameData::getIndexOfImage(void)
{
	return getImpl().indexOfImage;
}


void sig_chain::FrameData::setNumberOfLines(int lines)
{
	getImpl().numLines = lines;
	return;
}


int sig_chain::FrameData::getNumberOfLines(void)
{
	return getImpl().numLines;
}


void sig_chain::FrameData::dumpToFile(int repeats)
{
	wstring exportPath = _T(".//export");
	if (CreateDirectory(exportPath.c_str(), NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError()) {
		wstring cameraPath = _T(".//export//camera");
		if (CreateDirectory(cameraPath.c_str(), NULL) ||
			ERROR_ALREADY_EXISTS == GetLastError()) {
		}
	}

	int major = getIndexOfImage() / repeats;
	int minor = getIndexOfImage() % repeats;
	std::string path = (boost::format(".//export//camera//%03d_%02d.bin") % major % minor).str();

	std::ofstream file(path, std::ios::out | std::ofstream::binary);
	file.write((char *)getBuffer(), sizeof(unsigned short)*getDataSize());
	file.close();
	return;
}


void sig_chain::FrameData::setFrameLayout(int lineSize, int frameLines, bool alloc)
{
	getImpl().frameWidth = lineSize;
	getImpl().frameHeight = frameLines;

	if (alloc) {
		// Initialize buffer with data size of the largest bscan image.
		int bufferSize = lineSize * frameLines;
		getImpl().buffer = make_unique<unsigned short[]>(bufferSize);
		getImpl().rawBuffer = getImpl().buffer.get();
		// getImpl().sample = make_unique<float[]>(bufferSize);
	}
	else {
		getImpl().buffer.reset(nullptr);
		getImpl().rawBuffer = nullptr;
	}
	return;
}


void sig_chain::FrameData::setDataLayout(int width, int height)
{
	getImpl().dataWidth = width;
	getImpl().dataHeight = height;
	return;
}


int sig_chain::FrameData::addLineData(unsigned short * data, int numLines, bool frameStart)
{
	if (data == nullptr) {
		return 0;
	}

	if (frameStart) {
		setEmpty();
	}

	int left = getDataHeight() - getLineCount();
	int read = (left < numLines ? left : numLines);

	if (read > 0) {
		memcpy((uint8_t*)getDataEnd(), (uint8_t*)data, sizeof(unsigned short) * read * getDataWidth());
		getImpl().lineCount += read;
	}
	return read;
}


FrameData::FrameDataImpl & sig_chain::FrameData::getImpl(void) const
{
	return *d_ptr;
}

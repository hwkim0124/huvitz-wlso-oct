#include "pch.h"
#include "OctProcess.h"
#include "FrameBuffer.h"
#include "ChainSetup.h"
#include "ChainOutput.h"

#include <mutex>
#include <atomic>
#include <functional>

using namespace sig_chain;


struct OctProcess::OctProcessImpl
{
	FrameBuffer frameBuffer;

	OctProcessImpl() {
	}
};


OctProcess::OctProcess() :
	d_ptr(make_unique<OctProcessImpl>())
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
sig_chain::OctProcess::~OctProcess() = default;
sig_chain::OctProcess::OctProcess(OctProcess && rhs) = default;
OctProcess & sig_chain::OctProcess::operator=(OctProcess && rhs) = default;


OctProcess::OctProcessImpl & sig_chain::OctProcess::getImpl(void) const
{
	return *d_ptr;
}


bool sig_chain::OctProcess::initializeOctProcess(void)
{
	if (!getImpl().frameBuffer.initializeFrameBuffer()) {
		LogE() << "OctProcess not initialized!";
		return false;
	}

	LogD() << "OctProcess initialized.";
	return true;
}


bool sig_chain::OctProcess::startProcess(bool enface)
{
	ChainOutput::clearResultOfLastImage();

	bool result = getImpl().frameBuffer.startFrameBuffer(enface);
	LogD() << "OctProcess frame buffer started.";	
	return result;
}


void sig_chain::OctProcess::closeProcess(void)
{
	getImpl().frameBuffer.closeFrameBuffer();
	LogD() << "OctProcess frame buffer closed.";
	return ;
}


void sig_chain::OctProcess::releaseOctProcess(void)
{
	closeProcess();

	getImpl().frameBuffer.releaseFrameBuffer();
	LogD() << "OctProcess released.";
	return;
}


bool sig_chain::OctProcess::isBusying(void)
{
	bool result = getImpl().frameBuffer.isBusying();
	return result;
}


bool sig_chain::OctProcess::setPreviewFeature(int numImages, int numLines, int lineSize)
{
	bool result = getImpl().frameBuffer.setDataLayoutForPreview(lineSize, numLines, numImages);
	return result;
}

bool sig_chain::OctProcess::setMeasureFeature(int numImages, int numLines, int lineSize)
{
	bool result = getImpl().frameBuffer.setDataLayoutForMeasure(lineSize, numLines, numImages);
	return result;
}


bool sig_chain::OctProcess::setEnfaceFeature(int numImages, int numLines, int lineSize)
{
	bool result = getImpl().frameBuffer.setDataLayoutForEnface(lineSize, numLines, numImages);
	return result;
}


void sig_chain::OctProcess::receiveDataForPreview(std::uint16_t * data, int numLines, int idxOfImage)
{
	getImpl().frameBuffer.insertFrameDataForPreview(data, numLines, idxOfImage);
	return;
}


void sig_chain::OctProcess::receiveDataForMeasure(std::uint16_t * data, int numLines, int idxOfImage)
{
	getImpl().frameBuffer.insertFrameDataForMeasure(data, numLines, idxOfImage);
	return;
}


void sig_chain::OctProcess::receiveDataForEnface(std::uint16_t * data, int numLines, int idxOfImage)
{
	getImpl().frameBuffer.insertFrameDataForEnface(data, numLines, idxOfImage);
	return;
}


void sig_chain::OctProcess::notifyMeasureDataReceived(void)
{
	getImpl().frameBuffer.notifyFrameDataCompleted(false);
	return;
}


bool sig_chain::OctProcess::removeInitialSaturatedLines(std::uint16_t * data, int numLines)
{
	int satured = 3;
	if (numLines < satured) {
		return false;
	}

	// The line camera intensities at the positions starting a scan line are not stable, fully satured.  
	// It appears as the vertical stripes at the start of lines on the result image.  
	for (int i = 0; i < satured; i++) {
		unsigned short* dst = data + i * LINE_CAMERA_CCD_PIXELS;
		unsigned short* src = data + satured * LINE_CAMERA_CCD_PIXELS;
		memcpy((void*)dst, (void*)src, sizeof(unsigned short)*LINE_CAMERA_CCD_PIXELS);
	}

	return true;
}


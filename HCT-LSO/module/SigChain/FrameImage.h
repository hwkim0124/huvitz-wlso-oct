#pragma once

#include "SigChain2.h"



namespace sig_chain
{
	class SIGCHAIN_DLL_API FrameImage
	{
	public:
		FrameImage();
		virtual ~FrameImage();

		FrameImage(FrameImage&& rhs);
		FrameImage& operator=(FrameImage&& rhs);

		// Prevent copy construction and assignment. 
		FrameImage(const FrameImage& rhs) = delete;
		FrameImage& operator=(const FrameImage& rhs) = delete;

		enum {
			EMPTY = 0, RECEIVING, COMPLETED, PROCESSING, PROCESSED 
		};

		enum {
			PREVIEW = 0, ENFACE, MEASURE
		};

	public:
		unsigned char* getBuffer(void) const;
		uint8_t* getLateralBuffer(void) const;
		float* getIntensity(void) const;

		// Sets how many pixels the image buffer and how many samples the intensity buffer
		// hold, reallocating them when that changes. Not to be called while the image is in
		// use by the processing or callback threads; the contents are not preserved.
		void setCapacity(size_t imageSize, size_t intensitySize);

		void setDimension(int width, int height);
		int getWidth(void) const;
		int getHeight(void) const;
		int getIndexOfImage(void);
		void setIndexOfImage(int index);

		float getQualityIndex(void) const;
		float getSignalRatio(void) const;
		int getReferencePoint(void) const;

		void setQualityIndex(float value);
		void setSignalRatio(float value);
		void setReferencePoint(int value);

		void setStatus(int status);
		void setCompleted(void);
		void setEmpty(void);
		void setType(int type);

		bool isEmpty(void) const;
		bool isReceiving(void) const;
		bool isCompleted(void) const;
		bool isProcessing(void) const;
		bool isProcessed(void) const;

		bool isPreview(void) const;
		bool isEnface(void) const;
		bool isMeasure(void) const;

	private:
		struct FrameImageImpl;
		std::unique_ptr<FrameImageImpl> d_ptr;
		FrameImageImpl& getImpl(void) const;
	};

	typedef std::vector<FrameImage> FrameImageVect;
}

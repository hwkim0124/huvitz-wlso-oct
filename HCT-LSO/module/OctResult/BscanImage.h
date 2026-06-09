#pragma once

#include "OctResult2.h"

#include <memory>
#include <vector>
#include <string>


namespace cpp_util {
	class CvImage;
}


namespace oct_result
{
	class OCTRESULT_DLL_API BscanImage
	{
	public:
		BscanImage();
		BscanImage(const OctScanImageDescript& desc);
		BscanImage(std::uint8_t* data, int width, int height, float quality = 0.0f, float sigRatio = 0.0f, int refPoint = 0);
		virtual ~BscanImage();

		BscanImage(BscanImage&& rhs);
		BscanImage& operator=(BscanImage&& rhs);
		BscanImage(const BscanImage& rhs);
		BscanImage& operator=(const BscanImage& rhs);

	public:
		void setData(const std::uint8_t* data, int width, int height, float quality = 0.0f, float sigRatio = 0.0f, int refPoint = 0);
		void setData(const OctScanImageDescript& desc);
		bool setImage(cpp_util::CvImage* image);
		
		std::uint8_t* getBuffer(void) const;
		const OctScanImageDescript& getDescript(void) const;
		cpp_util::CvImage& getImage(void) const;
		std::wstring getFileName(bool path = false) const;

		bool isEmpty(void) const;
		int getWidth(void) const;
		int getHeight(void) const;

		float getQualityIndex(void) const;
		float getSignalRatio(void) const;
		int getReferencePoint(void) const;

		void setQualityIndex(float value);
		void setSignalRatio(float value);
		void setReferencePoint(int value);

		bool updateFile(void);
		bool exportFile(const std::wstring& path);
		bool importFile(const std::wstring& path);
		void flipVert(void);

	private:
		struct BscanImageImpl;
		std::unique_ptr<BscanImageImpl> d_ptr;
		BscanImageImpl& getImpl(void) const;
	};

	typedef std::vector<BscanImage> BscanImageList;
}


#pragma once

#include "OctResult2.h"

#include <memory>
#include <vector>


namespace oct_result
{
	class OCTRESULT_DLL_API EnfaceImage
	{
	public:
		EnfaceImage();
		EnfaceImage(const OctEnfaceImageDescript& desc);
		virtual ~EnfaceImage();

		EnfaceImage(EnfaceImage&& rhs);
		EnfaceImage& operator=(EnfaceImage&& rhs);
		EnfaceImage(const EnfaceImage& rhs);
		EnfaceImage& operator=(const EnfaceImage& rhs);

	public:
		void setData(const OctEnfaceImageDescript& desc);
		const OctEnfaceImageDescript& getDescript(void) const;

		std::uint8_t* getBuffer(void) const;
		bool isEmpty(void) const;
		int getWidth(void) const;
		int getHeight(void) const;

		bool exportFile(const std::wstring& dirPath, const std::wstring& imageName = L"enface", 
						int width = ENFACE_IMAGE_WIDTH, int height = ENFACE_IMAGE_HEIGHT);
		bool importFile(const std::wstring& dirPath, const std::wstring& imageName = L"enface");

		bool importFileWithExt(const std::wstring& dirPath, const std::wstring& imageName, const std::wstring& imageExt);

		bool processImage(void);

	private:
		struct EnfaceImageImpl;
		std::unique_ptr<EnfaceImageImpl> d_ptr;
		EnfaceImageImpl& getImpl(void) const;
	};

	typedef std::vector<EnfaceImage> EnfaceImageList;
}


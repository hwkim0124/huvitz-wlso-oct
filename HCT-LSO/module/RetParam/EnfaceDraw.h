#pragma once

#include "RetParam.h"

#include <memory>
#include <vector>


namespace ret_segm {
	class SegmImage;
	class SegmLayer;
}


namespace cpp_util {
	class CvImage;
}


namespace ret_param 
{
	class RETPARAM_DLL_API EnfaceDraw
	{
	public:
		EnfaceDraw();
		virtual ~EnfaceDraw();

		EnfaceDraw(EnfaceDraw&& rhs);
		EnfaceDraw& operator=(EnfaceDraw&& rhs);
		EnfaceDraw(const EnfaceDraw& rhs);
		EnfaceDraw& operator=(const EnfaceDraw& rhs);

	private:
		struct EnfaceDrawImpl;
		std::unique_ptr<EnfaceDrawImpl> d_ptr;

	protected:
		bool checkIfValidAllBscans(void) ;
		bool checkIfValidAllLayers(void) ;

		virtual void buildPlate(void);
		virtual void copyToImage(cpp_util::CvImage* image, int width, int height);

		void equalizeHistogram(cpp_util::CvImage* image);

		int getPlateWidth(void) const;
		int getPlateHeight(void) const;
		int getBscanWidth(void) const;
		int getBscanHeight(void) const;
		float getScanWidth(void) const;
		float getScanHeight(void) const;
		float getHorzPixelPerMM(void) const;
		float getVertPixelPerMM(void) const;

		ret_segm::SegmLayer* getUpperLayer(int index) const;
		ret_segm::SegmLayer* getLowerLayer(int index) const;
		cpp_util::CvImage* getPlate(void) const;

	public:
		bool initialize(const std::vector<ret_segm::SegmImage*> bscans, const std::vector<ret_segm::SegmLayer*> layers1,
						const std::vector<ret_segm::SegmLayer*> layers2, float scanWidth, float scanHeight);
		bool drawImage(cpp_util::CvImage* image, int width, int height);
	};

	typedef std::vector<EnfaceDraw> EnfaceDrawVect;

}


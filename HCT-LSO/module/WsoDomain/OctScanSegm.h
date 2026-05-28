#pragma once


namespace wso_domain
{
	const int OCT_SEGM_IMAGE_WIDTH_MAX = 2048;


	struct OctRetinaBsegmDescriptor
	{
		int ilms[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int nfls[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int ipls[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int opls[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int ioss[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int rpes[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int brms[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int oprs[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };

		int datums[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int inners[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int outers[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };

		int width = 0;
		int height = 0; 
		float range = 0.0f;

		int foveaX1 = 0;
		int foveaX2 = 0;
		int opticDiscX1 = 0;
		int opticDiscX2 = 0;
		int opticCupX1 = 0;
		int opticCupX2 = 0;
		int opticDiscPixelSize = 0;
		int opticCupPixelSize = 0;

		bool isFoveaValid = false;
		bool isOpticDiscValid = false;
		bool isOpticCupValid = false;
	};

	struct OctBsegmLayerPoints
	{
		int type;
		int width = 0;
		int height = 0;
		float range = 0.0f;

		int data[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
	};

	struct OctRetinaBsegmTraits
	{
		int width = 0;
		int height = 0;
		float range = 0.0f;

		int foveaX1 = 0;
		int foveaX2 = 0;
		int opticDiscX1 = 0;
		int opticDiscX2 = 0;
		int opticCupX1 = 0;
		int opticCupX2 = 0;
		int opticDiscPixelSize = 0;
		int opticCupPixelSize = 0;

		bool isFoveaValid = false;
		bool isOpticDiscValid = false;
		bool isOpticCupValid = false;
	};

	struct OctCorneaBsegmDescriptor
	{
		int epis[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int bows[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int ends[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };

		int datums[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int inners[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
		int outers[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };

		float curveRadius[OCT_SEGM_IMAGE_WIDTH_MAX] = { 0.0f };

		int width = 0;
		int height = 0;
		float range = 0.0f;

		int corneaTopX = 0;
		int corneaTopY = 0;
	};

	struct OctCorneaBsegmLayer
	{
		int type;
		int width = 0;
		int height = 0;
		float range = 0.0f;

		int data[OCT_SEGM_IMAGE_WIDTH_MAX] = { -1 };
	};

	struct OctCorneaBsegmTraits
	{
		int width = 0;
		int height = 0;
		float range = 0.0f;

		int corneaTopX = 0;
		int corneaTopY = 0;

		float curveRadius[OCT_SEGM_IMAGE_WIDTH_MAX] = { 0.0f };
	};
	
}
#pragma once

#include "RetParam.h"
#include "EnfaceData.h"

#include <memory>
#include <vector>


namespace cpp_util {
	class CvImage;
}

namespace ret_param
{
	class RETPARAM_DLL_API EnfaceTmap : public EnfaceData
	{
	public:
		EnfaceTmap();
		virtual ~EnfaceTmap();

		EnfaceTmap(EnfaceTmap&& rhs);
		EnfaceTmap& operator=(EnfaceTmap&& rhs);
		EnfaceTmap(const EnfaceTmap& rhs);
		EnfaceTmap& operator=(const EnfaceTmap& rhs);

	public:
		bool copyToImage(cpp_util::CvImage* image, int width, int height);

	private:
		struct EnfaceTmapImpl;
		std::unique_ptr<EnfaceTmapImpl> d_ptr;
	};
}


#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		bool WSOSYSTEM_DLL_API _stdcall isOctProtocolResultValid(void);
		void WSOSYSTEM_DLL_API _stdcall clearOctProtocolResult(void);

		int WSOSYSTEM_DLL_API _stdcall getOctResultPatternSectionCount(void);
		int WSOSYSTEM_DLL_API _stdcall getOctResultPreviewSectionCount(void);
		bool WSOSYSTEM_DLL_API _stdcall getOctResultPatternImage(int section_idx, int image_idx, OctScanImageDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctResultPreviewImage(int section_idx, int image_idx, OctScanImageDescript* desc);

		bool WSOSYSTEM_DLL_API _stdcall getOctResultEnfaceImage(OctEnfaceImageDescript* desc);
		bool WSOSYSTEM_DLL_API _stdcall getOctResultCorneaImage(OctCorneaImageDescript* desc);
	}
}

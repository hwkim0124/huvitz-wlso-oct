#pragma once

#include "CppUtil2.h"

extern "C"
{
	struct LsoAdjustParam
	{
		int brightness;		// -100 ~ 100
		int contrast;		// -100 ~ 100
		float gamma;		// 0.1 ~ 5.0
		int sharpness;		// -100 ~ 100
		int saturation;		// -100 ~ 100
		int channelView;	// [0 : all, 1 : R, 2 : G, 3 : B]
	};
	static_assert(sizeof(LsoAdjustParam) == 24, "LsoAdjustParam size must match its C# mirror");

	// Result Code
	enum LsoAdjustResult : int {
		LSOADJ_OK = 0,
		LSOADJ_PARAM = 2,
		LSOADJ_FAULT = 9
	};

	int CPPUTIL_DLL_API __stdcall LSOADJ_AdjustFile(const wchar_t* path, int channelOrder, const LsoAdjustParam* param, unsigned char** outData, int* outLen);

	void CPPUTIL_DLL_API __stdcall LSOADJ_FreeBuffer(unsigned char* data);

	// calling thread's last error
	int CPPUTIL_DLL_API __stdcall LSOADJ_GetLastError(char* buf, int len); 
}
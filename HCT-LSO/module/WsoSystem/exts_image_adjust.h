#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// Applies the display adjust pipeline (tone -> saturation -> sharpness -> channel
		// view) from src to dst. Both buffers are caller-allocated with identical geometry
		// and dst may alias src for in-place use. Holds no state, so the Toolkit and the
		// Agent can call it concurrently on their own buffers.
		// channels: 1 or 3. depthBytes: 1 (8-bit) or 2 (16-bit).
		// channelOrder: 0 = BGR, 1 = RGB. A 3-channel buffer's byte order differs between
		// consumers - the Toolkit review cache is RGB, the Agent capture path is BGR - and
		// it decides both which plane a channel view extracts and the luma weights.
		// Returns ADJ_OK, ADJ_PARAM on a rejected argument, or ADJ_FAULT.
		int WSOSYSTEM_DLL_API __stdcall adjustLsoImage(const unsigned char* src, unsigned char* dst,
			int width, int height, int srcStride, int dstStride,
			int channels, int depthBytes, int channelOrder, const LsoImageAdjust* param);

		// True when every field is neutral, so the caller can skip adjustLsoImage entirely.
		bool WSOSYSTEM_DLL_API __stdcall isLsoImageAdjustNeutral(const LsoImageAdjust* param);

		// Message from the calling thread's own last failed call. Thread-local, unlike the
		// capture equivalent, because two apps can be adjusting at once.
		// buf == nullptr or len <= 0 returns the required length without writing.
		int WSOSYSTEM_DLL_API __stdcall getLastImageAdjustError(char* buf, int len);
	}
}

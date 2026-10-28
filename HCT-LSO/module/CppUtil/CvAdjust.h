#pragma once

#include "CppUtil2.h"

namespace cpp_util
{
	// Byte order of a 3-channel buffer. The Toolkit review cache is RGB (BayerRG2RGB) while
	// the Agent capture path is BGR (BayerRG2BGR), so the caller has to say which it holds.
	enum class CvChannelOrder { BGR = 0, RGB = 1 };

	// Which channel the result is displayed through. A single channel is replicated into all
	// three, so the output geometry never differs from the input.
	enum class CvChannelView { ALL = 0, RED = 1, GREEN = 2, BLUE = 3 };

	// Normalized, depth-independent adjust parameters. Mirrors wso_domain::LsoImageAdjust,
	// which cannot be included here - CppUtil sits below the domain layer and its include
	// path carries no module directories.
	struct CvAdjustParam
	{
		int   brightness = 0;	// -100..100, additive offset in % of full scale
		int   contrast = 0;		// -100..100, gain about mid-scale
		float gamma = 1.0f;		// 0.1..5.0, 1.0 = neutral
		int   sharpness = 0;	// -100..100, >0 unsharp mask, <0 smoothing
		int   saturation = 0;	// -100..100, -100 = grayscale
		int   channelView = 0;	// CvChannelView
	};

	// Applies the display adjust pipeline: tone (brightness/contrast/gamma fused into one
	// lookup) -> saturation -> sharpness -> channel view.
	// src/dst are caller-allocated with identical geometry; dst may alias src. Keeps no
	// state, so concurrent calls on separate buffers are safe.
	// channels: 1 or 3. depthBytes: 1 (8-bit) or 2 (16-bit).
	// Returns false only on a rejected argument - a neutral param set is a successful copy.
	bool CPPUTIL_DLL_API applyImageAdjust(const unsigned char* src, unsigned char* dst,
		int width, int height, int srcStride, int dstStride,
		int channels, int depthBytes, CvChannelOrder order, const CvAdjustParam& param);

	// True when every field is neutral, so the caller can skip the copy altogether.
	bool CPPUTIL_DLL_API isImageAdjustNeutral(const CvAdjustParam& param);
}

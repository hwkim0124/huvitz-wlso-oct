#pragma once

#include <cstdint>
#include <vector>


namespace eye_align
{
	// Separable antialiased bilinear (triangle-filter) resample of an 8-bit grayscale
	// image, fused with the (pixel / 255 - mean) / std normalisation of the network input.
	//
	// Reproduces the resampling used when the pupil model was trained (torchvision
	// resize(antialias=true), which follows PIL's BILINEAR filter): for each output pixel
	// the input pixels within one output-pixel width of its centre are weighted by a
	// triangle. A plain bilinear or a box/area filter gives visibly different pixels at
	// a 2.5x downscale and shifts the predicted centre by up to several source pixels.
	//
	// The kernels depend only on the sizes; configure() once per source size and reuse.
	// apply() is not thread-safe (shared row buffer): the caller serialises.
	class GrayResampler
	{
	public:
		GrayResampler() = default;

		void configure(int inWidth, int inHeight, int outWidth, int outHeight);
		bool isConfigured(void) const;
		bool matches(int inWidth, int inHeight, int outWidth, int outHeight) const;

		int getInputWidth(void) const { return inWidth_; }
		int getInputHeight(void) const { return inHeight_; }
		int getOutputWidth(void) const { return outWidth_; }
		int getOutputHeight(void) const { return outHeight_; }

		// gray: row-major 8-bit pixels, stride bytes per row (>= inWidth).
		// dst: outWidth * outHeight floats = (v / 255 - mean) / stdDev, row-major.
		// Returns false when not configured or the arguments are invalid.
		bool apply(const std::uint8_t* gray, int stride, float* dst, float mean, float stdDev) const;

	private:
		// One-dimensional PIL/torchvision triangle-filter coefficients: for output sample i
		// the taps start at input index starts[i] and weights[offsets[i] .. offsets[i+1]).
		struct Kernel
		{
			std::vector<int> starts;
			std::vector<int> offsets;		// outSize + 1 entries
			std::vector<float> weights;
		};
		static Kernel buildKernel(int inSize, int outSize);

	private:
		int inWidth_ = 0, inHeight_ = 0, outWidth_ = 0, outHeight_ = 0;
		Kernel x_;
		Kernel y_;
		mutable std::vector<float> rowBuffer_;		// inHeight * outWidth, horizontal pass
	};
}

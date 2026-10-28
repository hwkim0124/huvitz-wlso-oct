#pragma once

namespace wso_device::detail
{
	// CameraDefs.h ordinal, not the PFNC wire identifier.
	enum class ColorPixelFormat : int { BayerRG12Packed = 13 };
	static_assert(static_cast<int>(ColorPixelFormat::BayerRG12Packed)
		== Spinnaker::PixelFormat_BayerRG12Packed);

	// Callback buffers are tightly packed pixels, never SDK row padding or packed bytes.
	// BayerRG12Packed remains tagged as such, but carries unpacked little-endian uint16.
	inline void copyCameraFrame(unsigned char* destination, const Spinnaker::ImagePtr& image,
		size_t byteCount)
	{
		if (image->GetPixelFormat() != Spinnaker::PixelFormat_BayerRG12Packed) {
			std::memcpy(destination, image->GetData(), byteCount);
			return;
		}
		const auto* source = static_cast<const unsigned char*>(image->GetData());
		const size_t width = image->GetWidth();
		const size_t height = image->GetHeight();
		const size_t stride = image->GetStride();
		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				const size_t b = y * stride + (x / 2) * 3;
				const unsigned short value = x % 2 == 0
					? (source[b] << 4) | (source[b + 1] >> 4)
					: (source[b + 2] << 4) | (source[b + 1] & 15);
				const size_t p = (y * width + x) * 2;
				destination[p] = static_cast<unsigned char>(value);
				destination[p + 1] = static_cast<unsigned char>(value >> 8);
			}
		}
	}
}

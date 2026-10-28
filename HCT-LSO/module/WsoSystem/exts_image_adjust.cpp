#include "pch.h"
#include "exts_image_adjust.h"

#include <cstring>
#include <exception>
#include <string>

namespace
{
	// Thread-local rather than file-static plus a mutex: the adjust call is stateless and
	// two apps can run it at once, so a shared string would let each overwrite the other's
	// diagnosis before it was read.
	thread_local std::string s_lastAdjustError;

	void setLastAdjustError(const char* message)
	{
		s_lastAdjustError = (message != nullptr) ? message : "";
	}

	cpp_util::CvAdjustParam toCvAdjustParam(const wso_domain::LsoImageAdjust& param)
	{
		cpp_util::CvAdjustParam converted;
		converted.brightness = param.brightness;
		converted.contrast = param.contrast;
		converted.gamma = param.gamma;
		converted.sharpness = param.sharpness;
		converted.saturation = param.saturation;
		converted.channelView = param.channelView;
		return converted;
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::adjustLsoImage(const unsigned char* src, unsigned char* dst,
	int width, int height, int srcStride, int dstStride,
	int channels, int depthBytes, int channelOrder, const LsoImageAdjust* param)
{
	try {
		if (param == nullptr) {
			setLastAdjustError("param is null");
			return ADJ_PARAM;
		}
		if (channelOrder != 0 && channelOrder != 1) {
			setLastAdjustError("channelOrder must be 0 (BGR) or 1 (RGB)");
			return ADJ_PARAM;
		}

		const auto order = (channelOrder == 1)
			? cpp_util::CvChannelOrder::RGB
			: cpp_util::CvChannelOrder::BGR;

		if (!cpp_util::applyImageAdjust(src, dst, width, height, srcStride, dstStride,
			channels, depthBytes, order, toCvAdjustParam(*param))) {
			setLastAdjustError("rejected geometry: check the buffers, size, stride, channels and depth");
			return ADJ_PARAM;
		}

		setLastAdjustError("");
		return ADJ_OK;
	}
	catch (const std::exception& e) {
		setLastAdjustError(e.what());
		return ADJ_FAULT;
	}
	catch (...) {
		setLastAdjustError("unexpected exception in adjustLsoImage");
		return ADJ_FAULT;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isLsoImageAdjustNeutral(const LsoImageAdjust* param)
{
	if (param == nullptr) {
		return true;
	}

	try {
		return cpp_util::isImageAdjustNeutral(toCvAdjustParam(*param));
	}
	catch (...) {
		return false;
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getLastImageAdjustError(char* buf, int len)
{
	if (buf == nullptr || len <= 0) {
		return (int)s_lastAdjustError.size();
	}
	int n = (int)s_lastAdjustError.size();
	if (n > len - 1) {
		n = len - 1;
	}
	std::memcpy(buf, s_lastAdjustError.data(), n);
	buf[n] = '\0';
	return n;
}

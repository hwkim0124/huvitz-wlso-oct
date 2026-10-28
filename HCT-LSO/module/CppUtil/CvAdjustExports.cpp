#include "pch.h"
#include "CvAdjustExports.h"
#include "CvAdjust.h"

#include <opencv2/imgcodecs.hpp>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <list>
#include <mutex>

namespace {
	thread_local std::string s_lastError;

	int fail(int code, const char* message) {
		s_lastError = message;
		return code;
	}

	cpp_util::CvAdjustParam toCv(const LsoAdjustParam& p)
	{
		cpp_util::CvAdjustParam c;
		c.brightness = p.brightness;
		c.contrast = p.contrast;
		c.gamma = p.gamma;
		c.sharpness = p.sharpness;
		c.saturation = p.saturation;
		c.channelView = p.channelView;
		return c;
	}

	// 캐시
	constexpr size_t kMaxCachedImages = 4;
	struct CachedImage {
		std::wstring path;
		std::uintmax_t size;
		std::filesystem::file_time_type writeTime;
		cv::Mat image;
	};

	std::mutex s_cacheLock;
	std::list<CachedImage> s_cache;

	cv::Mat loadDecoded(const std::wstring& path)
	{
		namespace fs = std::filesystem;
		const fs::path file(path);
		const auto size = fs::file_size(file);
		const auto writeTime = fs::last_write_time(file);

		{
			std::lock_guard<std::mutex> lock(s_cacheLock);
			for (auto it = s_cache.begin(); it != s_cache.end(); ++it) {
				if (it->path == path && it->size == size && it->writeTime == writeTime) {
					s_cache.splice(s_cache.begin(), s_cache, it);
					return it->image;
				}
			}
		}

		// Decode outside the lock so one slow decode does not stall requests for other images.
		std::vector<unsigned char> bytes(static_cast<size_t>(size));
		std::ifstream in(file, std::ios::binary);
		in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		if (!in) {
			return cv::Mat();
		}
		cv::Mat image = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
		if (image.empty()) {
			return image;
		}

		std::lock_guard<std::mutex> lock(s_cacheLock);
		s_cache.remove_if([&](const CachedImage& c) { return c.path == path; });
		s_cache.push_front({ path, size, writeTime, image });
		if (s_cache.size() > kMaxCachedImages) {
			s_cache.pop_back();
		}
		return image;
	}
}

int CPPUTIL_DLL_API __stdcall LSOADJ_AdjustFile(const wchar_t* path, int channelOrder,
	const LsoAdjustParam* param, unsigned char** outData, int* outLen) {
	try {
		if (path == nullptr || param == nullptr || outData == nullptr || outLen == nullptr) {
			return fail(LSOADJ_PARAM, "null argument");
		}
		if (channelOrder != 0 && channelOrder != 1) {
			return fail(LSOADJ_PARAM, "channelOrder must be 0 (BGR) or 1 (RGB)");
		}
		*outData = nullptr;
		*outLen = 0;

		const cv::Mat source = loadDecoded(path);
		if (source.empty()) {
			return fail(LSOADJ_PARAM, "read or decode failed");
		}

		// cached source is shared
		cv::Mat adjusted(source.size(), source.type());
		const auto order = (channelOrder == 1) ? cpp_util::CvChannelOrder::RGB : cpp_util::CvChannelOrder::BGR;
		if (!cpp_util::applyImageAdjust(source.data, adjusted.data, source.cols, source.rows,
			static_cast<int>(source.step), static_cast<int>(adjusted.step), source.channels(),
			static_cast<int>(source.elemSize1()), order, toCv(*param))) {
			return fail(LSOADJ_PARAM, "rejected geometry (channels must be 1 or 3, depth 8/16 bit)");
		}

		const char* ext = (adjusted.depth() == CV_8U) ? ".bmp" : ".png";
		std::vector<unsigned char> encoded;
		if (!cv::imencode(ext, adjusted, encoded)) {
			return fail(LSOADJ_FAULT, "encode failed");
		}
		auto* buffer = static_cast<unsigned char*>(std::malloc(encoded.size()));
		if (buffer == nullptr) {
			return fail(LSOADJ_FAULT, "out of memory");
		}
		std::memcpy(buffer, encoded.data(), encoded.size());
		*outData = buffer;
		*outLen = static_cast<int>(encoded.size());
		s_lastError.clear();
		return LSOADJ_OK;
	}
	catch (const std::exception& e) {
		return fail(LSOADJ_FAULT, e.what());
	}
	catch (...) {
		return fail(LSOADJ_FAULT, "unexpected exception in LSOADJ_AdjustEncoded");
	}
}

void CPPUTIL_DLL_API __stdcall LSOADJ_FreeBuffer(unsigned char* data) {
	std::free(data);
}

int CPPUTIL_DLL_API __stdcall LSOADJ_GetLastError(char* buf, int len) {
	const int n = static_cast<int>(s_lastError.size());
	if (buf == nullptr || len <= 0) {
		return n;
	}
	const int copied = (n < len - 1) ? n : len - 1;
	std::memcpy(buf, s_lastError.data(), copied);
	buf[copied] = '\0';
	return copied;
}
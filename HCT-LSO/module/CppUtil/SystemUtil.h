#pragma once

#include "CppUtil2.h"

#include <vector>

namespace cpp_util
{
	class CPPUTIL_DLL_API SystemUtil
	{
	public:
		SystemUtil();
		virtual ~SystemUtil();

	public:
		static bool isDirectory(const std::wstring& dirName);
		static bool isFile(const std::wstring& fileName);

		static auto findFilesInDirectory(const std::wstring& directory, const std::wstring& filter = std::wstring(L"*.*"), bool includePath = true) -> std::vector<std::wstring>;
		static bool moveFile(const std::wstring& pathFrom, const std::wstring& pathTo);
	};
}
#include "pch.h"
#include "StrFormat.h"

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <ctime>
#include <iomanip>


std::string cpp_util::wtoa(const wchar_t* ptr, size_t len)
{
	if (len == -1) {
		len = wcslen(ptr);
	}

	if (len <= 0) {
		return std::string("");
	}

	std::string str(WideCharToMultiByte(CP_THREAD_ACP, 0, ptr, (int)len, nullptr, 0, 0, 0), '\0');

	if (str.size() == 0) {
		throw std::system_error(GetLastError(), std::system_category());
	}

	if (0 == WideCharToMultiByte(CP_THREAD_ACP, 0, ptr, (int)len, &str[0], (int)str.size(), 0, 0)) {
		throw std::system_error(GetLastError(), std::system_category(), "Error converting wide string to narrow");
	}

	return str;
}


std::string cpp_util::wtoa(const std::wstring& wstr)
{
	return wtoa(&wstr[0], wstr.size());
}


std::wstring cpp_util::atow(const std::string& cstr)
{
	int wstr_length = MultiByteToWideChar(CP_UTF8, 0, cstr.c_str(), -1, NULL, 0);
	if (wstr_length > 0) {
		// std::wstring wstr(wstr_length, L'\0');
		std::wstring wstr(wstr_length, 0);
		MultiByteToWideChar(CP_UTF8, 0, cstr.c_str(), -1, &wstr[0], wstr_length);
		wstr.erase(wstr.find(L'\0'));
		return wstr;
	}
	else {
		return std::wstring();
	}
	/*
	// Deprecated in C++17.
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(cstr);
	return wide;
	*/
}


std::string cpp_util::format_string(const char* format, ...)
{
	char buff[FORMAT_STR_BUFF_SIZE];
	va_list ap;
	va_start(ap, format);
	vsnprintf_s(buff, FORMAT_STR_BUFF_SIZE, _TRUNCATE, format, ap);
	va_end(ap);
	return std::string(buff);
}


std::wstring cpp_util::format_string(const wchar_t* format, ...)
{
	wchar_t buff[FORMAT_STR_BUFF_SIZE];
	va_list ap;
	va_start(ap, format);
	vswprintf_s(buff, FORMAT_STR_BUFF_SIZE, format, ap);
	va_end(ap);
	return std::wstring(buff);
}

std::string CPPUTIL_DLL_API cpp_util::datetime_string(bool joiner)
{
	auto now = std::time(nullptr);
	struct tm timeinfo;

	auto err = localtime_s(&timeinfo, &now);
	if (!err) {
		char buff[128];
		string form = joiner ? "%Y-%m-%d %H:%M:%S" : "%Y%m%d_%H%M%S";
		strftime(buff, sizeof(buff), form.c_str(), &timeinfo);
		auto tstr = std::string(buff);
		return tstr;
	}
	return std::string();
}


std::string CPPUTIL_DLL_API cpp_util::date_string(bool joiner)
{
	auto now = std::time(nullptr);
	struct tm timeinfo;

	auto err = localtime_s(&timeinfo, &now);
	if (!err) {
		char buff[128];
		string form = joiner ? "%Y-%m-%d" : "%Y%m%d";
		strftime(buff, sizeof(buff), form.c_str(), &timeinfo);
		auto tstr = std::string(buff);
		return tstr;
	}
	return std::string();
}


std::string CPPUTIL_DLL_API cpp_util::date_string(std::time_t time, bool joiner)
{
	struct tm timeinfo;

	auto err = localtime_s(&timeinfo, &time);
	if (!err) {
		char buff[128];
		string form = joiner ? "%Y-%m-%d" : "%Y%m%d";
		strftime(buff, sizeof(buff), form.c_str(), &timeinfo);
		auto tstr = std::string(buff);
		return tstr;
	}
	return std::string();
}

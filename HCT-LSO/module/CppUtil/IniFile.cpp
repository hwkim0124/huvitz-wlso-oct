#include "pch.h"
#include "IniFile.h"

using namespace cpp_util;
// IniFile.cpp

IniFile::IniFile(const std::wstring& filePath)
    : m_filePath(filePath)
{
    // 파일 존재 여부 확인
    DWORD attrs = ::GetFileAttributesW(m_filePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        // 파일이 없으면 CREATE_NEW로 빈 파일 생성
        HANDLE hFile = ::CreateFileW(
            m_filePath.c_str(),
            GENERIC_WRITE,
            0,              // 공유 없음
            nullptr,        // 기본 보안
            CREATE_NEW,     // 새로 생성, 이미 있으면 실패
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hFile != INVALID_HANDLE_VALUE) {
            ::CloseHandle(hFile);
        }
    }
}

std::wstring IniFile::ReadString(
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& defaultValue
) const
{
    wchar_t buffer[1024] = { 0 };
    DWORD charsRead = ::GetPrivateProfileStringW(
        section.c_str(),
        key.c_str(),
        defaultValue.c_str(),
        buffer,
        static_cast<DWORD>(std::size(buffer)),
        m_filePath.c_str()
    );
    return std::wstring(buffer, charsRead);
}

int IniFile::ReadInt(
    const std::wstring& section,
    const std::wstring& key,
    int defaultValue
) const
{
    // 매크로로 UNICODE 빌드 시 GetPrivateProfileIntW 호출
    return ::GetPrivateProfileIntW(
        section.c_str(),
        key.c_str(),
        defaultValue,
        m_filePath.c_str()
    );
}

int IniFile::ReadUInt(
    const std::wstring& section,
    const std::wstring& key,
    int defaultValue
) const
{
    // 매크로로 UNICODE 빌드 시 GetPrivateProfileIntW 호출
    return (unsigned int)::GetPrivateProfileIntW(
        section.c_str(),
        key.c_str(),
        defaultValue,
        m_filePath.c_str()
    );
}

float IniFile::ReadFloat(
    const std::wstring& section,
    const std::wstring& key,
    float defaultValue
) const
{
    std::wstring s = ReadString(section, key, L"");
    try {
        return std::stof(s);
    }
    catch (...) {
        return defaultValue;
    }
}

double IniFile::ReadDouble(
    const std::wstring& section,
    const std::wstring& key,
    double defaultValue
) const
{
    std::wstring s = ReadString(section, key, L"");
    try {
        return std::stod(s);
    }
    catch (...) {
        return defaultValue;
    }
}

bool IniFile::WriteString(
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& value
) const
{
    return ::WritePrivateProfileStringW(
        section.c_str(),
        key.c_str(),
        value.c_str(),
        m_filePath.c_str()
    ) != FALSE;
}

bool IniFile::WriteInt(
    const std::wstring& section,
    const std::wstring& key,
    int value
) const
{
    return WriteString(section, key, std::to_wstring(value));
}


bool IniFile::WriteFloat(
    const std::wstring& section,
    const std::wstring& key,
    float value
) const
{
    return WriteString(section, key, std::to_wstring(value));
}


bool IniFile::WriteDouble(
    const std::wstring& section,
    const std::wstring& key,
    double value
) const
{
    return WriteString(section, key, std::to_wstring(value));
}

bool IniFile::DeleteKey(
    const std::wstring& section,
    const std::wstring& key
) const
{
    // 값을 NULL로 하면 해당 키 삭제
    return ::WritePrivateProfileStringW(
        section.c_str(),
        key.c_str(),
        nullptr,
        m_filePath.c_str()
    ) != FALSE;
}

bool IniFile::DeleteSection(
    const std::wstring& section
) const
{
    // 키 이름도 NULL로 하면 섹션 전체 삭제
    return ::WritePrivateProfileStringW(
        section.c_str(),
        nullptr,
        nullptr,
        m_filePath.c_str()
    ) != FALSE;
}


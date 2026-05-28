// IniFile.h
#pragma once

#include "CppUtil2.h"

#include <string>
#include <windows.h>

/**
 * @brief UNICODE 기반 INI 파일 읽기/쓰기 유틸리티 클래스
 **/

namespace cpp_util
{
    class CPPUTIL_DLL_API IniFile {
    public:
        /**
         * @param filePath INI 파일 경로 (절대/상대 모두 가능, wstring)
         */
        explicit IniFile(const std::wstring& filePath);

        /**
         * @brief 문자열 읽기 (wstring)
         */
        std::wstring ReadString(
            const std::wstring& section,
            const std::wstring& key,
            const std::wstring& defaultValue = L""
        ) const;

        /**
         * @brief 정수형 값 읽기
         */
        int ReadInt(
            const std::wstring& section,
            const std::wstring& key,
            int defaultValue = 0
        ) const;

        /**
         * @brief Unsigned Int형 값 읽기
         */

        int ReadUInt(
            const std::wstring& section,
            const std::wstring& key,
            int defaultValue = 0
        ) const;

        /**
        * @brief Unsigned Int형 값 읽기
        */

        float ReadFloat(
            const std::wstring& section,
            const std::wstring& key,
            float defaultValue = 0.0
        ) const;

        /**
            * @brief Double형 값 읽기
            */
        double ReadDouble(
            const std::wstring& section,
            const std::wstring& key,
            double defaultValue = 0.0
        ) const;

        /**
         * @brief 문자열 쓰기
         */
        bool WriteString(
            const std::wstring& section,
            const std::wstring& key,
            const std::wstring& value
        ) const;

        /**
         * @brief 정수형 값 쓰기
         */
        bool WriteInt(
            const std::wstring& section,
            const std::wstring& key,
            int value
        ) const; 

        /**
        * @brief Float형 값 쓰기
        */
        bool WriteFloat(
            const std::wstring& section,
            const std::wstring& key,
            float value
        ) const;  // ← 추가

        /**
        * @brief Double형 값 쓰기
        */
        bool WriteDouble(
            const std::wstring& section,
            const std::wstring& key,
            double value
        ) const;  // ← 추가

        /**
         * @brief 키 삭제
         */
        bool DeleteKey(
            const std::wstring& section,
            const std::wstring& key
        ) const;

        /**
         * @brief 섹션 전체 삭제
         */
        bool DeleteSection(
            const std::wstring& section
        ) const;

    private:
        std::wstring m_filePath;  ///< INI 파일 경로(wstring)
    };
}






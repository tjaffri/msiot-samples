//
// Copyright (c) 2015, Microsoft Corporation
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//


#include "pch.h"
#include "CommonUtils.h"
#include <stdarg.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <locale>
#if defined(__ANDROID_API__)
#include <boost/locale.hpp>
#include <boost/program_options/detail/convert.hpp>
#include <libs/program_options/src/convert.cpp>
#include <libs/program_options/src/utf8_codecvt_facet.cpp>
#else
#include <codecvt>
#endif

// HACK: VS 2015 does not yet support streams from char16_t and char32_t
// therefore, for Win 10, we must substitue wchar_t for char16_t.  We cannot do
// for all platforms since a wchar_t in iOS/Android is 4 bytes.
// See: https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error?forum=vcgeneral
#if defined(BRIDGE_WIN10)
#define BRIDGE_HACK_CHAR16 wchar_t
#define BRIDGE_HACK_TOWSTRING ToWstring
#define BRIDGE_HACK_TOUTF16 ToUtf16
#else
#define BRIDGE_HACK_CHAR16 char16_t
#define BRIDGE_HACK_TOWSTRING
#define BRIDGE_HACK_TOUTF16
#endif 

namespace StringUtils
{
    std::wstring Format(const wchar_t* fmt, ...)
    {
        static size_t MaxStringFormatSize = 1024 * 10;
        size_t size = 1024;
        std::vector<wchar_t> str;

        va_list args;
        while (true && size <= MaxStringFormatSize)
        {
            str.resize(size);

            va_start(args, fmt);
#if defined(__ANDROID__) || defined(__APPLE__)
            int32_t n = vswprintf(&str[0], size, fmt, args);
#else
            int32_t n = _vsnwprintf_s(&str[0], size, _TRUNCATE, fmt, args);
#endif
            va_end(args);

            if (n > -1 && static_cast<size_t>(n) < size)
            {
                str.resize(n); // truncate
                return std::wstring(str.begin(), str.end());
            }

            // grow buffer
            if (n > -1)
                size = n + 1024;
            else
                size *= 2;
        }
        return L"";
    }

    std::string Format(const char* fmt, ...)
    {
        static size_t MaxStringFormatSize = 1024 * 10;
        size_t size = 1024;
        std::vector<char> str;

        va_list args;
        while (true && size <= MaxStringFormatSize)
        {
            str.resize(size);

            va_start(args, fmt);
#if defined(__ANDROID__) || defined(__APPLE__)
            int32_t n = vsnprintf(&str[0], size, fmt, args);
#else
            int32_t n = _vsnprintf_s(&str[0], size, _TRUNCATE, fmt, args);
#endif
            va_end(args);

            if (n > -1 && static_cast<size_t>(n) < size)
            {
                str.resize(n); // truncate
                return std::string(str.begin(), str.end());
            }

            // grow buffer
            if (n > -1)
                size = n + 1024;
            else
                size *= 2;
        }
        return "";
    }

#if defined(__ANDROID_API__)
    std::string ToUtf8(_In_ const std::u16string& input)
    {
        return boost::locale::conv::utf_to_utf<char>(input);
    }

    std::string ToUtf8(_In_ const std::u32string& input)
    {
        return boost::locale::conv::utf_to_utf<char>(input);
    }

    std::string ToUtf8(_In_ const std::wstring& input)
    {
        return boost::to_utf8(input);
    }

    std::u16string ToUtf16(_In_ const std::string& input)
    {
        return boost::locale::conv::utf_to_utf<char16_t>(input);
    }

    std::u16string ToUtf16(_In_ const std::u32string& input)
    {
        return boost::locale::conv::utf_to_utf<char16_t>(input);
    }

    std::u16string ToUtf16(_In_ const std::wstring& input)
    {
        return boost::locale::conv::utf_to_utf<char16_t>(input);
    }

    std::u32string ToUtf32(_In_ const std::string& input)
    {
        return boost::locale::conv::utf_to_utf<char32_t>(input);
    }

    std::u32string ToUtf32(_In_ const std::u16string& input)
    {
        return boost::locale::conv::utf_to_utf<char32_t>(input);
    }

    std::u32string ToUtf32(_In_ const std::wstring& input)
    {
        return boost::locale::conv::utf_to_utf<char32_t>(input);
    }

    std::wstring ToWstring(_In_ const std::string& input)
    {
        return boost::from_utf8(input);
    }

    std::wstring ToWstring(_In_ const std::u16string& input)
    {
        return boost::locale::conv::utf_to_utf<wchar_t>(input);
    }

    std::wstring ToWstring(_In_ const std::u32string& input)
    {
        return boost::locale::conv::utf_to_utf<wchar_t>(input);
    }
#else // defined(__ANDROID_API_)
    template <size_t CharSize>
    struct Wstring
    {};

    template <>
    struct Wstring<2>
    {
        static std::wstring FromUtf8(_In_ const std::string& input)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfConverter;
            return utfConverter.from_bytes(input);
        }

        static std::wstring FromUtf16(_In_ const std::u16string& input)
        {
            return std::wstring(std::begin(input), std::end(input));
        }

#if !defined(BRIDGE_WIN10)
        static std::wstring FromUtf32(_In_ const std::u32string& input)
        {
            return FromUtf8(StringUtils::ToUtf8(input));
        }
#endif

        static std::string ToUtf8(_In_ const std::wstring& input)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfConverter;
            return utfConverter.to_bytes(input);
        }

        static std::u16string ToUtf16(_In_ const std::wstring& input)
        {
            return std::u16string(std::begin(input), std::end(input));
        }

#if !defined(BRIDGE_WIN10)
        static std::u32string ToUtf32(_In_ const std::wstring& input)
        {
            return StringUtils::ToUtf32(ToUtf8(input));
        }
#endif
    };

    template <>
    struct Wstring<4>
    {
        static std::wstring FromUtf8(_In_ const std::string& input)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utfConverter;
            return utfConverter.from_bytes(input);
        }

        static std::wstring FromUtf16(_In_ const std::u16string& input)
        {
            return FromUtf8(StringUtils::ToUtf8(input));
        }

        static std::wstring FromUtf32(_In_ const std::u32string& input)
        {
            return std::wstring(std::begin(input), std::end(input));
        }

        static std::string ToUtf8(_In_ const std::wstring& input)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utfConverter;
            return utfConverter.to_bytes(input);
        }

        static std::u16string ToUtf16(_In_ const std::wstring& input)
        {
            return StringUtils::ToUtf16(ToUtf8(input));
        }

        static std::u32string ToUtf32(_In_ const std::wstring& input)
        {
            return std::u32string(std::begin(input), std::end(input));
        }
    };

    std::string ToUtf8(_In_ const std::u16string& input)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<BRIDGE_HACK_CHAR16>, BRIDGE_HACK_CHAR16> utfConverter;
        return utfConverter.to_bytes(BRIDGE_HACK_TOWSTRING(input));
    }

#if !defined(BRIDGE_WIN10)
    std::string ToUtf8(_In_ const std::u32string& input)
    {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utfConverter;
        return utfConverter.to_bytes(input);
    }
#endif

    std::string ToUtf8(_In_ const std::wstring& input)
    {
        return Wstring<sizeof(wchar_t)>::ToUtf8(input);
    }

    std::u16string ToUtf16(_In_ const std::string& input)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<BRIDGE_HACK_CHAR16>, BRIDGE_HACK_CHAR16> utfConverter;
        return BRIDGE_HACK_TOUTF16(utfConverter.from_bytes(input));
    }

#if !defined(BRIDGE_WIN10)
    std::u16string ToUtf16(_In_ const std::u32string& input)
    {
        std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> utfConverter;
        return ToUtf16(ToUtf8(input));
    }
#endif

    std::u16string ToUtf16(_In_ const std::wstring& input)
    {
        return Wstring<sizeof(wchar_t)>::ToUtf16(input);
    }

#if !defined(BRIDGE_WIN10)
    std::u32string ToUtf32(_In_ const std::string& input)
    {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utfConverter;
        return utfConverter.from_bytes(input);
    }

    std::u32string ToUtf32(_In_ const std::u16string& input)
    {
        return ToUtf32(ToUtf8(input));
    }

    std::u32string ToUtf32(_In_ const std::wstring& input)
    {
        return Wstring<sizeof(wchar_t)>::ToUtf32(input);
    }
#endif

    std::wstring ToWstring(_In_ const std::string& input)
    {
        return Wstring<sizeof(wchar_t)>::FromUtf8(input);
    }

    std::wstring ToWstring(_In_ const std::u16string& input)
    {
        return Wstring<sizeof(wchar_t)>::FromUtf16(input);
    }

#if !defined(BRIDGE_WIN10)
    std::wstring ToWstring(_In_ const std::u32string& input)
    {
        return Wstring<sizeof(wchar_t)>::FromUtf32(input);
    }
#endif

#endif // defined(__ANDROID_API_)

}
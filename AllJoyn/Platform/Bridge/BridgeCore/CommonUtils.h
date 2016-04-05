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

#pragma once

#include <string>
#include "CommonSal.h"

#define QS_SUCCEEDED(hr) (hr == ER_OK)
#define QS_FAILED(hr) (hr != ER_OK)
#define CHK_QS(__hr) { hr = (__hr); if (QS_FAILED(hr)) goto CleanUp; }

// TODO: These are Windows error codes, we should use QStatus or a custom error type instead
#if defined(__ANDROID_API__) || defined (__APPLE__)

#define ERROR_SUCCESS           0x000
#define ERROR_IO_PENDING        0x3E5
#define ERROR_BAD_ARGUMENTS     0x0A0
#define ERROR_NOT_ENOUGH_MEMORY 0x008
#define ERROR_BAD_FORMAT        0x00B
#define ERROR_ACCESS_DENIED     0x005

// TODO: Re-write FileAccess to be a true PAL and remove this HANDLE reference
#define HANDLE                  void*
#define INVALID_HANDLE_VALUE    nullptr
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define FILE_BEGIN              0
#define FILE_END                2

#define __FUNCTION__ __PRETTY_FUNCTION__

#endif

namespace StringUtils
{
    std::wstring Format(const wchar_t* fmt, ...);
    std::string Format(const char* fmt, ...);

    std::string ToUtf8(const std::u16string& input);
#if !defined(BRIDGE_WIN10)
    std::string ToUtf8(const std::u32string& input);
#endif
    std::string ToUtf8(const std::wstring& input);

    std::u16string ToUtf16(const std::string& input);
#if !defined(BRIDGE_WIN10)
    std::u16string ToUtf16(const std::u32string& input);
#endif
    std::u16string ToUtf16(const std::wstring& input);

#if !defined(BRIDGE_WIN10)
    std::u32string ToUtf32(const std::string& input);
    std::u32string ToUtf32(const std::u16string& input);
    std::u32string ToUtf32(const std::wstring& input);
#endif

    std::wstring ToWstring(const std::string& input);
    std::wstring ToWstring(const std::u16string& input);
    std::wstring ToWstring(const std::u32string& input);
}

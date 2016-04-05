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

#include <mutex>
#include "CommonUtils.h"

#define DebugPropertyInfo void
#define DebugStackFrameDescriptor void
#define DebugStackFrameDescriptor64 void

#define CHK_AJSTATUS(x) { status = (x); if ((ER_OK != status))  { goto leave; }}
#define CHK_POINTER(x) {if ((nullptr == x)) { status = ER_OUT_OF_MEMORY; goto leave; }}

#define IfFailError(v, e) \
    { \
        JsErrorCode error = (v); \
        if (error != JsNoError) \
        { \
            DebugLogInfo(L"chakrahost: fatal error 0x%08x: %s.\n", (int)error, (e)); \
            goto error; \
        } \
    }

#define IfFailLeave(v, e) \
    { \
        JsErrorCode error = (v); \
        if (error != JsNoError) \
        { \
            DebugLogInfo(L"chakrahost: fatal error 0x%08x: %s.\n", (int)error, (e)); \
            hr = E_FAIL; \
            goto leave; \
        } \
    }

#define IfFailRet(v) \
    { \
        JsErrorCode error = (v); \
        if (error != JsNoError) \
        { \
            return error; \
        } \
    }

#define IfFailThrow(v, e) \
    { \
        JsErrorCode error = (v); \
        if (error != JsNoError) \
        { \
            ThrowException((e)); \
        } \
    }

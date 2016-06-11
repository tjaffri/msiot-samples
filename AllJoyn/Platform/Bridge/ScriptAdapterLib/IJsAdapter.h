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

#include "IAdapter.h"

namespace AdapterLib {

class IJsAdapterError
{
public:
    virtual void ReportError(_In_ const std::string& msg) = 0;
};

class IJsAdapter : public Bridge::IAdapter
{
public:
    static std::shared_ptr<IJsAdapter> Get();

    virtual uint32_t AddDevice(
        _In_ std::shared_ptr<Bridge::IAdapterDevice> device,
        _In_ const std::string& baseTypeXml,
        _In_ const std::string& jsScript,
        _In_ const std::string& jsModulesPath,
        _Out_opt_ std::shared_ptr<Bridge::IAdapterAsyncRequest>& requestPtr) = 0;

    virtual void SetAdapterError(_In_ std::shared_ptr<IJsAdapterError>& adaptErr) = 0;
};

}
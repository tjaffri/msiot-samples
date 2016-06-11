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
#include "IAdapter.h"

using namespace Bridge;

namespace AdapterLib
{
    // Tracks the status of an async request to a JsAdapter, allowing callers to set a continuation handler
    // and/or wait for completion.
    class JsAdapterRequest : public IAdapterAsyncRequest
    {
    public:
        JsAdapterRequest();

        // When status is set to anything other than ERROR_IO_PENDING, the continuation (if set) is invoked
        // and any outstanding calls to Wait() are unblocked.
        void SetStatus(uint32_t status);

        // IAdapterAsyncRequest methods

        uint32_t Continue(std::function<uint32_t(uint32_t)> continuationHandler) override;
        uint32_t Status() override;
        uint32_t Wait(uint32_t timeoutMsec) override;
        uint32_t Cancel() override;
        uint32_t Release() override;

    private:
        uint32_t _status;
        std::mutex _mutex;
        std::condition_variable _condition;
        std::function<uint32_t(uint32_t)> _continuationHandler;
    };
}

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
#include "BridgeCommon.h"
#include "JsAdapterRequest.h"

using namespace Bridge;

namespace AdapterLib
{

    JsAdapterRequest::JsAdapterRequest() :
        _status(ERROR_IO_PENDING)
    {
    }

    void JsAdapterRequest::SetStatus(uint32_t status)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // If status is being changed from pending to not pending and a completion handler was set,
        // invoke it then use the updated status instead.
        if (_continuationHandler && _status == ERROR_IO_PENDING && status != _status)
        {
            status = _continuationHandler(status);
        }

        _status = status;
        _condition.notify_all();
    }

    uint32_t JsAdapterRequest::Continue(std::function<uint32_t(uint32_t)> continuationHandler)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_continuationHandler)
        {
            return ERROR_NOT_CAPABLE;
        }

        _continuationHandler = continuationHandler;

        // If the status is not pending and not aborted, invoke the continuation handler immediately.
        if (_continuationHandler && _status != ERROR_IO_PENDING && _status != ERROR_REQUEST_ABORTED)
        {
            _status = _continuationHandler(_status);
        }

        return ERROR_SUCCESS;
    }

    uint32_t JsAdapterRequest::Status()
    {
        return _status;
    }

    uint32_t JsAdapterRequest::Wait(uint32_t timeoutMsec)
    {
        std::unique_lock<std::mutex> lock(_mutex);

        // Wait until the status is anything other than ERROR_IO_PENDING (or the timeout elapses).
        bool completed = _condition.wait_for(
            lock, std::chrono::milliseconds(timeoutMsec), [&]() { return _status != ERROR_IO_PENDING; });

        // Note after receiving an ERROR_SUCCESS result from a wait, callers must still check Status()
        // to get the actual status of the overall request.
        return completed ? ERROR_SUCCESS : ERROR_TIMEOUT;
    }

    uint32_t JsAdapterRequest::Cancel()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_status == ERROR_IO_PENDING)
        {
            // This unblocks any threads stuck in Wait(). The actual request operation will not be aborted,
            // though any results from it will be ignored. The continuation (if set) is NOT invoked.
            _status = ERROR_REQUEST_ABORTED;
            _condition.notify_all();
            return ERROR_SUCCESS;
        }
        else
        {
            // The request is already completed (or aborted).
            return ERROR_NOT_CAPABLE;
        }
    }

    uint32_t JsAdapterRequest::Release()
    {
        this->Cancel();
        return ERROR_SUCCESS;
    }
}

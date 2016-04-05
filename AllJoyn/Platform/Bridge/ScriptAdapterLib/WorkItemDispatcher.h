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
//#pragma once
#include "AsyncQueue.h"

class WorkItemDispatcher
{
public:
    using WorkItemFunctorType = std::function<void()>;

    WorkItemDispatcher() { }

    bool Dispatch(const WorkItemFunctorType& workItemFunctor)
    {
        return workItemFunctor == nullptr || _asyncQueue.Push(workItemFunctor);
    }

    bool Dispatch(WorkItemFunctorType&& workItemFunctor)
    {
        return workItemFunctor == nullptr || _asyncQueue.Push(std::move(workItemFunctor));
    }

    bool DispatchAndWait(WorkItemFunctorType&& workItemFunctor)
    {
        bool ret = workItemFunctor == nullptr || _asyncQueue.Push(std::move(workItemFunctor));
        if (ret)
        {
            _asyncQueue.WaitForAll();
        }
        return ret;
    }

    void Shutdown()
    {
        _asyncQueue.Uninitialize();
    }

    void Initialize()
    {
        auto queueItemHandler = std::make_shared<QueueItemHandler>();

        _asyncQueue.Initialize(queueItemHandler);
    }

private:

    class QueueItemHandler final : public IQueueItemHandler<WorkItemFunctorType>
    {
    public:
        QueueItemHandler() {}
        ~QueueItemHandler() {}

        void OnStarted() override {}
        void OnProcessQueueItem(_In_ WorkItemFunctorType& workItemFunctor) override { workItemFunctor(); }
        void OnStopped() override {}
    };

    AsyncQueue<WorkItemFunctorType> _asyncQueue;
};

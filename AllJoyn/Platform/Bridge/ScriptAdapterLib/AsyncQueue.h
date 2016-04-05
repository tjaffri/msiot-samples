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

// Interface implemented by callers
template <class QueueItem>
class IQueueItemHandler
{
public:
    virtual void OnStarted() = 0;
    virtual void OnProcessQueueItem(_In_ QueueItem& item) = 0;
    virtual void OnStopped() = 0;
};

static bool s_crtIsTerminating = false;
static bool s_registered_atexit_handler = false;

// Implements a FIFO queue that has its own worker thread.
// Callers must register a handler interface during initialization.
// Callers can push items onto the queue. The registered interface is then notified
// from the AsyncQueue's worker thread to process each queued item.
template <class QueueItem>
class AsyncQueue
{
public:
    AsyncQueue() :
        _stopWorkerThread(false),
        _workerThreadStopped(false),
        _handler(nullptr),
        _isInitialized(false)
    {
        // Ensure that we register an "atexit" handler for the CRT.
        // This handler is used to let us know when the CRT is terminating.
        if (!s_registered_atexit_handler)
        {
            if (std::atexit(AsyncQueue_atexit_handler) == 0)
            {
                s_registered_atexit_handler = true;
            }
        }
    };

    ~AsyncQueue()
    {
        Uninitialize();
    }

    void Initialize(_In_ const std::shared_ptr<IQueueItemHandler<QueueItem>>& handler)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (handler == nullptr) throw new std::invalid_argument("handler cannot be null");

        // Verify that this async queue has not already been initialized
        if (!_isInitialized)
        {
            _stopWorkerThread = false;
            _workerThreadStopped = false;
            _handler = handler;

            // Start the thread.
            try
            {
                _workerThread = std::thread(&AsyncQueue::WaitForAndProcessItems, this);
                _isInitialized = true;
            }
            catch (const std::exception&)
            {
                // Since the thread couldn't start, remove our reference on the handler
                _handler = nullptr;
                throw;
            }
        }
    }

    void Uninitialize()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        std::queue<QueueItem> emptyQueue;

        if (_isInitialized)
        {
            if (!_stopWorkerThread)
            {
                // Signal the worker thread to stop
                _stopWorkerThread = true;
                _actionRequired.notify_one();

                // If the caller did not gracefully Uninitialize the queue and we are being destroyed as the result
                // of the CRT being torn down, we need to modify our wait behavior.
                // Wait for the worker thread to finish, unless the CRT is terminating (all std::threads will have been stopped if the CRT is terminating).
                if (!s_crtIsTerminating)
                {
                    _actionRequired.wait(lock, [this] { return _workerThreadStopped || !_workerThread.joinable(); });
                }
                // Once we get here, the worker thread has finished all work and has exited the threadproc, or it has been terminated by the CRT.
                // If it's in a joinable state, detach it.  Note that terminated/aborted threads are still joinable and must be detached (or joined)
                // else the CRT will throw an exception.
                if (_workerThread.joinable())
                {
                    _workerThread.detach();
                }
            }
            // Clear state
            _items.swap(emptyQueue);
            _handler = nullptr;
            _workerThreadStopped = false;
            _isInitialized = false;
        }
    }

    // Pushes another item onto the queue for processing later on the worker thread.
    // Returns false and no-ops if the queue is not initialized.
    template <typename QueueItemType>
    bool Push(_In_ QueueItemType&& item)
    {
        bool queueWasEmpty = false;

        {
            std::lock_guard<std::mutex> lock(_mutex);

            // Check whether this queue has been initialized
            if (!_isInitialized)
            {
                return false;
            }

            queueWasEmpty = _items.empty();
            _items.push(std::forward<QueueItemType>(item));
        }

        // Only notify the worker thread if the queue is no longer empty
        // (as that's the only way the worker thread would be waiting
        // for this signal)
        if (queueWasEmpty)
        {
            _actionRequired.notify_all();
        }

        return true;
    }

    // Waits until the queue is empty
    void WaitForAll()
    {
        std::unique_lock<std::mutex> lock(_mutex);

        _isEmpty.wait(lock, [this] {return (_items.empty() || _stopWorkerThread); });
    }

private:
    class UnlockGuard
    {
    public:
        UnlockGuard(std::unique_lock<std::mutex>& lock) : _lock(lock) { _lock.unlock(); }
        ~UnlockGuard() { _lock.lock(); }

    private:
        UnlockGuard(const UnlockGuard&) = delete;
        UnlockGuard& operator=(const UnlockGuard&) = delete;

        std::unique_lock<std::mutex>& _lock;
    };

    void WaitForAndProcessItems()
    {
        std::unique_lock<std::mutex> lock(_mutex);

        std::shared_ptr<IQueueItemHandler<QueueItem>> handler = _handler;
        handler->OnStarted();

        for (;;)
        {
            // Wait until either:
            // - queue is not empty
            // - worker thread has been asked to stop
            _actionRequired.wait(lock, [this] { if (_items.empty()) _isEmpty.notify_all(); return (!_items.empty() || _stopWorkerThread); });
            if (_stopWorkerThread)
            {
                break;
            }

            // Move the items to a temporary queue
            std::queue<QueueItem> items(std::move(_items));

            // Process all queue items outside the lock, the UnlockGuard will re-lock on destruction
            UnlockGuard unlock(lock);

            while (!items.empty())
            {
                // Pop the next item off and notify the registered worker to process it
                QueueItem item = std::move(items.front());
                items.pop();

                try
                {
                    handler->OnProcessQueueItem(item);
                }
                catch (...)
                {
                    std::cerr << "Caught exception while processing async queue items.";
                }
            }
        }

        try
        {
            handler->OnStopped();
        }
        catch (...)
        {
            std::cerr << "Caught exception while stopping async queue handler.";
        }

        // Signal that the thread is done
        _workerThreadStopped = true;
        _actionRequired.notify_all();
        //std::notify_all_at_thread_exit(_actionRequired, std::move(lock)); // This should replace the previous line but Android doesn't support it
    }

    static void AsyncQueue_atexit_handler()
    {
        s_crtIsTerminating = true;
    }

    std::queue<QueueItem> _items;
    std::condition_variable _actionRequired;
    std::condition_variable _isEmpty;
    std::mutex _mutex;
    std::thread _workerThread;
    bool _stopWorkerThread;
    bool _workerThreadStopped;
    std::shared_ptr<IQueueItemHandler<QueueItem>> _handler;
    bool _isInitialized;
};

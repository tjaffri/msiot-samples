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
#include "AdapterConstants.h"
#include "ConfigManager.h"
#include "BridgeConfig.h"
#include "IBridge.h"

namespace Bridge
{
    class BridgeDevice;

    class DsbBridge final : public IBridge, public IAdapterSignalListener, public std::enable_shared_from_this<DsbBridge>
    {
    public:
        static std::shared_ptr<DsbBridge> SingleInstance();

        DsbBridge();
        virtual ~DsbBridge();

        // IBridge
        QStatus Initialize() override;
        QStatus Shutdown() override;
        void AddAdapter(_In_ std::shared_ptr<IAdapter> adapter) override;
        void AddDevice(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterDevice> device) override;

        // IAdapterSignalListener implementation
        void AdapterSignalHandler(_In_ std::shared_ptr<IAdapter> sender, _In_ std::shared_ptr<IAdapterSignal> signal, _In_opt_ intptr_t context = 0) override;

        QStatus	InitializeDevices(_In_  std::shared_ptr<IAdapter> adapter, _In_ ConfigManager& configMgr, _In_ bool isUpdate = false);

        inline std::map<int, std::shared_ptr<BridgeDevice>>& GetDeviceList() { return m_deviceList; }

        ConfigManager* GetConfigManager(std::shared_ptr<IAdapter> adapter);

    private:
        static std::shared_ptr<DsbBridge> g_TheOneOnlyInstance;

        QStatus	RegisterAdapterSignalHandlers(std::shared_ptr<IAdapter> adapter, bool isRegister);
        QStatus InitializeAdapters();

        QStatus	CreateDevice(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterDevice> device);
        QStatus	UpdateDevice(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterDevice> device, _In_ bool exposedOnAllJoynBus);
        QStatus RemoveDevice(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterDevice> device);

        void MonitorThread();

        QStatus InitializeInternal();
        QStatus ShutdownInternal();

        // indicate if alljoyn has been initialized
        bool m_alljoynInitialized;

        // underlying adapters
        std::vector<std::shared_ptr<IAdapter>> m_adapters;

        // CSP / configuration for each adapter
        std::vector<ConfigManager*> m_configManagers;

        // device representations
        std::map<int, std::shared_ptr<BridgeDevice>> m_deviceList;

        // Synchronization object
        std::recursive_mutex m_bridgeLock; // TODO: This should be re-architected to use a std mutex
        std::mutex m_threadLock;
        std::condition_variable m_actionRequired;
        std::thread m_thread;
        bool m_triggerReset;
        bool m_stopWorkerThread;
        bool m_workerThreadStopped;

    public:
        ConfigManager* GetConfigManagerForBusObject(_In_ alljoyn_busobject busObject);

        // The following is needed to synchronize access to the configuration file.
        // It is unlikely, but possible that a device is added to the config file
        // at the same time the CSP attempts to read it.
        friend CspBridge;
        std::recursive_mutex& GetLock(); // TODO: This should be re-architected and removed

        //called from background MonitorThread();
        QStatus Reset();
        void TriggerReset() { { std::lock_guard<std::mutex> threadlock(m_threadLock); m_triggerReset = true; } m_actionRequired.notify_one(); }
    };
}

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

#include "IAdapter.h"
#include "DsbServiceNames.h"
#include "Bridge.h"
#include "BridgeConfig.h"
#include "ConfigManager.h"
#include "BridgeDevice.h"
#include "AllJoynHelper.h"
#include "AdapterDevice.h"

using namespace Bridge;
using namespace std;

// Device Arrival Signal
std::string Constants::device_arrival_signal            = "Device_Arrival";
std::string Constants::device_arrival__device_handle    = "Device_Handle";

// Device removal signal
std::string Constants::device_removal_signal            = "Device_Removal";
std::string Constants::device_removal__device_handle    = "Device_Handle";

// Change Of Value signal
std::string Constants::change_of_value_signal           = "Change_Of_Value";
std::string Constants::cov__property_handle             = "Property_Handle";
std::string Constants::cov__attribute_handle            = "Attribute_Handle";

// LampStateChanged Signal
std::string Constants::lamp_state_changed_signal_name   = "LampStateChanged";
std::string Constants::lamp_id__parameter_name          = "LampID";

std::shared_ptr<DsbBridge> DsbBridge::g_TheOneOnlyInstance = nullptr;

std::shared_ptr<DsbBridge> DsbBridge::SingleInstance()
{
	if (g_TheOneOnlyInstance == nullptr)
	{
		g_TheOneOnlyInstance = std::shared_ptr<DsbBridge>(new DsbBridge());
	}
    return g_TheOneOnlyInstance;
}

/*static*/ std::shared_ptr<IBridge> IBridge::Get()
{
    return DsbBridge::SingleInstance();
}

DsbBridge::DsbBridge() :
    m_alljoynInitialized(false),
    m_triggerReset(false),
    m_stopWorkerThread(false),
    m_workerThreadStopped(true)
{
}

DsbBridge::~DsbBridge()
{
    Shutdown();
}

void DsbBridge::AddAdapter(std::shared_ptr<IAdapter> adapter)
{
    m_adapters.insert(m_adapters.end(), adapter);
    BridgeLog::LogInfo(adapter->AdapterName());
}

void DsbBridge::MonitorThread()
{
    BridgeLog::LogEnter(__FUNCTION__);
    std::unique_lock<std::mutex> lock(m_threadLock);

    for (;;)
    {
        m_actionRequired.wait(lock, [this] { return m_triggerReset || m_stopWorkerThread; });

        if (m_stopWorkerThread)
        {
            goto Leave;
        }

        if (m_triggerReset)
        {
            BridgeLog::LogInfo("Adapter Reset Event");
            int32_t hr = g_TheOneOnlyInstance->Reset();
            if (hr < 0)
            {
                BridgeLog::LogError("Adapter Reset Failed", hr);
                goto Leave;
            }
            m_triggerReset = false;
        }
    }

Leave:
    m_workerThreadStopped = true;
    m_actionRequired.notify_all();
    BridgeLog::LogLeave(__FUNCTION__);
}

QStatus
DsbBridge::Initialize()
{
    BridgeLog::LogEnter(__FUNCTION__);
    QStatus hr = ER_OK;

    std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);

    // If the background thread has been created, then we have already called Initialize.
    // Caller must call Shutdown first
    if (m_alljoynInitialized)
    {
        return ER_INIT_FAILED;
    }

    // initialize AllJoyn
    QStatus status = alljoyn_init();
    if (ER_OK != status)
    {
        hr = status;
        goto Leave;
    }
    m_alljoynInitialized = true;

    m_thread = std::thread(&DsbBridge::MonitorThread, this);
    m_workerThreadStopped = false;

    hr = this->InitializeInternal();
    if (hr != ER_OK)
    {
        goto Leave;
    }

Leave:
    if (hr != ER_OK)
    {
        this->Shutdown();
    }

    BridgeLog::LogLeave(__FUNCTION__, hr);
    return hr;
}

QStatus
DsbBridge::InitializeInternal()
{
    BridgeLog::LogEnter(__FUNCTION__);
    QStatus hr = ER_OK;

    // initialize adapter
    hr = this->InitializeAdapters();
    if (hr != ER_OK)
    {
        goto leave;
    }

leave:
    BridgeLog::LogLeave(__FUNCTION__);
    return hr;
}


QStatus
DsbBridge::Shutdown()
{
    BridgeLog::LogInfo("DsbBridge::Shutdown");
    QStatus hr = ER_OK;
    std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);

    if (!m_workerThreadStopped)
    {
        {
            std::lock_guard<std::mutex> threadlock(m_threadLock);
            m_stopWorkerThread = true;
        }
        m_actionRequired.notify_one();
        {
            std::unique_lock<std::mutex> threadlock(m_threadLock);
            m_actionRequired.wait(threadlock, [this] { return m_workerThreadStopped || !m_thread.joinable(); });
        }
    }

    if (m_thread.joinable())
    {
        m_thread.detach();
    }

    hr = ShutdownInternal();

    if (m_alljoynInitialized)
    {
        alljoyn_shutdown();
        m_alljoynInitialized = false;
    }

    BridgeLog::LogLeave(__FUNCTION__, hr);
    return hr;
}

QStatus DsbBridge::ShutdownInternal()
{
    BridgeLog::LogEnter(__FUNCTION__);
    QStatus hr = ER_OK;

    for (auto iter = m_configManagers.begin(); iter != m_configManagers.end(); iter++)
    {
        ConfigManager* mgr = *iter;
        mgr->Shutdown();
        delete mgr;
    }

    m_configManagers.clear();

    for (auto iter = m_adapters.begin(); iter != m_adapters.end(); iter++)
    {
        std::shared_ptr<IAdapter> adapter = *iter;

        (void)this->RegisterAdapterSignalHandlers(adapter, false);

        hr = adapter->Shutdown() == ERROR_SUCCESS ? ER_OK : ER_FAIL;
    }

    m_adapters.clear();

    for (auto val : this->m_deviceList)
    {
        val.second->Shutdown();
    }
    this->m_deviceList.clear();

    BridgeLog::LogLeave(__FUNCTION__);
    return hr;
}

QStatus DsbBridge::InitializeDevices(_In_  std::shared_ptr<IAdapter> adapter, _In_ ConfigManager& configMgr, _In_ bool isUpdate)
{
    bool bUpdateXmlConfig = false;
    QStatus status = ER_OK;
    uint32_t dwStatus = ERROR_SUCCESS;
    AdapterDeviceVector devicesList;
    std::shared_ptr<IAdapterIoRequest> request;

    //enumerate all devices
    dwStatus = adapter->EnumDevices(
        isUpdate ? ENUM_DEVICES_OPTIONS::CACHE_ONLY : ENUM_DEVICES_OPTIONS::FORCE_REFRESH,
        devicesList,
        request);

    if (ERROR_IO_PENDING == dwStatus)
    {
        // wait for completion
        dwStatus = request->Wait(WAIT_TIMEOUT_FOR_ADAPTER_OPERATION);
    }
    if (ERROR_SUCCESS != dwStatus)
    {
        status = ER_FAIL;
        goto Leave;
    }

    {
        // loop through the Devices list and create corresponding AllJoyn bus objects
        std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);
        for (auto device : devicesList)
        {
            DsbObjectConfig objConfigItem;
            try
            {
                bUpdateXmlConfig |= configMgr.GetObjectConfigItem(device, objConfigItem);
            }
            catch (...)
            {
                //TBD - after config xml part is done
                continue;
            }

            if (isUpdate)
            {
                status = UpdateDevice(adapter, device, objConfigItem.bVisible);
            }
            else if (objConfigItem.bVisible)
            {
                // only create device that are exposed
                // don't leave if a device cannot be created, just create the others
                status = CreateDevice(adapter, device);
            }
        }

        // Save the Bridge Configuration to an XML file
        if (bUpdateXmlConfig)
        {
            configMgr.ToFile();
        }
    }
Leave:
    return status;
}

QStatus DsbBridge::AddDevice(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterDevice> device)
{
    return CreateDevice(adapter, device);
}

QStatus DsbBridge::CreateDevice(std::shared_ptr<IAdapter> adapter, std::shared_ptr<IAdapterDevice> device)
{
    QStatus status = ER_OK;
    std::shared_ptr<BridgeDevice> newDevice = nullptr;
    std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);
    AdapterDevice* ad = nullptr;

    //DeviceAdded(adapter, device);

    // create and initialize the device
    newDevice = std::make_shared<BridgeDevice>();
    if (nullptr == newDevice.get())
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    status = newDevice->Initialize(adapter, device);
    if (ER_OK != status)
    {
        goto leave;
    }

    // add device in device list
    m_deviceList.insert(std::make_pair(device->GetHashCode(), newDevice));

    // Pass the BridgeDevice to the AdapterDevice. This is used to raise signals back to AJ (see AdapterDevice::RaiseSignal).
    ad = dynamic_cast<AdapterDevice*>(device.get());
    if (ad != nullptr) {
        ad->BridgeDevice() = newDevice;
    }

leave:
    if (ER_OK != status)
    {
        if (nullptr != newDevice)
        {
            newDevice->Shutdown();
        }
    }

    return status;
}

QStatus DsbBridge::UpdateDevice(_In_  std::shared_ptr<IAdapter> adapter, std::shared_ptr<IAdapterDevice>  device, bool exposedOnAllJoynBus)
{
    QStatus status = ER_OK;

    // check if device is already in device list (hence exposed on alljoyn)
    auto deviceIterator = m_deviceList.find(device->GetHashCode());
    if (deviceIterator == m_deviceList.end() &&
        exposedOnAllJoynBus)
    {
        // device doesn't exist (hence isn't exposed on AllJoyn) => create it (creation also expose on alljoyn)
        status = CreateDevice(adapter, device);
    }
    else if (deviceIterator != m_deviceList.end() && !exposedOnAllJoynBus)
    {
        std::shared_ptr<BridgeDevice> tempDevice = deviceIterator->second;
        m_deviceList.erase(deviceIterator);
        status = RemoveDevice(adapter, device);
        tempDevice->Shutdown();
    }

    return status;
}

QStatus DsbBridge::RemoveDevice(_In_  std::shared_ptr<IAdapter> adapter, std::shared_ptr<IAdapterDevice>  device)
{
    //DeviceRemoved(adapter, device);
    return QStatus::ER_OK;
}

_Use_decl_annotations_
void
DsbBridge::AdapterSignalHandler(
    std::shared_ptr<IAdapter> sender,
    std::shared_ptr<IAdapterSignal> signal,
	intptr_t /*context*/
    )
{
    DsbObjectConfig objConfigItem;
    std::shared_ptr<IAdapterDevice> adapterDevice = nullptr;

    if (signal->Name() == Constants::DEVICE_ARRIVAL_SIGNAL() ||
        signal->Name() == Constants::DEVICE_REMOVAL_SIGNAL())
    {
        //look for the device handle
        for (auto param : signal->Params())
        {
            if (param->Name() == Constants::DEVICE_ARRIVAL__DEVICE_HANDLE() ||
                param->Name() == Constants::DEVICE_REMOVAL__DEVICE_HANDLE())
            {
				PropertyValue propVal = param->Data();
                adapterDevice = propVal.Get<std::shared_ptr<IAdapterDevice>>();
                break;
            }
        }
        if (adapterDevice == nullptr)
        {
            goto Leave;
        }
    }


    if (signal->Name() == Constants::DEVICE_ARRIVAL_SIGNAL())
    {
        // get config for the device
        try
        {
            ConfigManager* mgr = GetConfigManager(sender);
            if (mgr != nullptr)
            {
                bool bUpdateXml = mgr->GetObjectConfigItem(adapterDevice, objConfigItem);
                if (bUpdateXml)
                {
                    std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);
                    mgr->ToFile();
                }
            }
        }
        catch (...)
        {
            //TBD - after config xml part is done
        }

        // create the new device and expose it on alljoyn (if required)
        if (objConfigItem.bVisible)
        {
            // only create device that are exposed
            //
            // Note: creation failure is handle inside CreateDevice and there is no more
            // that can be done to propagate error in this routine => don't check status
            CreateDevice(sender, adapterDevice);
        }
    }
    else if (signal->Name() == Constants::DEVICE_REMOVAL_SIGNAL())
    {
        // remove device
        // note that config doesn't need to be updated, if the device come again
        // it will be shown again and config clean up is done by CSP)
        std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);
        UpdateDevice(sender, adapterDevice, false);
    }

Leave:
    return;
}

QStatus
DsbBridge::RegisterAdapterSignalHandlers(std::shared_ptr<IAdapter> adapter, bool isRegister)
{
    std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);

    for (auto signal : adapter->Signals())
    {
        uint32_t status;

        if (isRegister)
        {
            status = adapter->RegisterSignalListener(signal, g_TheOneOnlyInstance->shared_from_this(), 0);
        }

        else
        {
            status = adapter->UnregisterSignalListener(signal, g_TheOneOnlyInstance->shared_from_this());
        }

        if (status != ERROR_SUCCESS)
        {
            return ER_FAIL;
        }
    }

    return ER_OK;
}

QStatus
DsbBridge::InitializeAdapters()
{
    QStatus hr = ER_OK;

    if (this->m_adapters.size() == 0)
    {
        hr = ER_FAIL;
        goto Leave;
    }

    for (auto iter = m_adapters.begin(); hr == ER_OK && iter != m_adapters.end(); iter++)
    {
        std::shared_ptr<IAdapter> adapter = *iter;
		hr = adapter->Initialize() == ERROR_SUCCESS ? ER_OK : ER_FAIL;
        if (hr == 0)
        {
            ConfigManager* mgr = new ConfigManager();
            this->m_configManagers.push_back(mgr);
            hr = mgr->Initialize(adapter, g_TheOneOnlyInstance->shared_from_this()) == ERROR_SUCCESS ? ER_OK : ER_FAIL;
            if (QS_SUCCEEDED(hr))
            {
                // connect config manager to AllJoyn
                QStatus status = mgr->ConnectToAllJoyn();
                if (ER_OK != status)
                {
                    hr = status;
                }
            }

            if (QS_SUCCEEDED(hr))
            {
                // initialize devices
                hr = InitializeDevices(adapter, *mgr);
            }

            if (QS_SUCCEEDED(hr))
            {
                hr = this->RegisterAdapterSignalHandlers(adapter, true);
            }
        }
    }

Leave:
    return hr;
}

ConfigManager* DsbBridge::GetConfigManager(std::shared_ptr<IAdapter> adapter)
{
    ConfigManager* mgr = nullptr;
    for (auto iter = m_configManagers.begin(); iter != m_configManagers.end(); iter++)
    {
        ConfigManager* config = *iter;
        if (config->GetAdapter() == adapter)
        {
            mgr = config;
            break;
        }
    }
    return mgr;
}

ConfigManager* DsbBridge::GetConfigManagerForBusObject(_In_ alljoyn_busobject busObject)
{
    for (auto iter = m_configManagers.begin(); iter != m_configManagers.end(); iter++)
    {
        ConfigManager* config = *iter;
        if (config->HasBusObject(busObject))
        {
            return config;
        }
    }
    return nullptr;
}

std::recursive_mutex& DsbBridge::GetLock()
{
    return m_bridgeLock;
}

QStatus DsbBridge::Reset()
{
    QStatus hr = ER_OK;
    std::lock_guard<std::recursive_mutex> lock(m_bridgeLock);

    hr = this->ShutdownInternal();
    if (QS_SUCCEEDED(hr))
    {
        hr = this->InitializeInternal();
        if (QS_FAILED(hr))
        {
            this->ShutdownInternal();
        }
    }

    return hr;
}

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

#include <thread>
#include <chrono>

#include "BridgeCommon.h"
#include "Bridge.h"
#include "BridgeLog.h"
#include "DsbServiceNames.h"
#include "CspAdapter.h"
#include "CspBridge.h"
#include "AllJoynAbout.h"
#include "AllJoynFileTransfer.h"
#include "BridgeConfig.h"
#include "AllJoynHelper.h"
#include "FileAccess.h"

#include "ConfigManager.h"

using namespace Bridge;
using namespace BridgePAL;

static const uint32_t SESSION_LINK_TIMEOUT = 30;        // seconds

ConfigManager::ConfigManager()
    : m_adapter(nullptr),
    m_AJBusAttachment(nullptr),
    m_AJBusListener(nullptr),
    m_parent(nullptr)
{
}

ConfigManager::~ConfigManager()
{
    Shutdown();
}

QStatus ConfigManager::Initialize(_In_ std::shared_ptr<IAdapter> adapter, std::shared_ptr<DsbBridge> bridge)
{
    QStatus hr = ER_OK;

    if (nullptr == bridge)
    {
        hr = ER_BAD_ARG_2;
        goto Leave;
    }

    // sanity check
    if (nullptr == adapter)
    {
        hr = ER_BAD_ARG_1;
        goto Leave;
    }

    m_parent = bridge;
    m_adapter = adapter;

    m_fileName = adapter->AdapterName() + ".xml";

    hr = m_bridgeConfig.Init(m_fileName);
    if (QS_FAILED(hr))
    {
        goto Leave;
    }

Leave:
    return hr;
}

QStatus ConfigManager::Shutdown()
{
    ShutdownAllJoyn();
    m_adapter = nullptr;
    m_parent = nullptr;
    return ER_OK;
}

QStatus ConfigManager::ConnectToAllJoyn()
{
    const int MAX_ATTEMPTS = 60;
    const int ATTEMPT_DELAY_MS = 500;

    BridgeLog::LogEnter(__FUNCTION__);
    QStatus status = ER_OK;
    alljoyn_buslistener_callbacks busListenerCallbacks = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    alljoyn_sessionportlistener_callbacks sessionPortListenerCallbacks = { ConfigManager::AcceptSessionJoinerCallback, ConfigManager::SessionJoined };
    alljoyn_sessionlistener_callbacks sessionListenerCallbacks = { nullptr, nullptr, (alljoyn_sessionlistener_sessionmemberremoved_ptr)ConfigManager::MemberRemoved };
    alljoyn_sessionopts opts = nullptr;
    alljoyn_sessionport sp = DSB_SERVICE_PORT;
    std::string appName;

	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	
	// verify if already connected
    if (nullptr != m_AJBusAttachment)
    {
        goto Leave;
    }

    // build service name
    status = BuildServiceName();
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Could not build service name.");
        goto Leave;
    }

    // create the bus attachment
    AllJoynHelper::EncodeStringForAppName(m_adapter->ExposedApplicationName(), appName);
    m_AJBusAttachment = alljoyn_busattachment_create(appName.c_str(), QCC_TRUE);
    if (nullptr == m_AJBusAttachment)
    {
        BridgeLog::LogInfo("Could not create bus attachment.");
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    // create the bus listener
    m_AJBusListener = alljoyn_buslistener_create(&busListenerCallbacks, nullptr);
    if (nullptr == m_AJBusListener)
    {
        BridgeLog::LogInfo("Could not create bus listener.");
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    // introduce the bus attachment and the listener
    alljoyn_busattachment_registerbuslistener(m_AJBusAttachment, m_AJBusListener);

    // start the bus attachment
    status = alljoyn_busattachment_start(m_AJBusAttachment);
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Could not start bus attachment.");
        goto Leave;
    }

    // initialize about service
    status = m_about.Initialize(m_AJBusAttachment);
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Could not initialize.");
        goto Leave;
    }

    // set adapter info in about
    m_about.SetApplicationName(m_adapter->ExposedApplicationName().c_str());
    m_about.SetApplicationGuid(m_adapter->ExposedApplicationGuid().c_str());
    m_about.SetManufacturer(m_adapter->Vendor().c_str());
	m_about.SetDeviceName(fileAccess->GetHostName().c_str());
    m_about.SetSWVersion(m_adapter->Version().c_str());
    m_about.SetModel(m_adapter->AdapterName().c_str());

    status = InitializeCSPBusObjects();
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Problem initializing CSP Bus Objects.");
        goto Leave;
    }

    //
    // Due to a legacy issue with the FltMgr Driver, it is possible that the NamedPipeTrigger (NpSvcTrigger) filter driver will not
    // be attached to the Named Pipe Device Driver.  Normally when a client tries to connect to alljoyn, MSAJAPI (the alljoyn library)
    // will open a pipe connection with the AllJoyn Router Service.  The AllJoyn Router Service is configured as demand start.  The
    // act of connecting to the router should trigger the AllJoyn Router Service to start through the NpSvcTrigger filter drive.  On
    // boot however, the NpSvcTrigger filter driver isn't properly attached to the Named Pipe Driver for up to 15 seconds.  During this
    // time, if a client tries to connect to AllJoyn an ER_TRANSPORT_NOT_AVAILABLE error is returned from AllJoyn instead.
    //
    // To mitigate this issue, the following loop attempts to reconnect for up to 30 seconds.
    //
    {
        int attempt = 0;
        for (;;)
        {
            // connect the bus attachment
            status = alljoyn_busattachment_connect(m_AJBusAttachment, nullptr);
            if (ER_OK == status)
            {
                break;
            }
            
            if (attempt >= MAX_ATTEMPTS)
            {
                goto Leave;
            }
            
            BridgeLog::LogInfo("Retrying bus attachment.");
            ++attempt;
            std::this_thread::sleep_for(std::chrono::milliseconds(ATTEMPT_DELAY_MS));
        }
    }

    /*
    * Advertise this service on the bus.
    * There are three steps to advertising this service on the bus.
    * 1) Request a well-known name that will be used by the client to discover
    *    this service.
    * 2) Create a session.
    * 3) Advertise the well-known name.
    */
    status = alljoyn_busattachment_requestname(m_AJBusAttachment, m_serviceName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Failed to request bus attachment name.");
        goto Leave;
    }

    // callback will get this class as context
    m_sessionListener = alljoyn_sessionlistener_create(&sessionListenerCallbacks, this);
    if (nullptr == m_sessionListener)
    {
        BridgeLog::LogInfo("Failed to create session listener.");
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    m_sessionPortListener = alljoyn_sessionportlistener_create(&sessionPortListenerCallbacks, this);
    if (nullptr == m_sessionPortListener)
    {
        BridgeLog::LogInfo("Failed to create session port listener.");
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_TRUE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    if (nullptr == opts)
    {
        BridgeLog::LogInfo("Failed to create session options.");
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    status = alljoyn_busattachment_bindsessionport(m_AJBusAttachment, &sp, opts, m_sessionPortListener);
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Failed to bind session port.");
        goto Leave;
    }

    status = alljoyn_busattachment_advertisename(m_AJBusAttachment, m_serviceName.c_str(), alljoyn_sessionopts_get_transports(opts));
    if (ER_OK != status)
    {
        BridgeLog::LogInfo("Failed to advertise service name");
        goto Leave;
    }

    // announce
    m_about.Announce();

Leave:
    if (nullptr != opts)
    {
        alljoyn_sessionopts_destroy(opts);
    }

    if (ER_OK != status)
    {
        ShutdownAllJoyn();
    }

    BridgeLog::LogLeave(__FUNCTION__, status);
    return status;
}

QStatus ConfigManager::ShutdownAllJoyn()
{
    QStatus status = ER_OK;

    // verify if already shutdown
    if (nullptr == m_AJBusAttachment)
    {
        goto Leave;
    }

    // note that destruction of all alljoyn bus objects (CSP, about and device related)
    // and interfaces must be performed before bus attachment destruction
    // cancel advertised name and session port binding
    if (!m_serviceName.empty())
    {
        alljoyn_busattachment_canceladvertisename(this->m_AJBusAttachment, m_serviceName.c_str(), ALLJOYN_TRANSPORT_ANY);
    }
    alljoyn_busattachment_unbindsessionport(this->m_AJBusAttachment, DSB_SERVICE_PORT);

    if (nullptr != this->m_sessionPortListener)
    {
        alljoyn_sessionportlistener_destroy(this->m_sessionPortListener);
        this->m_sessionPortListener = nullptr;
    }

    if (!m_serviceName.empty())
    {
        alljoyn_busattachment_releasename(this->m_AJBusAttachment, m_serviceName.c_str());
        m_serviceName.clear();
    }
    alljoyn_busattachment_disconnect(this->m_AJBusAttachment, nullptr);

    // destroy CSP interfaces
    m_adapterCSP.Destroy();
    m_bridgeCSP.Destroy();

    // remove authentication handler
    m_authHandler.ShutDown();

    // shutdown about
    m_about.ShutDown();

    alljoyn_busattachment_stop(this->m_AJBusAttachment);

    // destroy bus listener and session port listener
    if (nullptr != this->m_AJBusListener)
    {
        alljoyn_busattachment_unregisterbuslistener(this->m_AJBusAttachment, this->m_AJBusListener);
        alljoyn_buslistener_destroy(this->m_AJBusListener);
        this->m_AJBusListener = nullptr;
    }

    alljoyn_busattachment_destroy(this->m_AJBusAttachment);
    this->m_AJBusAttachment = nullptr;

Leave:
    return status;
}

QStatus ConfigManager::InitializeCSPBusObjects()
{
    QStatus status = ER_OK;

    // init CSP related bus objects if this isn't an update
    //-------------------------------------------------------
    if (IsConfigurationAccessSecured())
    {
		status = m_authHandler.InitializeWithKeyXAuthentication(m_AJBusAttachment, m_bridgeConfig.BridgeKeyX());
        if (ER_OK != status)
        {
            goto Leave;
        }
    }

    status = m_adapterCSP.Initialize(&m_AJBusAttachment, this);
    if (ER_OK != status)
    {
        goto Leave;
    }

    // add bus object description in about service
    status = m_about.AddObject(m_adapterCSP.GetBusObject(), m_adapterCSP.GetInterface());
    if (ER_OK != status)
    {
        goto Leave;
    }

    status = m_bridgeCSP.Initialize(&m_AJBusAttachment, this, m_bridgeConfig);
    if (ER_OK != status)
    {
        goto Leave;
    }
    // add bus object description in about service
    status = m_about.AddObject(m_bridgeCSP.GetBusObject(), m_bridgeCSP.GetInterface());
    if (ER_OK != status)
    {
        goto Leave;
    }

Leave:
    return status;
}

bool Bridge::ConfigManager::IsConfigurationAccessSecured()
{
	if (!m_bridgeConfig.BridgeKeyX().empty())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Bridge::ConfigManager::IsDeviceAccessSecured()
{
	if (!m_bridgeConfig.DeviceKeyX().empty() ||
        (!m_bridgeConfig.DeviceUsername().empty() &&
		 !m_bridgeConfig.DevicePassword().empty()) ||
        (!m_bridgeConfig.DeviceEcdheEcdsaPrivateKey().empty() &&
         !m_bridgeConfig.DeviceEcdheEcdsaCertChain().empty()))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ConfigManager::HasBusObject(_In_ alljoyn_busobject busObject)
{
    return (m_bridgeCSP.GetBusObject() == busObject) ||
           (m_adapterCSP.GetBusObject() == busObject);
}

AllJoynFileTransfer* ConfigManager::GetAllJoynFileTransferInstance(_In_ alljoyn_busobject busObject)
{
    AllJoynFileTransfer *objectInstance = nullptr;

    if (m_bridgeCSP.GetBusObject() == busObject)
    {
        objectInstance = reinterpret_cast<AllJoynFileTransfer *> (&m_bridgeCSP);
    }
    else if (m_adapterCSP.GetBusObject() == busObject)
    {
        objectInstance = reinterpret_cast<AllJoynFileTransfer *> (&m_adapterCSP);
    }

    return objectInstance;
}

QStatus ConfigManager::SetDeviceConfig(_In_ std::wstring &tempFileName, _Out_ std::shared_ptr<DsbBridge>& finalEvent)
{
    QStatus updateStatus = ER_OK;
    QStatus hr = ER_OK;
    std::string bridgeConfigFileName = StringUtils::ToUtf8(tempFileName.c_str());
    BridgeConfig newConfig;

    std::string keyX = m_bridgeConfig.BridgeKeyX();

    std::string deviceKeyX = m_bridgeConfig.DeviceKeyX();
    std::string deviceUserName = m_bridgeConfig.DeviceUsername();
    std::string devicePassword = m_bridgeConfig.DevicePassword();
    std::string deviceEcdheEcdsaPrivateKey = m_bridgeConfig.DeviceEcdheEcdsaPrivateKey();
    std::string deviceEcdheEcdsaCertChain = m_bridgeConfig.DeviceEcdheEcdsaCertChain();

    // default final event to nothing
    finalEvent.reset();

    // Try to load the new configuration file
    hr = newConfig.Init(bridgeConfigFileName);
    if (QS_FAILED(hr))
    {
        goto Leave;
    }

    // Try to merge the new configuration file with the original configuration file.
    // On failure, reload the in-memory version from file
    hr = m_bridgeConfig.MergeFrom(newConfig);
    if (QS_FAILED(hr))
    {
        m_bridgeConfig.Init(m_fileName);
        goto Leave;
    }

    // Update the bridge configuration file with the new data values
    hr = m_bridgeConfig.ToFile();
    if (QS_FAILED(hr))
    {
        goto Leave;
    }

    // If one of the authentication methods has changed, signal a reset request.
    // This will shutdown all devices including the Bridge and then regenerate them
    // with whatever visibility was specified in the current bridge config file
    if (keyX != m_bridgeConfig.BridgeKeyX() ||
        deviceKeyX != m_bridgeConfig.DeviceKeyX() ||
        deviceUserName != m_bridgeConfig.DeviceUsername() ||
        devicePassword != m_bridgeConfig.DevicePassword() ||
        deviceEcdheEcdsaPrivateKey != m_bridgeConfig.DeviceEcdheEcdsaPrivateKey() ||
        deviceEcdheEcdsaCertChain != m_bridgeConfig.DeviceEcdheEcdsaCertChain())
    {
        // set post write file event to reset event
        finalEvent = this->m_parent;
    }
    // Otherwise, update the status of each adapter device to either expose or hide
    // it from AllJoyn based on the new bridge configuration settings
    else
    {
        if (nullptr != m_parent)
        {
            std::lock_guard<std::recursive_mutex> lock(DsbBridge::SingleInstance()->GetLock());
            updateStatus = m_parent->InitializeDevices(this->m_adapter, *this, true);
            if (updateStatus != ER_OK)
            {
                hr = ER_FAIL;
            }
        }
    }

Leave:
    return hr;
}


QStatus ConfigManager::SetAdapterConfig(_In_ std::vector<uint8_t>& adapterConfig)
{
    m_adapter->SetConfiguration(adapterConfig);
    return ER_OK;
}


QStatus ConfigManager::GetAdapterConfig(_Out_ std::vector<uint8_t>& pAdapterConfig)
{
    m_adapter->GetConfiguration(pAdapterConfig);
    return ER_OK;
}

bool ConfigManager::GetObjectConfigItem(_In_ std::shared_ptr<IAdapterDevice> device, _Out_ DsbObjectConfig& objConfigItem)
{
    bool bConfigItemAdded = false;
    // Select this object from the object configuration file
    QStatus hr = m_bridgeConfig.FindObject(device->SerialNumber(), objConfigItem);

    // If the object wasn't found, then try to add it to the configuration file, defaulted to false
    // If it can't be added, we will still not expose it
    if (hr == ER_NONE)
    {
        // Try to add the object to the in memory configuration.  (Try is here to handle potential exceptions from wstring)
        try
        {
            objConfigItem.bVisible = m_bridgeConfig.DefaultVisibility();
            objConfigItem.id = device->SerialNumber();
            objConfigItem.description = device->Model();
            hr = m_bridgeConfig.AddObject(objConfigItem);
            bConfigItemAdded = true;
        }

        catch (...)
        {
            throw;
        }
    }
    else if (QS_FAILED(hr))
    {
        // Unexpected error searching for device configuration
        throw std::invalid_argument("Invalid configuration item");
    }
    
    // Return whether or not this is a new item (indicates that the in-memory XML file needs to be saved back to disk)
    // Save is not automatically done here to allow caller a chance to decide when it is appropriate to perform update)
    return bConfigItemAdded;
}

void ConfigManager::ToFile()
{
    // Try to update configuration file, do not fail on write error
    (void)m_bridgeConfig.ToFile();
}

QCC_BOOL AJ_CALL ConfigManager::AcceptSessionJoinerCallback(const void * /*context*/, alljoyn_sessionport sessionPort, const char * /*joiner*/, const alljoyn_sessionopts /*opts*/)
{
    if (DSB_SERVICE_PORT != sessionPort)
    {
        return QCC_FALSE;
    }
    else
    {
        return QCC_TRUE;
    }
}

void ConfigManager::SessionJoined(_In_ const void *context, _In_ alljoyn_sessionport /*sessionPort*/, _In_ alljoyn_sessionid id, _In_ const char* /*joiner*/)
{
    QStatus status = ER_OK;
    const ConfigManager *configManager = nullptr;
    uint32_t timeOut = SESSION_LINK_TIMEOUT;

    configManager = reinterpret_cast<const ConfigManager *> (context);
    if (nullptr == configManager)
    {
        goto leave;
    }

    // Enable concurrent callbacks since some of the calls below could block
    alljoyn_busattachment_enableconcurrentcallbacks(configManager->m_AJBusAttachment);

    // set session listener and time-out
    status = alljoyn_busattachment_setsessionlistener(configManager->m_AJBusAttachment, id, configManager->m_sessionListener);
    if (status != ER_OK)
    {
        goto leave;
    }
    status = alljoyn_busattachment_setlinktimeout(configManager->m_AJBusAttachment, id, &timeOut);
    if (status != ER_OK)
    {
        goto leave;
    }

leave:
    return;
}

void ConfigManager::MemberRemoved(_In_  void* context, _In_ alljoyn_sessionid /*sessionid*/, _In_ const char* uniqueName)
{
    ConfigManager *configManager = nullptr;

    // get config manager instance
    configManager = reinterpret_cast<ConfigManager *> (context);
    if (nullptr == configManager)
    {
        goto leave;
    }

    // reset access
    configManager->m_authHandler.ResetAccess(uniqueName);

    // end CSP file transfer
    configManager->m_adapterCSP.EndTransfer();
    configManager->m_bridgeCSP.EndTransfer();

leave:
    return;
}

QStatus ConfigManager::BuildServiceName()
{
    QStatus status = ER_OK;
    std::string tempString;

    m_serviceName.clear();

    // sanity check
    if (nullptr == m_adapter)
    {
        status = ER_INVALID_DATA;
        goto leave;
    }

    // set root/prefix for AllJoyn service name (aka bus name) and interface names :
    // 'prefixForAllJoyn'.DeviceSystemBridge.'AdapterName
    AllJoynHelper::EncodeStringForRootServiceName(m_adapter->ExposedAdapterPrefix(), tempString);
    if (tempString.empty())
    {
        status = ER_BUS_BAD_BUS_NAME;
        goto leave;
    }
    m_serviceName += tempString;

    AllJoynHelper::EncodeStringForServiceName(m_adapter->AdapterName(), tempString);
    if (tempString.empty())
    {
        status = ER_BUS_BAD_BUS_NAME;
        goto leave;
    }
    m_serviceName += ".";
    m_serviceName += tempString;


leave:
    return status;
}

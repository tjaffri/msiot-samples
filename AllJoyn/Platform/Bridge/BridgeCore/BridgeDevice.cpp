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

#include <sstream>

#include "Bridge.h"
#include "DsbServiceNames.h"
#include "BridgeDevice.h"
#include "DeviceProperty.h"
#include "PropertyInterface.h"
#include "DeviceMain.h"
#include "AllJoynHelper.h"
#include "ControlPanel.h"

using namespace Bridge;
using namespace std;

static const uint32_t FIRST_UNIQUE_ID = 1;
static const string BASE_INTERFACE_NAME = ".interface_";
static const uint32_t SESSION_LINK_TIMEOUT = 30;        // seconds

BridgeDevice::BridgeDevice()
    : m_device(nullptr),
    m_AJBusAttachment(nullptr),
    m_AJBusListener(nullptr),
    m_AJSessionPortListener(nullptr),
    m_uniqueIdForInterfaces(FIRST_UNIQUE_ID),
    m_deviceMain(nullptr),
    m_supportCOVSignal(false),
    m_adapter(nullptr)
{
}
Bridge::BridgeDevice::~BridgeDevice()
{
}

void BridgeDevice::RaiseSignal(std::shared_ptr<IAdapterSignal> signal)
{
    m_deviceMain->HandleSignal(signal);
}

QStatus BridgeDevice::Initialize(std::shared_ptr<IAdapter> adapter, std::shared_ptr<IAdapterDevice> device)
{

    QStatus status = ER_OK;
    QStatus hr = ER_OK;

    // sanity check
    if (nullptr == device)
    {
        status = ER_BAD_ARG_1;
        goto leave;
    }
    m_device = device;
    m_adapter = adapter;

    // create Device service name
    status = BuildServiceName();
    if (ER_OK != status)
    {
        goto leave;
    }

    // init alljoyn
    status = InitializeAllJoyn();
    if (ER_OK != status)
    {
        goto leave;
    }

    // initialize about service
    status = m_about.Initialize(m_AJBusAttachment);
    if (ER_OK != status)
    {
        goto leave;
    }

    // set device info in about
    m_about.SetApplicationName(m_adapter->ExposedApplicationName().c_str());
    m_about.SetApplicationGuid(m_adapter->ExposedApplicationGuid().c_str());
    m_about.SetDeviceName(m_device->Name().c_str());
    m_about.SetManufacturer(m_device->Vendor().c_str());
    m_about.SetModel(m_device->Model().c_str());
    m_about.SetSWVersion(m_device->Version().c_str());
    m_about.SetDeviceId(m_device->SerialNumber().c_str());
    m_about.SetDescription(m_device->Description().c_str());


    // If an Icon is available, try to add it to the bus attachment too.  Just continue on error
    if (device->Icon() != nullptr)
    {
        m_icon.Initialize(m_AJBusAttachment, device->Icon());
    }

    // create device properties
    //status = CreateDeviceProperties();
    //if (ER_OK != status)
    //{
    //    goto leave;
    //}

    // create main device
    m_deviceMain = new(std::nothrow) DeviceMain();
    if (nullptr == m_deviceMain)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }
    status = m_deviceMain->Initialize(this->shared_from_this());
    if (ER_OK != status)
    {
        goto leave;
    }

    // Create a control panel if requested by the caller.
    status = InitControlPanel();
    if (ER_OK != status)
    {
        goto leave;
    }

    m_about.AddObject(m_deviceMain->GetBusObject(), m_deviceMain->GetInterfaceDescription());

    // connect to AllJoyn
    status = ConnectToAllJoyn();
    if (ER_OK != status)
    {
        goto leave;
    }

    // register signals
    hr = RegisterSignalHandlers(true);
    if (hr != ER_OK)
    {
        status = ER_OS_ERROR;
        goto leave;
    }

    // announce
    m_about.Announce();

leave:
    if (ER_OK != status)
    {
        Shutdown();
    }
    return status;
}

void BridgeDevice::Shutdown()
{
    if (nullptr != m_device)
    {
        // unregister signals
        RegisterSignalHandlers(false);
    }

    // note that destruction of all alljoyn bus objects (about and device related)
    // and interfaces must be performed before bus attachment destruction

    // shutdown about
    m_about.ShutDown();

    m_icon.ShutDown();

    if (nullptr != m_pControlPanel)
    {
        delete m_pControlPanel;
        m_pControlPanel = nullptr;
    }

    // shutdown device properties
    for (auto &var : m_deviceProperties)
    {
        var.second->Shutdown();
        delete var.second;
    }
    m_deviceProperties.clear();

    // shutdown main device interface
    if (nullptr != m_deviceMain)
    {
        delete m_deviceMain;
        m_deviceMain = nullptr;
    }

    // shutdown interface
    for (auto &propertyInterface : m_propertyInterfaces)
    {
        delete propertyInterface;
    }
    m_propertyInterfaces.clear();

    // shutdown AllJoyn
    ShutdownAllJoyn();

    // final cleanup
    m_RootStringForAllJoynNames.clear();
    m_ServiceName.clear();
    m_device = nullptr;
    m_supportCOVSignal = false;
    m_adapter = nullptr;
}

QStatus BridgeDevice::BuildServiceName()
{
    QStatus status = ER_OK;
    string tempString;

    m_RootStringForAllJoynNames.clear();

    // set root/prefix for AllJoyn service name (aka bus name) and interface names :
    // 'prefixForAllJoyn'.'AdapterName'.'DeviceName'
    AllJoynHelper::EncodeStringForRootServiceName(m_adapter->ExposedAdapterPrefix(), tempString);
    if (tempString.empty())
    {
        status = ER_BUS_BAD_BUS_NAME;
        goto leave;
    }
    m_RootStringForAllJoynNames += tempString;

    AllJoynHelper::EncodeStringForServiceName(m_adapter->AdapterName(), tempString);
    if (tempString.empty())
    {
        status = ER_BUS_BAD_BUS_NAME;
        goto leave;
    }
    m_RootStringForAllJoynNames += ".";
    m_RootStringForAllJoynNames += tempString;

    // set service name (aka bus name)
    m_ServiceName.clear();
    m_ServiceName = m_RootStringForAllJoynNames;

    //add device name
    AllJoynHelper::EncodeStringForServiceName(m_device->Name(), tempString);
    if (!tempString.empty())
    {
        m_ServiceName += ".";
        m_ServiceName += tempString;
    }

    // add serial number to service name if not empty
    AllJoynHelper::EncodeStringForServiceName(m_device->SerialNumber(), tempString);
    if (!tempString.empty())
    {
        m_ServiceName += ".";
        m_ServiceName += tempString;
    }

leave:
    return status;
}

QStatus BridgeDevice::CreateDeviceProperties()
{
    QStatus status = ER_OK;
    DeviceProperty *deviceProperty = nullptr;

    // verify if change of value signal (aka COV) is supported
    // this must be done before creating any interface otherwise
    // alljoyn will never be able to signal change of value of an exposed
    // property
    VerifyCOVSupport();

    for (auto tempProperty : m_device->Properties())
    {
        PropertyInterface *propertyInterface = nullptr;

        // create device property
        deviceProperty = new (std::nothrow) DeviceProperty();
        if (nullptr == deviceProperty)
        {
            status = ER_OUT_OF_MEMORY;
            goto leave;
        }

        // get the interface
        status = GetInterfaceProperty(tempProperty, &propertyInterface);
        if (ER_OK != status)
        {
            goto leave;
        }

        status = deviceProperty->Initialize(tempProperty, propertyInterface, this->shared_from_this());
        if (ER_OK != status)
        {
            goto leave;
        }
        m_deviceProperties.insert(std::make_pair(*deviceProperty->GetPathName(), deviceProperty));
        m_about.AddObject(deviceProperty->GetBusObject(), deviceProperty->GetPropertyInterface()->GetInterfaceDescription());

        deviceProperty = nullptr;
    }

leave:
    if (ER_OK != status &&
        nullptr != deviceProperty)
    {
        delete deviceProperty;
    }
    return status;
}

bool BridgeDevice::IsBusObjectPathUnique(std::string &path)
{
    auto index = m_deviceProperties.find(path);
    if (m_deviceProperties.end() == index)
    {
        return true;
    }
    else
    {
        return false;
    }
}

QStatus BridgeDevice::InitializeAllJoyn()
{
    QStatus status = ER_OK;
    alljoyn_buslistener_callbacks busListenerCallbacks = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    //ConfigManager *configManager = nullptr;
    string appName;

    // verify if already connected
    if (nullptr != m_AJBusAttachment)
    {
        goto leave;
    }

    // create the bus attachment
    AllJoynHelper::EncodeStringForAppName(m_adapter->ExposedApplicationName(), appName);
    m_AJBusAttachment = alljoyn_busattachment_create(appName.c_str(), QCC_TRUE);
    if (nullptr == m_AJBusAttachment)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    // create the bus listener
    m_AJBusListener = alljoyn_buslistener_create(&busListenerCallbacks, nullptr);
    if (nullptr == m_AJBusListener)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    // introduce the bus attachment and the listener
    alljoyn_busattachment_registerbuslistener(m_AJBusAttachment, m_AJBusListener);

    // start the bus attachment
    status = alljoyn_busattachment_start(m_AJBusAttachment);
    if (ER_OK != status)
    {
        goto leave;
    }

    // set up authentication if necessary
    //configManager = DsbBridge::SingleInstance()->GetConfigManager();
    //if (configManager->IsDeviceAccessSecured())
    //{
    //    status = m_authHandler.InitializeWithAllAuthenticationMethods(m_AJBusAttachment,
    //        configManager->GetBridgeConfig()->DeviceKeyX(),
    //        configManager->GetBridgeConfig()->DeviceUsername(),
    //        configManager->GetBridgeConfig()->DevicePassword(),
    //        configManager->GetBridgeConfig()->DeviceEcdheEcdsaPrivateKey(),
    //        configManager->GetBridgeConfig()->DeviceEcdheEcdsaCertChain());
    //    if (ER_OK != status)
    //    {
    //        goto leave;
    //    }
    //}

leave:
    return status;
}

QStatus BridgeDevice::ConnectToAllJoyn()
{
    QStatus status = ER_OK;
    alljoyn_sessionportlistener_callbacks sessionPortListenerCallbacks = { BridgeDevice::AcceptSessionJoinerCallback, (alljoyn_sessionportlistener_sessionjoined_ptr) BridgeDevice::SessionJoined };
    alljoyn_sessionlistener_callbacks sessionListenerCallbacks = { nullptr, nullptr, (alljoyn_sessionlistener_sessionmemberremoved_ptr) BridgeDevice::MemberRemoved };
    alljoyn_sessionopts opts = nullptr;
    alljoyn_sessionport sp = DSB_SERVICE_PORT;

    // connect the bus attachment
    status = alljoyn_busattachment_connect(m_AJBusAttachment, nullptr);
    if (ER_OK != status)
    {
        goto Leave;
    }

    /*
    * Advertise this service on the bus.
    * There are three steps to advertising this service on the bus.
    * 1) Request a well-known name that will be used by the client to discover
    *    this service.
    * 2) Create a session.
    * 3) Advertise the well-known name.
    */
    status = alljoyn_busattachment_requestname(m_AJBusAttachment, m_ServiceName.c_str(), DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
    if (ER_OK != status)
    {
        goto Leave;
    }

    // callback will get this class as context
    m_AJsessionListener = alljoyn_sessionlistener_create(&sessionListenerCallbacks, (const void *) this);
    if (nullptr == m_AJsessionListener)
    {
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    m_AJSessionPortListener = alljoyn_sessionportlistener_create(&sessionPortListenerCallbacks, (const void *) this);
    if (nullptr == m_AJSessionPortListener)
    {
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    opts = alljoyn_sessionopts_create(ALLJOYN_TRAFFIC_TYPE_MESSAGES, QCC_TRUE, ALLJOYN_PROXIMITY_ANY, ALLJOYN_TRANSPORT_ANY);
    if (nullptr == opts)
    {
        status = ER_OUT_OF_MEMORY;
        goto Leave;
    }

    status = alljoyn_busattachment_bindsessionport(m_AJBusAttachment, &sp, opts, m_AJSessionPortListener);
    if (ER_OK != status)
    {
        goto Leave;
    }

    status = alljoyn_busattachment_advertisename(m_AJBusAttachment, m_ServiceName.c_str(), alljoyn_sessionopts_get_transports(opts));
    if (ER_OK != status)
    {
        goto Leave;
    }

Leave:
    if (nullptr != opts)
    {
        alljoyn_sessionopts_destroy(opts);
    }
    return status;
}

void BridgeDevice::ShutdownAllJoyn()
{
    if (nullptr != m_AJBusAttachment)
    {
        // cancel advertised name and session port binding
        if (!m_ServiceName.empty())
        {
            alljoyn_busattachment_canceladvertisename(m_AJBusAttachment, m_ServiceName.c_str(), ALLJOYN_TRANSPORT_ANY);
        }
        alljoyn_busattachment_unbindsessionport(m_AJBusAttachment, DSB_SERVICE_PORT);

        if (!m_ServiceName.empty())
        {
            // Release the service name
            alljoyn_busattachment_releasename(m_AJBusAttachment, m_ServiceName.c_str());
        }

        alljoyn_busattachment_disconnect(m_AJBusAttachment, nullptr);

        // remove authentication handler
        m_authHandler.ShutDown();

        alljoyn_busattachment_stop(m_AJBusAttachment);

        // destroy bus attachment
        alljoyn_busattachment_destroy(m_AJBusAttachment);
        m_AJBusAttachment = nullptr;
    }

    // destroy bus listener and session port listener
    if (nullptr != m_AJBusListener)
    {
        alljoyn_buslistener_destroy(m_AJBusListener);
        m_AJBusListener = nullptr;
    }

    if (nullptr != m_AJSessionPortListener)
    {
        alljoyn_sessionportlistener_destroy(m_AJSessionPortListener);
        m_AJSessionPortListener = nullptr;
    }



}

PropertyInterface *BridgeDevice::FindMatchingInterfaceProperty(string & interfaceName)
{
    PropertyInterface *propertyInterface = nullptr;

    for (auto tempInterface : m_propertyInterfaces)
    {
        if ((*tempInterface->GetInterfaceName()) == interfaceName)
        {
            propertyInterface = tempInterface;
            break;
        }
    }
    return propertyInterface;
}

PropertyInterface *BridgeDevice::FindMatchingInterfaceProperty(std::shared_ptr<IAdapterProperty> adapterProperty)
{
    PropertyInterface *propertyInterface = nullptr;

    for (auto tempInterface : m_propertyInterfaces)
    {
        if (tempInterface->InterfaceMatchWithAdapterProperty(adapterProperty))
        {
            propertyInterface = tempInterface;
            break;
        }
    }
    return propertyInterface;
}

QStatus BridgeDevice::CreateInterfaceProperty(std::shared_ptr<IAdapterProperty> adapterProperty, string & interfaceName, PropertyInterface **propertyInterface)
{
    QStatus status = ER_OK;
    PropertyInterface *newInterface = nullptr;

    *propertyInterface = nullptr;

    // create new interface
    newInterface = new(std::nothrow) PropertyInterface();
    if (nullptr == newInterface)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    status = newInterface->Create(this->m_adapter, adapterProperty, interfaceName, this->shared_from_this());
    if (ER_OK != status)
    {
        goto leave;
    }

    // add new interface in interface list
    m_propertyInterfaces.push_back(newInterface);
    *propertyInterface = newInterface;

leave:
    if (ER_OK != status &&
        nullptr != newInterface)
    {
        delete newInterface;
    }
    return status;
}

QStatus BridgeDevice::GetInterfaceProperty(std::shared_ptr<IAdapterProperty> adapterProperty, PropertyInterface **propertyInterface)
{
    string interfaceName;
    std::ostringstream uniqueIdForInterface;
    std::string tempString;
    bool useHint = false;
    QStatus status = ER_OK;

    //add hint name if hint is provided, else use default interface name generation
    AllJoynHelper::EncodeStringForInterfaceName(adapterProperty->InterfaceHint(), tempString);
    if (!tempString.empty())
    {
        interfaceName = tempString;
        useHint = true;
    }
    else
    {
        // interface name is made of service name plus a unique Id
        uniqueIdForInterface << GetUniqueIdForInterface();

        interfaceName = m_RootStringForAllJoynNames;

        //add device name
        AllJoynHelper::EncodeStringForServiceName(m_device->Name(), tempString);
        if (!tempString.empty())
        {
            interfaceName += ".";
            interfaceName += tempString;
        }

        interfaceName += BASE_INTERFACE_NAME;
        interfaceName += uniqueIdForInterface.str();
    }

    //Find if the property interface already exist
    if (useHint)
    {
        *propertyInterface = FindMatchingInterfaceProperty(interfaceName);
    }
    else
    {
        *propertyInterface = FindMatchingInterfaceProperty(adapterProperty);
    }

    if (nullptr == *propertyInterface)
    {
        //interface doesn't exist, create one
        status = CreateInterfaceProperty(adapterProperty, interfaceName, propertyInterface);
    }

    return status;
}

bool Bridge::BridgeDevice::IsEqual(std::shared_ptr<IAdapterDevice>  device)
{
    // consider that adapter device are equal if their respective serial number are equal
    if (m_device->SerialNumber() == device->SerialNumber())
    {
        return true;
    }
    else
    {
        return false;
    }
}

QCC_BOOL AJ_CALL BridgeDevice::AcceptSessionJoinerCallback(const void * /*context*/, alljoyn_sessionport sessionPort, const char * /*joiner*/, const alljoyn_sessionopts /*opts*/)
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

void AJ_CALL BridgeDevice::SessionJoined(void * context, alljoyn_sessionport /*sessionPort*/, alljoyn_sessionid id, const char * /*joiner*/)
{
    QStatus status = ER_OK;
    BridgeDevice* bridgeDevice= nullptr;
    uint32_t timeOut = SESSION_LINK_TIMEOUT;

    bridgeDevice = reinterpret_cast<BridgeDevice *> (context);
    if (nullptr == bridgeDevice)
    {
        goto leave;
    }

    // Enable concurrent callbacks since some of the calls below could block
    alljoyn_busattachment_enableconcurrentcallbacks(bridgeDevice->m_AJBusAttachment);

    // set session listener and time-out
    status = alljoyn_busattachment_setsessionlistener(bridgeDevice->m_AJBusAttachment, id, bridgeDevice->m_AJsessionListener);
    if (status != ER_OK)
    {
        goto leave;
    }
    status = alljoyn_busattachment_setlinktimeout(bridgeDevice->m_AJBusAttachment, id, &timeOut);
    if (status != ER_OK)
    {
        goto leave;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(DsbBridge::SingleInstance()->GetLock());
        bridgeDevice->m_activeSessions.push_back(id);
    }

leave:
    return;
}

void AJ_CALL BridgeDevice::MemberRemoved(void * context, alljoyn_sessionid sessionid, const char * uniqueName)
{
    BridgeDevice* bridgeDevice = nullptr;

    // get config manager instance
	bridgeDevice = reinterpret_cast<BridgeDevice *> (context);
	if (nullptr == bridgeDevice)
    {
        goto leave;
    }

    // reset access
    bridgeDevice->m_authHandler.ResetAccess(uniqueName);

    {
        std::lock_guard<std::recursive_mutex> lock(DsbBridge::SingleInstance()->GetLock());
        auto iter = std::find(bridgeDevice->m_activeSessions.begin(), bridgeDevice->m_activeSessions.end(), sessionid);

        if (iter != bridgeDevice->m_activeSessions.end())
        {
            bridgeDevice->m_activeSessions.erase(iter);
        }
    }

leave:
    return;
}

QStatus BridgeDevice::RegisterSignalHandlers(bool isRegister)
{
    std::lock_guard<std::recursive_mutex> lock(DsbBridge::SingleInstance()->GetLock());

    for (auto signal : m_device->Signals())
    {
        if (isRegister)
        {
            // silently failing here is OK because it could happen that adapter device
            // have some signal in its list and still to register listener
            std::string name = this->m_adapter->AdapterName();
            this->m_adapter->RegisterSignalListener(signal, this->shared_from_this(), 0);
        }
        else
        {
            this->m_adapter->UnregisterSignalListener(signal, this->shared_from_this());
        }
    }

    return ER_OK;
}

void BridgeDevice::VerifyCOVSupport()
{
    std::lock_guard<std::recursive_mutex> lock(DsbBridge::SingleInstance()->GetLock());

    // default to no change of value signal (aka COV)
    m_supportCOVSignal = false;

    // go through signals, look for COV and verify it is supported
    for (auto signal : m_device->Signals())
    {
        if (signal->Name() == Constants::CHANGE_OF_VALUE_SIGNAL())
        {
            m_supportCOVSignal = true;
        }
    }
}

void BridgeDevice::AdapterSignalHandler(std::shared_ptr<IAdapter> /*sender*/, std::shared_ptr<IAdapterSignal> signal, intptr_t /*context*/)
{
    if (signal->Name() == Constants::CHANGE_OF_VALUE_SIGNAL())
    {
        HandleCOVSignal(signal);
    }
    else
    {
        m_deviceMain->HandleSignal(signal);
    }
}

void BridgeDevice::HandleCOVSignal(std::shared_ptr<IAdapterSignal>  signal)
{
    std::shared_ptr<IAdapterProperty> adapterProperty = nullptr;
    std::shared_ptr<IAdapterValue> newValue = nullptr;

    // get present value and property name from the signal
    for (auto param : signal->Params())
    {
        if (param->Name() == Constants::COV__PROPERTY_HANDLE())
        {
            adapterProperty = (param->Data()).Get<std::shared_ptr<IAdapterProperty>>();
        }
        else if (param->Name() == Constants::COV__ATTRIBUTE_HANDLE())
        {
            newValue = (param->Data()).Get<std::shared_ptr<IAdapterValue>>();
        }
    }

    if (adapterProperty != nullptr && newValue != nullptr)
    {
        // get the property that has changed
        for (auto val : m_deviceProperties)
        {
            if (val.second->GetAdapterProperty() == adapterProperty)
            {
                val.second->EmitSignalCOV(newValue, m_activeSessions);
                break;
            }
        }
    }
}

std::shared_ptr<IAdapterProperty> BridgeDevice::GetAdapterProperty(_In_ std::string busObjectPath)
{
    std::shared_ptr<IAdapterProperty> adapterProperty = nullptr;

    // find IAdapterProperty from its exposed bus object path
    auto val = m_deviceProperties.find(busObjectPath.c_str());
    if (val != m_deviceProperties.end())
    {
        adapterProperty = val->second->GetAdapterProperty();
    }

    return adapterProperty;
}

std::string BridgeDevice::GetBusObjectPath(_In_ std::shared_ptr<IAdapterProperty> adapterProperty)
{
    std::string busObjectPath;

    // find exposed bus object path from IAdapterProperty
    for (auto val : m_deviceProperties)
    {
        if (val.second->GetAdapterProperty() == adapterProperty)
        {
            busObjectPath = *(val.second->GetPathName());
        }
    }
    return busObjectPath;
}

QStatus BridgeDevice::InitControlPanel()
{
    const wchar_t CONTROL_PANEL_NAME[] = L"Simple";
    QStatus status = ER_OK;

    IAdapterDeviceControlPanel* controlPanelDevice = dynamic_cast<IAdapterDeviceControlPanel*>(m_device.get());
    if (controlPanelDevice == nullptr)
    {
        // Continue without initialization
        goto leave;
    }

    if (controlPanelDevice->ControlPanelHandler() != nullptr)
    {
        auto controlPanelHandler = controlPanelDevice->ControlPanelHandler();
		IControlPanelHandler* cph = controlPanelHandler.get();
        if (dynamic_cast<IControlPanelHandlerSimple*>(cph) != nullptr)
        {
            m_pControlPanel = new (std::nothrow) ControlPanelSimple(controlPanelHandler, m_device);
            if (m_pControlPanel == nullptr)
            {
                status = ER_OUT_OF_MEMORY;
                goto leave;
            }

            // If Control Panel Initialization fails, proceed anyway.  This should not be a blocker
			std::wstring wname = StringUtils::ToWstring(m_device->Name());
            status = m_pControlPanel->Initialize(this->m_adapter, GetBusAttachment(), wname.c_str(), CONTROL_PANEL_NAME);
            if (status != ER_OK)
            {
                delete m_pControlPanel;
                m_pControlPanel = nullptr;
                status = ER_OK;
            }
        }
        else if (dynamic_cast<IControlPanelHandlerUniversal*>(cph) != nullptr)
        {
			m_pControlPanel = new (std::nothrow) ControlPanelUniversal(controlPanelHandler, m_device);
            if (m_pControlPanel == nullptr)
            {
				status = ER_OUT_OF_MEMORY;
				goto leave;
            }

            // If Control Panel Initialization fails, proceed anyway.  This should not be a blocker
			std::wstring wname = StringUtils::ToWstring(m_device->Name());
			status = m_pControlPanel->Initialize(this->m_adapter, GetBusAttachment(), wname.c_str(), CONTROL_PANEL_NAME);
            if (status != ER_OK)
            {
                delete m_pControlPanel;
                m_pControlPanel = nullptr;
                status = ER_OK;
            }
        }
    }

leave:

    return status;
}

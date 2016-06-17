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
#include "Bridge.h"
#include "DeviceMain.h"
#include "DeviceMethod.h"
#include "DeviceSignal.h"
#include "AllJoynHelper.h"
#include "AdapterDevice.h"

using namespace Bridge;

static const std::string INTERFACE_NAME_FOR_MAIN_DEVICE = ".MainInterface";

DeviceMain::DeviceMain()
    : m_AJBusObject(nullptr),
    m_interfaceDescription(nullptr),
    m_parent(nullptr),
    m_registeredOnAllJoyn(false),
    m_indexForMethod(1),
    m_indexForSignal(1)
{
}

DeviceMain::~DeviceMain()
{
    Shutdown();
}

QStatus AJ_CALL DeviceMain::GetPropertyHandler(_In_ const void* context, _In_z_ const char* ifcName, _In_z_ const char* propName, _Out_ alljoyn_msgarg val)
{
    QStatus status = ER_OK;
    DeviceMain *device = nullptr;

    device = (DeviceMain *)context;
    if (nullptr == device)	// sanity test
    {
        return ER_BAD_ARG_1;
    }

    status = device->GetPropertyValue(ifcName, propName, val);

    return status;
}

QStatus AJ_CALL DeviceMain::SetPropertyHandler(_In_ const void* context, _In_z_ const char* ifcName, _In_z_ const char* propName, _Out_ alljoyn_msgarg val)
{
    QStatus status = ER_OK;
    DeviceMain *device = nullptr;

    device = (DeviceMain *)context;
    if (nullptr == device)	// sanity test
    {
        return ER_BAD_ARG_1;
    }

    status = device->SetPropertyValue(ifcName, propName, val);

    return status;
}

QStatus DeviceMain::GetPropertyValue(_In_z_ const char* /*ifcName*/, _In_z_ const char* propName, _Out_ alljoyn_msgarg val)
{
    QStatus status = ER_OK;

	std::shared_ptr<IAdapterIoRequest> request = nullptr;
	std::string sName(propName);

    std::shared_ptr<IAdapterProperty> iap( dynamic_cast<IAdapterProperty*>( new AdapterProperty(sName, sName)));

    auto signatureIterator = m_devicePropertySignatures.find(propName);
    if (signatureIterator == m_devicePropertySignatures.end())
    {
        return ER_NOT_IMPLEMENTED;
    }
    std::string signature = signatureIterator->second;
    std::shared_ptr<IAdapterValue> value = AllJoynHelper::GetDefaultAdapterValue(signature.c_str());

    unsigned int adapterStatus = GetAdapter()->GetPropertyValue(m_parent->GetAdapterDevice(), iap, sName, value, request);
    if (ERROR_IO_PENDING == adapterStatus &&
        nullptr != request)
    {
        // wait for completion
        adapterStatus = request->Wait(WAIT_TIMEOUT_FOR_ADAPTER_OPERATION);
    }
    if ((ERROR_SUCCESS == adapterStatus) && (value != nullptr))
    {
        status = AllJoynHelper::SetMsgArg(value, val);
    }
    else
    {
        status = ER_FAIL;
    }
    return status;
}

QStatus DeviceMain::SetPropertyValue(_In_z_ const char* /*ifcName*/, _In_z_ const char* propName, _Out_ alljoyn_msgarg val)
{
    QStatus status = ER_OK;

	std::shared_ptr<IAdapterIoRequest> request = nullptr;
	std::string sName(propName);

    std::shared_ptr<IAdapterValue> value = nullptr;
    std::shared_ptr<IAdapterProperty> iap(dynamic_cast<IAdapterProperty*>(new AdapterProperty(sName, sName)));

    // Get the value and convert it to an IAdapterValue

    status = AllJoynHelper::GetAdapterValueFromMsgArg(val, value);
    if (ER_OK == status)
    {
        unsigned int adapterStatus = GetAdapter()->SetPropertyValue(m_parent->GetAdapterDevice(), iap, value, request);
        if (ERROR_IO_PENDING == adapterStatus &&
            nullptr != request)
        {
            // wait for completion
            adapterStatus = request->Wait(WAIT_TIMEOUT_FOR_ADAPTER_OPERATION);
        }
    }

    return status;
}

QStatus DeviceMain::Initialize(std::shared_ptr<BridgeDevice> parent)
{
    QStatus status = ER_OK;
    std::string tempString;
    alljoyn_busobject_callbacks callbacks =
    {
        DeviceMain::GetPropertyHandler,
        DeviceMain::SetPropertyHandler,
        nullptr,
        nullptr
    };


    // sanity check
    if (nullptr == parent)
    {
        status = ER_BAD_ARG_1;
        goto leave;
    }

    m_parent = parent;

    // build bus object path
    AllJoynHelper::EncodeBusObjectName(parent->GetAdapterDevice()->Name(), tempString);
    m_busObjectPath = "/" + tempString;

    // create alljoyn bus object and register it
    m_AJBusObject = alljoyn_busobject_create(m_busObjectPath.c_str(), QCC_FALSE, &callbacks, (void*)this);
    if (nullptr == m_AJBusObject)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    if (status == ER_OK)
    {
        // alternatively, we can create the interface from XML.
        size_t cStandardInterfaces = 0;
        cStandardInterfaces = alljoyn_busattachment_getinterfaces(parent->GetBusAttachment(), nullptr, 0);
        {
            // #CreateAJinterfaces
            // Replaced with creation of interface from XML
            std::string alljoynXml = m_parent->GetAdapterDevice()->BaseTypeXML();
            if (alljoynXml.length() > 0)
            {
				std::string xmlStr(alljoynXml);
                status = alljoyn_busattachment_createinterfacesfromxml(parent->GetBusAttachment(), xmlStr.c_str());
            }
        }

        // #RegisterAJMethodHandlers on custom interfaces
        // We need to loop through the interface methods and register AJMethod as their handler
        size_t cInterfaces = 0;
        cInterfaces = alljoyn_busattachment_getinterfaces(parent->GetBusAttachment(), nullptr, 0);
        if (cInterfaces > 0)
        {
            alljoyn_interfacedescription* interfaces = new alljoyn_interfacedescription[cInterfaces];
            if (alljoyn_busattachment_getinterfaces(parent->GetBusAttachment(), interfaces, cInterfaces) > 0)
            {
                // Loop through each custom interface we created from XML.
                // Note: we're using a trick here: we know how many standard interfaces existed before we
                // created interfaces from XML, and we know that in current implementations these interfaces are
                // returned first. This could change at any time, in which case we rally should match interfaces
                // by name instead. We can do this by having the IAdapterDevice return the XML and the name for the XML
                for (size_t curInterface = 0; curInterface < (cInterfaces - cStandardInterfaces); curInterface++)
                {
                    // Get the name. Ignore known interfaces.
                    // const char* szName = alljoyn_interfacedescription_getname(interfaces[curInterface]);

                    // Activate this interface
                    alljoyn_interfacedescription_activate(interfaces[curInterface]);

                    // add interface to bus object and expose it
                    status = alljoyn_busobject_addinterface_announced(m_AJBusObject, interfaces[curInterface]);
                    if (status == ER_OK)
                    {
                        size_t cMembers = alljoyn_interfacedescription_getmembers(interfaces[curInterface], nullptr, 0);
                        if (cMembers > 0)
                        {
                            alljoyn_interfacedescription_member* members = new alljoyn_interfacedescription_member[cMembers];
                            if (alljoyn_interfacedescription_getmembers(interfaces[curInterface], members, cMembers) > 0)
                            {
								std::shared_ptr<Bridge::AdapterSignalVector> signals(new Bridge::AdapterSignalVector);

                                // For each interface member, find the methods and register AJMethod as the handler
                                for (size_t curMember = 0; curMember < cMembers; curMember++)
                                {

                                    alljoyn_interfacedescription_member& member = members[curMember];
                                    if (member.memberType == ALLJOYN_MESSAGE_METHOD_CALL)
                                    {
                                        status = alljoyn_busobject_addmethodhandler(m_AJBusObject, member, AJMethod, nullptr);

										std::string methodName(member.name);

                                        DeviceMethod* method = new(std::nothrow) DeviceMethod();
                                        if (nullptr == method)
                                        {
                                            status = ER_OUT_OF_MEMORY;
                                            goto leave;
                                        }

										std::shared_ptr<IAdapterMethod> adapterMethod = nullptr;
                                        // See if user provided the method interface already...
                                        auto methods = m_parent->GetAdapterDevice()->Methods();
                                        for (size_t i = 0, n = methods.size(); i < n; i++)
                                        {
											auto imethod = methods[i];
                                            if (imethod->Name() == methodName)
                                            {
												adapterMethod = imethod;
                                                break;
                                            }
                                        }

                                        if (adapterMethod == nullptr)
                                        {
                                            adapterMethod = std::shared_ptr<AdapterMethod>(new AdapterMethod(methodName));
                                        }

                                        // add input params
                                        // TODO: add member.argNames into IAdapterValue::Name fields...
										AdapterMethod* adapt = dynamic_cast<AdapterMethod*>(adapterMethod.get());
										if (adapt != nullptr)
										{
											for (const char* sig = member.signature; *sig != 0; sig++)
											{
												std::shared_ptr<IAdapterValue> arg = AllJoynHelper::GetDefaultAdapterValue(sig);
												adapt->AddInputParam(arg);
												if (*sig == 'a')
												{
													sig++;
												}
											}

											// add out params
											for (const char* sig = member.returnSignature; *sig != 0; sig++)
											{
												std::shared_ptr<IAdapterValue> arg = AllJoynHelper::GetDefaultAdapterValue(sig);
												adapt->AddOutputParam(arg);
												if (*sig == 'a')
												{
													sig++;
												}
											}
										}
                                        method->Initialize(parent->GetDeviceMainObject(), adapterMethod, false);

                                        m_deviceMethods.insert(std::make_pair(method->GetName(), method));
                                        method = nullptr;
                                    }
                                    else if (member.memberType == ALLJOYN_MESSAGE_SIGNAL)
                                    {
                                        // Create adapter object for this signal
										std::string signalName(member.name);

										std::shared_ptr<Bridge::IAdapterSignal> adapterSignal = std::shared_ptr<IAdapterSignal>(new AdapterSignal(signalName));

                                        signals->push_back(adapterSignal);

                                        // Create adapter object for this signal
                                        DeviceSignal* signal = new(std::nothrow) DeviceSignal();
                                        if (nullptr == signal)
                                        {
                                            status = ER_OUT_OF_MEMORY;
                                            goto leave;
                                        }

                                        status = signal->Initialize(this, adapterSignal, member);
                                        if (ER_OK != status)
                                        {
                                            goto leave;
                                        }

                                        m_deviceSignals.insert(std::make_pair(adapterSignal->Name(), signal));
                                        signal = nullptr;
                                    }
                                }
                                // Insert signals into the adapter if they exist
                                if (signals->size() > 0)
                                {
                                    parent->GetAdapterDevice()->Signals(*(signals.get()));
                                }
                            }
                            delete[] members;
                        }

                        size_t cProps = alljoyn_interfacedescription_getproperties(interfaces[curInterface], nullptr, 0);
                        if (cProps > 0)
                        {
                            alljoyn_interfacedescription_property* properties = new alljoyn_interfacedescription_property[cProps];
                            if (alljoyn_interfacedescription_getproperties(interfaces[curInterface], properties, cProps) > 0)
                            {
                                for (size_t curProp = 0; curProp < cProps; curProp++)
                                {
                                    alljoyn_interfacedescription_property& property = properties[curProp];
                                    m_devicePropertySignatures.insert(std::make_pair(property.name, property.signature));
                                }
                            }
                            delete[] properties;
                        }
                    }
                }
            }
            delete[] interfaces;
        }
    }

    // JS developer can call also CreateMethod and add the methods to the device manually.
    //status = CreateMethodsAndSignals();
    //if (ER_OK != status)
    //{
    //    goto leave;
    //}

    // expose bus object
    status = alljoyn_busattachment_registerbusobject(parent->GetBusAttachment(), m_AJBusObject);
    if (ER_OK != status)
    {
        goto leave;
    }
    m_registeredOnAllJoyn = true;

leave:
    return status;
}

void DeviceMain::Shutdown()
{
    // clear methods and signals
    for (auto val : m_deviceMethods)
    {
        delete val.second;
    }
    m_deviceMethods.clear();

    for (auto val : m_deviceSignals)
    {
        delete val.second;
    }
    m_deviceSignals.clear();

    // shutdown AllJoyn Bus object
    if (nullptr != m_AJBusObject)
    {
        if (m_registeredOnAllJoyn)
        {
            // unregister bus object
            alljoyn_busattachment_unregisterbusobject(m_parent->GetBusAttachment(), m_AJBusObject);
            m_registeredOnAllJoyn = false;
        }
        alljoyn_busobject_destroy(m_AJBusObject);
        m_AJBusObject = nullptr;
    }

    // final clean up
    m_interfaceName.clear();
    m_busObjectPath.clear();
    m_parent = nullptr;
    m_indexForMethod = 1;
    m_indexForSignal = 1;
}

QStatus DeviceMain::CreateMethodsAndSignals()
{
    QStatus status = ER_OK;
    DeviceMethod *method = nullptr;
    DeviceSignal *signal = nullptr;
    std::string tempString;

    if ((m_parent != nullptr &&
		m_parent->GetAdapterDevice()->Methods().size() > 0) ||
        (m_parent->GetAdapterDevice()->Signals().size() > 0))
    {
        // 1st create the interface
        m_interfaceName = m_parent->GetRootNameForInterface();

        //add device name
        AllJoynHelper::EncodeStringForServiceName(m_parent->GetAdapterDevice()->Name(), tempString);
        if (!tempString.empty())
        {
            m_interfaceName += ".";
            m_interfaceName += tempString;
        }

        m_interfaceName += INTERFACE_NAME_FOR_MAIN_DEVICE;

        // create interface
        if (DsbBridge::SingleInstance()->GetConfigManager(this->GetAdapter())->IsDeviceAccessSecured())
        {
            status = alljoyn_busattachment_createinterface_secure(m_parent->GetBusAttachment(), m_interfaceName.c_str(), &m_interfaceDescription, AJ_IFC_SECURITY_REQUIRED);
        }
        else
        {
            status = alljoyn_busattachment_createinterface(m_parent->GetBusAttachment(), m_interfaceName.c_str(), &m_interfaceDescription);
        }
        if (ER_OK != status)
        {
            return status;
        }

		std::shared_ptr<IAdapterDevice> adapter = m_parent->GetAdapterDevice();
        DebugLogInfo("Creating interface for adapter [%s]", adapter->Name().c_str());

        {
            for (auto adapterMethod : m_parent->GetAdapterDevice()->Methods())
            {
                // only create method if it hasn't already been defined

                if (IsMethodNameUnique(adapterMethod->Name()))
                {
                    DebugLogInfo("Creating method [%s]", adapterMethod->Name().c_str());

                    method = new(std::nothrow) DeviceMethod();
                    if (nullptr == method)
                    {
                        status = ER_OUT_OF_MEMORY;
                        goto leave;
                    }

                    status = method->Initialize(this, adapterMethod, true);
                    if (ER_OK != status)
                    {
                        goto leave;
                    }

                    m_deviceMethods.insert(std::make_pair(method->GetName(), method));
                    method = nullptr;
                }
            }
        }

        // activate the interface
        alljoyn_interfacedescription_activate(m_interfaceDescription);

        // add interface to bus object and expose it
        status = alljoyn_busobject_addinterface_announced(m_AJBusObject, m_interfaceDescription);

        // add method handlers
        if (m_deviceMethods.size() > 0)
        {
            for (auto methodIterator = m_deviceMethods.begin(); methodIterator != m_deviceMethods.end(); methodIterator++)
            {
                // add method handler
                alljoyn_interfacedescription_member member = { 0 };
                QCC_BOOL found = false;

                // get adapter method to invoke
                method = methodIterator->second;

                found = alljoyn_interfacedescription_getmember(m_interfaceDescription, method->GetName().c_str(), &member);
                if (!found)
                {
                    status = ER_INVALID_DATA;
                    goto leave;
                }

                status = alljoyn_busobject_addmethodhandler(m_AJBusObject, member, AJMethod, nullptr);
                if (ER_OK != status)
                {
                    goto leave;
                }
            }
        }
    }

leave:
    if (nullptr != method)
    {
        delete method;
    }
    if (nullptr != signal)
    {
        delete signal;
    }

    return status;
}

bool DeviceMain::IsMethodNameUnique(std::string name)
{
    // verify there is no method with same name
    auto methodIterator = m_deviceMethods.find(name);
    if (methodIterator == m_deviceMethods.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool DeviceMain::IsSignalNameUnique(std::string name)
{
    bool retVal = true;

    // verify there is no signal with same name
    for (auto tempSignal : m_deviceSignals)
    {
        if (tempSignal.second->GetName() == name)
        {
            retVal = false;
            break;
        }
    }

    return retVal;
}

DeviceMain *DeviceMain::GetInstance(_In_ alljoyn_busobject busObject)
{
    // sanity check
    if (nullptr == busObject)
    {
        return nullptr;
    }

    // find out the DeviceMain instance that correspond to the alljoyn bus object
    DeviceMain *objectPointer = nullptr;
    auto deviceList = DsbBridge::SingleInstance()->GetDeviceList();
    for (auto device : deviceList)
    {
        objectPointer = device.second->GetDeviceMainObject();
        if (objectPointer != nullptr &&
            objectPointer->GetBusObject() == busObject)
        {
            break;
        }
        else
        {
            objectPointer = nullptr;
        }
    }

    return objectPointer;
}

//#AJMethodCall
// Maps a call received from AllJoyn bus object to a call into the Chakra Host.
// NOTE: This likely will execute on a different thread. Really, we should have the host spawn a new, dedicated thread and schedule to it,
// but for now, we simply assign this thread to the host.
void DeviceMain::AJMethod(_In_ alljoyn_busobject busObject, _In_ const alljoyn_interfacedescription_member* member, _In_ alljoyn_message msg)
{
    QStatus status = ER_OK;
    uint32_t adapterStatus = ERROR_SUCCESS;

    std::map<std::string, DeviceMethod *>::iterator methodIterator;
    DeviceMethod *deviceMehod = nullptr;
    alljoyn_msgarg outArgs = nullptr;
    size_t nbOfArgs = 0;

    // get instance of device main from bus object
    DeviceMain *deviceMain = DeviceMain::GetInstance(busObject);
    if (nullptr == deviceMain)
    {
        status = ER_OS_ERROR;
        goto leave;
    }

    // get adapter method to invoke
    methodIterator = deviceMain->m_deviceMethods.find(member->name);
    if (methodIterator == deviceMain->m_deviceMethods.end())
    {
        status = ER_NOT_IMPLEMENTED;
        goto leave;
    }
    deviceMehod = methodIterator->second;

    // invoke method
    adapterStatus = methodIterator->second->InvokeMethod(msg, &outArgs, &nbOfArgs);
    if (ERROR_SUCCESS != adapterStatus)
    {
        status = ER_OS_ERROR;
        goto leave;
    }

leave:
    if (ER_OK != status)
    {
        alljoyn_busobject_methodreply_status(busObject, msg, status);
    }
    else if (0 == nbOfArgs)
    {
        alljoyn_busobject_methodreply_args(busObject, msg, nullptr, 0);
    }
    else
    {
        alljoyn_busobject_methodreply_args(busObject, msg, outArgs, nbOfArgs);
    }
    if (nullptr != outArgs)
    {
        alljoyn_msgarg_destroy(outArgs);
    }
}

void DeviceMain::HandleSignal(std::shared_ptr<IAdapterSignal> adapterSignal)
{
    // sanity check
    if (nullptr == adapterSignal)
    {
        return;
    }

    // get corresponding signal class instance
    auto signal = m_deviceSignals.find(adapterSignal->Name());
    if (m_deviceSignals.end() == signal)
    {
        // unknown IAdapterSignal
        return;
    }

    // send signal to alljoyn
    signal->second->SendSignal(adapterSignal);
}

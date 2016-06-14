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
#include "Bridge.h" // TODO: This should be replaced with IBridge.h
#include "AdapterDevice.h"
#include "ScriptHost.h"
#include "JsAdapter.h"
#include "JsAdapterRequest.h"

using namespace Bridge;

namespace AdapterLib
{
    std::shared_ptr<JsAdapter> JsAdapter::g_TheOneOnlyInstance = nullptr;

    std::shared_ptr<JsAdapter> JsAdapter::SingleInstance()
    {
        if (g_TheOneOnlyInstance == nullptr)
        {
            g_TheOneOnlyInstance = std::shared_ptr<JsAdapter>(new JsAdapter());
        }
        return g_TheOneOnlyInstance;
    }

    /*static*/ std::shared_ptr<IJsAdapter> IJsAdapter::Get()
    {
        return JsAdapter::SingleInstance();
    }

    static const char* APPLICATION_GUID = "1C5A7F71-CF18-482E-98F4-BD228E07DD1F";

    JsAdapter::JsAdapter()
    {
        this->vendor = "Microsoft";
        this->adapterName = "Javascript Device System Bridge";
        // the adapter prefix must be something like "com.mycompany" (only alpha num and dots)
        // it is used by the Device System Bridge as root string for all services and interfaces it exposes
        this->exposedAdapterPrefix = "com.";
        this->exposedAdapterPrefix += this->vendor.c_str();
        this->exposedApplicationGuid = APPLICATION_GUID;
        this->exposedApplicationName = "Javascript Device System Bridge";

        // TODO: Add real version information
        this->version = "0.1.0.0";

        // Create Adapter Signals
        this->CreateSignals();
    }

    JsAdapter::~JsAdapter()
    {
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::SetConfiguration(const std::vector<uint8_t>& /*ConfigurationData*/)
    {
        return ERROR_SUCCESS;
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::GetConfiguration(std::vector<uint8_t>& /*ConfigurationDataPtr*/)
    {
        return ERROR_SUCCESS;
    }

    uint32_t
    JsAdapter::Initialize()
    {
        return ERROR_SUCCESS;
    }

    uint32_t
    JsAdapter::Shutdown()
    {
        return ERROR_SUCCESS;
    }

    // Create a new IAdapterDevice for the Alljoyn interface described by the given xml, and associate
    // it with the given JavaScript and initialize the Javascript with the given context object.
    uint32_t JsAdapter::AddDevice(
        _In_ std::shared_ptr<IAdapterDevice> device,
        _In_ const std::string& baseTypeXml,
        _In_ const std::string& jsScript,
        _In_ const std::string& jsModulesPath,
        _Out_opt_ std::shared_ptr<IAdapterAsyncRequest>& requestPtr)
    {
        ScriptHost* host = new ScriptHost();

        device->BaseTypeXML(baseTypeXml);

        // Set the host context on the device, required when it processes methods, events etc.
        device->HostContext(reinterpret_cast<intptr_t>(reinterpret_cast<void*>(host)));

        auto adapterRequest = std::make_shared<JsAdapterRequest>();
        requestPtr = adapterRequest;
        host->InitializeHost(device, jsScript, jsModulesPath, [=](uint32_t status)
        {
            if (status == ERROR_SUCCESS)
            {
                std::shared_ptr<IAdapter> adapter = std::dynamic_pointer_cast<IAdapter>(g_TheOneOnlyInstance);
                std::shared_ptr<IBridge> bridge = IBridge::Get();
                QStatus bridgeStatus = bridge->AddDevice(adapter, device);
                if (bridgeStatus != ER_OK)
                {
                    std::cerr << "JsAdapter could not add device. Bridge returned status: " << bridgeStatus << std::endl;
                    status = ERROR_GEN_FAILURE;
                }
            }
            else
            {
                std::cerr << "JsAdapter could not add device. Initialization returned status: " << status << std::endl;
            }

            adapterRequest->SetStatus(status);
        });

        return adapterRequest->Status();
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::EnumDevices(
        ENUM_DEVICES_OPTIONS /*Options*/,
        AdapterDeviceVector& /*DeviceListPtr*/,
        std::shared_ptr<IAdapterIoRequest>& /*RequestPtr*/
        )
    {
        return ERROR_SUCCESS;
    }


    _Use_decl_annotations_
    uint32_t
    JsAdapter::GetProperty(
        std::shared_ptr<IAdapterProperty>& /*Property*/,
        std::shared_ptr<IAdapterIoRequest>& /*RequestPtr*/
        )
    {
        return ERROR_SUCCESS;
    }


    _Use_decl_annotations_
    uint32_t
    JsAdapter::SetProperty(
        std::shared_ptr<IAdapterProperty> /*Property*/,
        std::shared_ptr<IAdapterIoRequest>& /*RequestPtr*/
        )
    {
        return ERROR_SUCCESS;
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::GetPropertyValue(
        _In_ std::shared_ptr<IAdapterDevice> device,
        std::shared_ptr<IAdapterProperty> Property,
        std::string AttributeName,
        std::shared_ptr<IAdapterValue>& ValuePtr,
        std::shared_ptr<IAdapterIoRequest>& RequestPtr
        )
    {
        uint32_t status = ERROR_SUCCESS;

        if (RequestPtr != nullptr)
        {
            RequestPtr = nullptr;
        }
        // sanity check
        AdapterProperty* tempProperty = dynamic_cast<AdapterProperty*>(Property.get());
        if (ValuePtr == nullptr ||
            tempProperty == nullptr ||
            tempProperty->Attributes().empty())
        {
            return static_cast<uint32_t>(ER_BAD_ARG_1);
        }

        // Run the script to get that property.

        ScriptHost* host = reinterpret_cast<ScriptHost*>((void*)device->HostContext());

        auto adapterRequest = std::make_shared<JsAdapterRequest>();
        std::shared_ptr<IAdapterValue> resultValue = ValuePtr;
        RequestPtr = adapterRequest;
        host->InvokePropertyGetter(Property->Name(), resultValue, [=](uint32_t status)
        {
            if (status != ERROR_SUCCESS)
            {
                std::cerr << "GetPropertyValue failed on adapter" << std::endl;
            }

            adapterRequest->SetStatus(status);
        });

        return adapterRequest->Status();
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::SetPropertyValue(
        std::shared_ptr<IAdapterDevice> device,
        std::shared_ptr<IAdapterProperty> Property,
        std::shared_ptr<IAdapterValue> Value,
        std::shared_ptr<IAdapterIoRequest>& RequestPtr
        )
    {
        uint32_t status = ERROR_SUCCESS;
        if (RequestPtr != nullptr)
        {
            RequestPtr = nullptr;
        }

        // sanity check
        AdapterProperty* tempProperty = dynamic_cast<AdapterProperty*>(Property.get());
        AdapterValue* tempValue = dynamic_cast<AdapterValue*>(Value.get());
        if (tempValue == nullptr ||
            tempProperty == nullptr ||
            tempProperty->Attributes().empty())
        {
            return static_cast<uint32_t>(ER_BAD_ARG_1);
        }

        // find corresponding attribute
        for (auto attribute : tempProperty->Attributes())
        {
            if (attribute->Value() != nullptr &&
                attribute->Value()->Name() == tempValue->Name())
            {
                attribute->Value()->Data() = tempValue->Data();
                return ERROR_SUCCESS;
            }
        }

        // Run the script to set that property.

        AdapterValueVector paramsIn;
        AdapterValueVector paramsOut;
        ScriptHost* host = reinterpret_cast<ScriptHost*>((void*)device->HostContext());
        std::shared_ptr<IAdapterValue> sharedVal(dynamic_cast<IAdapterValue*>(tempValue));
        paramsIn.push_back(sharedVal);

        auto adapterRequest = std::make_shared<JsAdapterRequest>();
        RequestPtr = adapterRequest;
        host->InvokePropertySetter(tempProperty->Name(), Value, [=](uint32_t status)
        {
            if (status != ERROR_SUCCESS)
            {
                std::cerr << "GetPropertyValue failed on adapter" << std::endl;
            }

            adapterRequest->SetStatus(status);
        });

        return adapterRequest->Status();
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::CallMethod(
        std::shared_ptr<IAdapterDevice> device,
        std::shared_ptr<IAdapterMethod>& method,
        std::shared_ptr<IAdapterIoRequest>& requestPtr
        )
    {
        uint32_t status = 0;

        // Call the script function
        ScriptHost* host = reinterpret_cast<ScriptHost*>((void*)device->HostContext());

        std::shared_ptr<IAdapterValue> resultPtr;
        if (method->OutputParams().size() == 1)
        {
            resultPtr = method->OutputParams()[0];
        }

        auto adapterRequest = std::make_shared<JsAdapterRequest>();
        requestPtr = adapterRequest;
        host->InvokeMethod(method->Name(), method->InputParams(), resultPtr, false, [=](uint32_t status)
        {
            if (status != ERROR_SUCCESS)
            {
                std::cerr << "CallMethod failed on adapter" << std::endl;
            }

            adapterRequest->SetStatus(status);
        });

        return adapterRequest->Status();
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::RegisterSignalListener(
        std::shared_ptr<IAdapterSignal> Signal,
        std::shared_ptr<IAdapterSignalListener> Listener,
        intptr_t ListenerContext
        )
    {
        if (Signal == nullptr || Listener == nullptr)
        {
            return static_cast<uint32_t>(ER_BAD_ARG_1);
        }

        std::lock_guard<std::mutex> lock(_mutex);

        this->signalListeners.insert(
            {
                Signal->GetHashCode(),
                SIGNAL_LISTENER_ENTRY(Signal, Listener, ListenerContext)
            }
        );

        return ERROR_SUCCESS;
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::UnregisterSignalListener(
        std::shared_ptr<IAdapterSignal> /*Signal*/,
        std::shared_ptr<IAdapterSignalListener> /*Listener*/
        )
    {
        return ERROR_SUCCESS;
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::NotifySignalListener(
        std::shared_ptr<IAdapterSignal> signal
        )
    {
        if (signal == nullptr)
        {
            return static_cast<uint32_t>(ER_BAD_ARG_1);
        }

        std::lock_guard<std::mutex> lock(_mutex);

        auto handlerRange = this->signalListeners.equal_range(signal->GetHashCode());

        std::vector<std::pair<int, SIGNAL_LISTENER_ENTRY>> handlers(
            handlerRange.first,
            handlerRange.second);

        for (auto iter = handlers.begin(); iter != handlers.end(); ++iter)
        {
            IAdapterSignalListener* listener = iter->second.Listener.get();
            intptr_t listenerContext = iter->second.Context;
            std::shared_ptr<IAdapter> adapt(dynamic_cast<IAdapter*>(this));
            listener->AdapterSignalHandler(adapt, signal, listenerContext);
        }

        return ERROR_SUCCESS;
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::NotifyDeviceArrival(
        std::shared_ptr<IAdapterDevice> Device
        )
    {
        if (Device == nullptr)
        {
            return static_cast<uint32_t>(ER_BAD_ARG_1);
        }

        std::shared_ptr<IAdapterSignal> deviceArrivalSignal = this->signals.at(DEVICE_ARRIVAL_SIGNAL_INDEX);
        deviceArrivalSignal->Params().at(DEVICE_ARRIVAL_SIGNAL_PARAM_INDEX)->Data() = Device;
        this->NotifySignalListener(deviceArrivalSignal);

        return ERROR_SUCCESS;
    }

    _Use_decl_annotations_
    uint32_t
    JsAdapter::NotifyDeviceRemoval(
        std::shared_ptr<IAdapterDevice> Device
        )
    {
        if (Device == nullptr)
        {
            return static_cast<uint32_t>(ER_BAD_ARG_1);
        }

        std::shared_ptr<IAdapterSignal> deviceRemovalSignal = this->signals.at(DEVICE_REMOVAL_SIGNAL_INDEX);
        deviceRemovalSignal->Params().at(DEVICE_REMOVAL_SIGNAL_PARAM_INDEX)->Data() = Device;
        this->NotifySignalListener(deviceRemovalSignal);

        return ERROR_SUCCESS;
    }

    void
    JsAdapter::CreateSignals()
    {
        try
        {
            // Device Arrival Signal
            std::shared_ptr<AdapterSignal> deviceArrivalSignal(new AdapterSignal(Constants::DEVICE_ARRIVAL_SIGNAL()));
            std::shared_ptr<IAdapterValue> arrivalValue(new AdapterValue(Constants::DEVICE_ARRIVAL__DEVICE_HANDLE(), nullptr));
            deviceArrivalSignal->AddParameter(arrivalValue);

            // Device Removal Signal
            std::shared_ptr<AdapterSignal> deviceRemovalSignal(new AdapterSignal(Constants::DEVICE_REMOVAL_SIGNAL()));
            std::shared_ptr<IAdapterValue> removalValue(new AdapterValue(Constants::DEVICE_REMOVAL__DEVICE_HANDLE(), nullptr));
            deviceRemovalSignal->AddParameter(removalValue);

            // Add Signals to Adapter Signals
            this->signals.push_back(deviceArrivalSignal);
            this->signals.push_back(deviceRemovalSignal);
        }
        catch (...)
        {
            // Log
            std::cerr << "Error creating signals" << std::endl;
            throw;
        }
    }

    //#EnumDevicesFromJavaScript -
    //IAdapterDeviceVector^ JsAdapter::EnumDevicesFromJavaScript()
    //{
    //    Platform::Collections::Vector<Bridge::IAdapterDevice^>^ deviceVector = nullptr;

    //    {
    //        // Call the enumDevicesMethod
    //        JsValueRef result;
    //        std::vector<JsValueRef> args;
    //        JsErrorCode errorCode = ChakraHost::CallScriptFunctionWithResult(&result, L"enumDevices", args);
    //        if (errorCode == JsNoError)
    //        {
    //            if (result == nullptr)
    //            {
    //                DebugLogInfo(L"enumDevices did not return a deviceVector ?");
    //            }
    //            else
    //            {
    //                // Convert the result into an AdapterDeviceVector and iterate over the collection
    //                IInspectable* deviceVectorInspectable = nullptr;
    //                errorCode = JsObjectToInspectable(result, &deviceVectorInspectable);
    //                if (errorCode == JsNoError)
    //                {
    //                    deviceVector = reinterpret_cast<Platform::Collections::Vector<Bridge::IAdapterDevice^>^>(deviceVectorInspectable);
    //                    if (deviceVector != nullptr)
    //                    {
    //                        auto device = deviceVector->GetAt(0);
    //                    }
    //                }
    //                else
    //                {
    //                    DebugLogInfo(L"Javascript is not IInspectable ?");
    //                }
    //            }
    //        }
    //        else
    //        {
    //            DebugLogInfo(L"enumDevices function not found in the Javascript");
    //        }
    //    }

    //    return deviceVector;
    //}

    void JsAdapter::SetAdapterError(std::shared_ptr<IJsAdapterError>& adaptErr)
    {
        this->adapterError = adaptErr;
    }

} // namespace AdapterLib

//-----------------------------------------------------------------------------
// Desc: Outputs a formatted string to the BridgeLog
//-----------------------------------------------------------------------------
void DebugLogInfo(_In_ wchar_t* /*strMsg*/, ...)
{
}
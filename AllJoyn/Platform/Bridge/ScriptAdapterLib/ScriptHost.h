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
#include <unordered_map>
#include "WorkItemDispatcher.h"

namespace ScriptHostConstants
{
    const char DeviceObjectName[] = "device";
    const char InitializationFunctionName[] = "initDevice";
    const char RaiseSignalName[] = "raiseSignal";
    const char ReportResultName[] = "reportResult";
    const char Name[] = "name";
    const char Props[] = "props";
    const char Vendor[] = "vendor";
    const char Model[] = "model";
    const char Version[] = "version";
    const char FirmwareVersion[] = "firmwareVersion";
    const char SerialNumber[] = "serialNumber";
    const char Description[] = "description";
    const char This[] = "This";
    const char Then[] = "then";
    const char ContextName[] = "_adapterDeviceCtx";
};

class ScriptHost
{
public:
    ScriptHost() 
    {
        JX_SetNull(&_deviceObject);
    }

    ~ScriptHost() 
    {
        _dispatcher.DispatchAndWait([&]()
        {
            if (!JX_IsNullOrUndefined(&_deviceObject))
            {
                JX_ClearPersistent(&_deviceObject);
            }
            JX_StopEngine();
        });
        _dispatcher.Shutdown();
    }

    // Initializes this host instance.
    // This starts a new JxCore instance on a dedicated thread.
    void InitializeHost(
        std::shared_ptr<Bridge::IAdapterDevice> device,
        const std::string& script,
        const std::string& modulesPath,
        std::function<void(uint32_t)> callback)
    {
        _dispatcher.Initialize();

        _dispatcher.Dispatch([=]()
        {
            JX_InitializeOnce(".");
            JX_InitializeNewEngine();
            JX_DefineMainFile(script.c_str());
            JX_StartEngine();

            // Register global object
            RegisterDeviceObject(device.get());
            JX_Loop();

            // Call script function initDevice
            JXValue result;
            std::vector<JXValue> params;
            params.emplace_back(_deviceObject);
            if (InternalCallScriptFunction(&result, ScriptHostConstants::InitializationFunctionName, params))
            {
                GetAsyncJxResult(&result,
                    [=](bool success, JXResult* asyncResult)
                    {
                        callback(success ? ERROR_SUCCESS : ERROR_GEN_FAILURE);
                    });
            }
            else
            {
                callback(ERROR_GEN_FAILURE);
            }

            JX_Free(&result);
        });
    }

    // Call a global JavaScript function, and get the result, converting input and output parameters
    // to/from JXValue and PropertyValue.
    // NOTE that due to the way JxCore handles global scope, global functions
    // should be added to the "global" object e.g.
    //          var myFunc = function () { ...... }
    //          globals.myFunc = myFunc;
    // This dispatches the call to the JxCore instance and returns status and results asynchronously via the callback.
    // In future, this can be moved to return results asynchronously.
    // The "insertDeviceObject" parameter, if true, adds the "device" object as the first parameter. The device object is the projected 
    // version of the IAdapterDevice used to start this script host instance.
    void CallScriptFunction(
        const std::string& name,
        const Bridge::AdapterValueVector& inParams,
        bool insertDeviceObject,
        std::function<void(uint32_t,Bridge::AdapterValueVector&)> callback)
    {
        _dispatcher.Dispatch([=]()
        {
            // Convert the input parameters
            std::vector<JXValue> jxInputParams;
            ConvertPropertyValuesToJxValues(inParams, jxInputParams);

            // Should we insert the device object as the first parameter?
            if (insertDeviceObject)
            {
                jxInputParams.insert(jxInputParams.begin(), _deviceObject);
            }
            
            // Call the script function
            JXValue result;
            if (InternalCallScriptFunction(&result, name, jxInputParams))
            {
                GetAsyncJxResult(&result,
                    [=](bool success, JXResult* asyncResult)
                    {
                        if (success)
                        {
                            Bridge::AdapterValueVector outParams;
                            ConvertJxResultToOutParam(asyncResult, outParams);
                            callback(ERROR_SUCCESS, outParams);
                        }
                        else
                        {
                            callback(ERROR_GEN_FAILURE, Bridge::AdapterValueVector());
                        }
                    });
            }
            else
            {
                callback(ERROR_GEN_FAILURE, Bridge::AdapterValueVector());
            }

            JX_Free(&result);
        });
    }

    static void ConvertPropertyValuesToJxValues(const Bridge::AdapterValueVector& valuesIn, std::vector<JXValue>& valuesOut)
    {
        for (auto& valueIn : valuesIn)
        {
            if (valueIn != nullptr)
            {
                Bridge::PropertyValue ipv = (valueIn.get())->Data();
                JXValue value;
                JX_New(&value);
                switch (ipv.Type())
                {
                    case Bridge::PropertyType::Boolean:
                        JX_SetBoolean(&value, ipv.Get<bool>());
                        break;
                    case Bridge::PropertyType::UInt8:
                        JX_SetInt32(&value, ipv.Get<uint8_t>());
                        break;
                    case Bridge::PropertyType::Int16:
                        JX_SetInt32(&value, ipv.Get<int16_t>());
                        break;
                    case Bridge::PropertyType::Int32:
                        JX_SetInt32(&value, ipv.Get<int32_t>());
                        break;
                    case Bridge::PropertyType::Int64:
                        // Lossy conversion
                        JX_SetInt32(&value, ipv.Get<int32_t>());
                        break;
                    case Bridge::PropertyType::UInt16:
                        JX_SetInt32(&value, ipv.Get<uint16_t>());
                        break;
                    case Bridge::PropertyType::UInt32:
                        JX_SetInt32(&value, ipv.Get<uint32_t>());
                        break;
                    case Bridge::PropertyType::UInt64:
                        // Lossy conversion
                        JX_SetInt32(&value, ipv.Get<uint32_t>());
                        break;
                    case Bridge::PropertyType::Double:
                        JX_SetDouble(&value, ipv.Get<double>());
                        break;
                    case Bridge::PropertyType::String:
                        JX_SetString(&value, ipv.Get<std::string>().c_str(), (const uint32_t)ipv.Get<std::string>().length());
                        break;
                    default:
                    {
                        std::string errorString = "Failed to convert AdapterValue into JXValue";
                        JX_SetError(&value, errorString.c_str(), (const uint32_t)errorString.length());
                        break;
                    }
                }
                valuesOut.push_back(value);
            }
        }
    }

    static void ConvertJxResultToOutParam(JXValue* result, Bridge::AdapterValueVector& valuesOut)
    {
        Bridge::PropertyValue value = nullptr;
        
        for (auto& valueOut : valuesOut)
        {
            if (valueOut != nullptr)
            {
                Bridge::PropertyValue ipv = (valueOut.get())->Data();
                {
                    switch (ipv.Type())
                    {
                    case Bridge::PropertyType::Boolean:
                        value = JX_GetBoolean(result);
                        break;
                    case Bridge::PropertyType::UInt8:
                        value = (uint8_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::Int16:
                        value = (int16_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::Int32:
                        value = (int32_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::Int64:
                        // Lossy conversion
                        value = (int64_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::UInt16:
                        value = (uint16_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::UInt32:
                        value = (uint32_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::UInt64:
                        // Lossy conversion
                        value = (uint64_t)JX_GetInt32(result);
                        break;
                    case Bridge::PropertyType::Double:
                        value = JX_GetDouble(result);
                        break;
                    case Bridge::PropertyType::String:
                        value = std::move(std::string(JX_GetString(result)));
                        break;
                    default:
                        value = std::move(std::string("Not implemented: Cannot convert JX data type back to PropertyValue"));
                        break;
                    }

                    valueOut->Data() = value;
                }
            }
        }
    }

    void GetAsyncJxResult(
        JXValue* result,
        std::function<void(bool,JXValue*)> resultCallback)
    {
        if (JX_IsObject(result))
        {
            JXValue resultThen;
            JX_GetNamedProperty(result, ScriptHostConstants::Then, &resultThen);
            bool resultIsAPromise = JX_IsFunction(&resultThen);
            JX_Free(&resultThen);

            if (resultIsAPromise)
            {
                // The result is an object with a then() method, so it is assumed to be a Promise.
                // Call then() to set up success and failure callbacks via the reportResult()
                // native method registered on the global device object.

                // This call ID value could eventually roll over, and that's fine. If a device method
                // invocation hasn't returned by the time 4 billion more have been invoked, it never will.
                int32_t callId = ++_nextCallId;
                _callbackMap.emplace(callId, resultCallback);

                std::string callbackScript = StringUtils::Format(
                    "(function (promise) {"
                    "  promise.then("
                    "    function (result) {"
                    "      %s.%s(%d, true, result);"
                    "    },"
                    "    function (error) {"
                    "      %s.%s(%d, false, error);"
                    "    });"
                    "})",
                    ScriptHostConstants::DeviceObjectName, ScriptHostConstants::ReportResultName, callId,
                    ScriptHostConstants::DeviceObjectName, ScriptHostConstants::ReportResultName, callId);

                JXValue callbackFunction;
                JX_Evaluate(callbackScript.c_str(), nullptr, &callbackFunction);

                JXValue unusedResult;
                if (!JX_CallFunction(&callbackFunction, result, 1, &unusedResult))
                {
                    JXValue callbackError;
                    JX_New(&callbackError);
                    const char message[] = "Error setting up promise callbacks.";
                    JX_SetError(&callbackError, message, _countof(message));

                    resultCallback(false, &callbackError);

                    JX_Free(&callbackError);
                }

                JX_Free(&unusedResult);
                JX_Free(&callbackFunction);

                JX_Loop();
                return;
            }
        }

        // The result is not a Promise, so just invoke the success handler directly.
        resultCallback(true, result);
    }

    static void ConvertJxValuesToAdapterValues(std::vector<JXValue>& valuesIn, Bridge::AdapterValueVector& valuesOut)
    {
        std::string name;
        Bridge::PropertyValue value = nullptr;
        int ix = 0;
        for (auto& valueIn : valuesIn)
        {
            switch (valueIn.type_)
            {
                case RT_Null:
                {
                    value = nullptr;
                }
                break;
                case RT_Boolean:
                {
                    value = JX_GetBoolean(&valueIn);
                }
                break;
                case RT_Double:
                {
                    value = JX_GetDouble(&valueIn);
                }
                break;
                case RT_Int32:
                {
                    value = JX_GetInt32(&valueIn);
                }
                break;
                case RT_String:
                {
                    value = std::move(std::string(JX_GetString(&valueIn)));
                }
                break;
                default:
                    value = std::move(std::string("Not implemented: Cannot convert JX data type back to PropertyValue"));
                    break;
            }

            if (valuesOut.size() > ix)
            {
                valuesOut[ix]->Data() = value;
            }
            else
            {
                std::shared_ptr<Bridge::AdapterValue> adapterValue(new Bridge::AdapterValue(name, value));
                valuesOut.push_back(adapterValue);
            }
            ix++;
        }
    }

private:

    bool InternalCallScriptFunction(JXValue* result, const std::string& name, std::vector<JXValue>& params)
    {
        JXValue global;
        JXValue func;
        JX_GetGlobalObject(&global);

        JX_GetNamedProperty(&global, name.c_str(), &func);
        bool completed = JX_CallFunction(&func, params.data(), (const int) params.size(), result);
        JX_Loop();
        return completed;
    }

    void SetStringProperty(JXValue* obj, const std::string& name, const std::string& value)
    {
        JXValue val;
        JX_New(&val);
        JX_SetString(&val, value.c_str(), (const int32_t) value.length());
        JX_SetNamedProperty(obj, name.c_str(), &val);
        //JX_Free(&val);
    }

    void RegisterDeviceObject(Bridge::IAdapterDevice* pDevice)
    {
        // Set native methods
        JXValue global;
        JX_CreateEmptyObject(&_deviceObject);

        JX_GetGlobalObject(&global);
        JX_SetNamedProperty(&global, ScriptHostConstants::DeviceObjectName, &_deviceObject);
        // Register the native methods
        JX_SetNativeMethod(&_deviceObject, ScriptHostConstants::RaiseSignalName, &RaiseSignalCallback);
        JX_SetNativeMethod(&_deviceObject, ScriptHostConstants::ReportResultName, &ReportResultCallback);
        // Set the initial standard properties
        SetStringProperty(&_deviceObject, ScriptHostConstants::Name, pDevice->Name());
        SetStringProperty(&_deviceObject, ScriptHostConstants::Props, pDevice->Props());
        SetStringProperty(&_deviceObject, ScriptHostConstants::Vendor, pDevice->Vendor());
        SetStringProperty(&_deviceObject, ScriptHostConstants::Model, pDevice->Model());
        SetStringProperty(&_deviceObject, ScriptHostConstants::Version, pDevice->Version());
        SetStringProperty(&_deviceObject, ScriptHostConstants::FirmwareVersion, pDevice->FirmwareVersion());
        SetStringProperty(&_deviceObject, ScriptHostConstants::SerialNumber, pDevice->SerialNumber());
        SetStringProperty(&_deviceObject, ScriptHostConstants::Description, pDevice->Description());
        JXValue testVal;
        JX_New(&testVal);
        JX_SetInt32(&testVal, 42);
        JX_SetNamedProperty(&_deviceObject, "testVal", &testVal);

        // Register generic get/set for properties
        //JX_SetNativeMethod(&_deviceObject, "getProp", getProp);
        //JX_SetNativeMethod(&_deviceObject, "setProp", setProp);

        // As a workaround for legacy scripts, inject "this" as a global object.
        JX_SetNamedProperty(&global, ScriptHostConstants::This, &_deviceObject);

        // Make this persistent so we can always use it
        JX_MakePersistent(&_deviceObject);
        // Set the context for the device object to be the IAdapterDevice
        JX_WrapObject(&_deviceObject, pDevice);
        // Workaround: WrapObject doesn't persist accross calls into the JS, so we'll store
        // the context in a named property instead.
        JXValue ctx;
        JX_New(&ctx);
        std::string ctxString = std::to_string(reinterpret_cast<uintptr_t>(pDevice));
        JX_SetString(&ctx, ctxString.c_str(), (const uint32_t)ctxString.length());
        JX_SetNamedProperty(&_deviceObject, ScriptHostConstants::ContextName, &ctx);
    }

    static void ReportResultCallback(JXValue* argv, int argc)
    {
        Bridge::IAdapterDevice* pDevice = GetDeviceContext();
        if (pDevice && pDevice->HostContext() && argc == 3)
        {
            ScriptHost* host = reinterpret_cast<ScriptHost*>((void*)pDevice->HostContext());

            // These 3 parameters are passed in to the device.reportResult() method by the promise continuation script.
            int32_t callId = JX_GetInt32(argv + 0);
            bool success = JX_GetBoolean(argv + 1);
            JXValue* pResult = argv + 2;

            // Look up the result callback in the map, and call it if found.
            // There's no need to lock access to this map since it is always accessed on the JS engine main thread.
            auto entry = host->_callbackMap.find(callId);
            if (entry != host->_callbackMap.end())
            {
                std::function<void(bool, JXValue*)> callback = entry->second;
                host->_callbackMap.erase(callId);

                callback(success, pResult);
            }
        }
    }

    static void RaiseSignalCallback(JXValue* argv, int argc)
    {
        Bridge::IAdapterDevice* pDevice = GetDeviceContext();
        if (pDevice)
        {
            if (argc == 1)
            {
                std::string signalName = JX_GetString(argv + 0);
                pDevice->RaiseSignal(signalName);
            }
            else if (argc >= 2)
            {
                std::string signalName = JX_GetString(argv + 0);
                std::vector<JXValue> args;
                args.push_back(argv[1]);
                Bridge::AdapterValueVector signalParams;
                ConvertJxValuesToAdapterValues(args, signalParams);

                if ((signalParams.size() > 0) && (signalParams[0] != nullptr))
                {
                    pDevice->RaiseSignal(signalName, signalParams[0].get()->Data());
                }
            }
        }
    }

    static Bridge::IAdapterDevice* GetDeviceContext()
    {
        // Get get the global object and find our context object
        JXValue obj;
        JXValue global;
        JX_GetGlobalObject(&global);
        JX_GetNamedProperty(&global, ScriptHostConstants::DeviceObjectName, &obj);

        if (obj.type_ == RT_Object)
        {
            // Workaround: JX_WrapObject doesn't persist accross calls into the JS, so we'll retrieve
            // the context from a named property instead of using JX_UnwrapObject.
            JXValue ctx;
            JX_GetNamedProperty(&obj, ScriptHostConstants::ContextName, &ctx);
            std::istringstream convert(JX_GetString(&ctx));
            uintptr_t pCtx;
            if (convert >> pCtx)
            {
                Bridge::IAdapterDevice* pDevice = reinterpret_cast<Bridge::IAdapterDevice*>(pCtx);
                return pDevice;
            }
        }

        return nullptr;
    }

    JXValue _deviceObject;
    std::unordered_map<int32_t, std::function<void(bool,JXValue*)>> _callbackMap;
    int32_t _nextCallId;
    WorkItemDispatcher _dispatcher;
    //TODO: Timer to do periodic JX_Loop() calls
};

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
    const char Length[] = "length";
    const char MainModule[] = "mainModule";
    const char Exports[] = "exports";
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

            uint32_t status = InternalCallScriptFunction(
                &result, ScriptHostConstants::InitializationFunctionName, params);
            if (status == ERROR_SUCCESS)
            {
                GetAsyncJxResult(&result,
                    [=](bool success, JXResult* asyncResult)
                    {
                        callback(success ? ERROR_SUCCESS : ERROR_GEN_FAILURE);
                    });
            }
            else
            {
                callback(status);
            }

            JX_Free(&result);
        });
    }

    // Call a JavaScript property getter on the device module, and convert the result from JXValue to IAdapterValue.
    // Device properties must be exported from the node module.
    // This dispatches the call to the JxCore instance and returns status asynchronously via the callback.
    void InvokePropertyGetter(
        const std::string& name,
        std::shared_ptr<Bridge::IAdapterValue> outParam,
        std::function<void(uint32_t)> callback)
    {
        _dispatcher.Dispatch([=]()
        {
            JXValue exports;
            GetExportsObject(&exports);

            JXValue prop;
            JX_GetNamedProperty(&exports, name.c_str(), &prop);
            if (!JX_IsUndefined(&prop))
            {
                // An actual property was found. Get the value directly.
                ConvertJxValueToAdapterValue(&prop, outParam);
                callback(ERROR_SUCCESS);
            }
            else
            {
                // Check for a getProperty() method that implements the property, with appropriate casing.
                std::locale loc;
                std::string methodName = std::string("get") + std::toupper(name[0], loc) + name.substr(1);

                JX_GetNamedProperty(&exports, methodName.c_str(), &prop);
                if (JX_IsFunction(&prop))
                {
                    JXValue result;
                    bool completed = JX_CallFunction(&prop, nullptr, 0, &result);
                    if (completed)
                    {
                        GetAsyncJxResult(&result, [=](bool success, JXResult* asyncResult)
                        {
                            if (success)
                            {
                                ConvertJxValueToAdapterValue(asyncResult, outParam);
                                callback(ERROR_SUCCESS);
                            }
                            else
                            {
                                callback(ERROR_GEN_FAILURE);
                            }
                        });
                    }
                    else
                    {
                        callback(ERROR_GEN_FAILURE);
                    }

                    JX_Free(&result);
                }
                else
                {
                    callback(ERROR_NOT_FOUND);
                }
            }

            JX_Free(&prop);
            JX_Free(&exports);
        });
    }

    // Call a JavaScript property setter on the device module, converting the value from IAdapterValue to JXValue.
    // Device properties must be exported from the node module.
    // This dispatches the call to the JxCore instance and returns status asynchronously via the callback.
    void InvokePropertySetter(
        const std::string& name,
        const std::shared_ptr<Bridge::IAdapterValue> inParam,
        std::function<void(uint32_t)> callback)
    {
        _dispatcher.Dispatch([=]()
        {
            // Convert the input parameter
            std::vector<JXValue> jxInputParams;
            Bridge::AdapterValueVector inParams(1);
            inParams[0] = inParam;
            ConvertAdapterValuesToJxValues(inParams, jxInputParams);

            JXValue exports;
            GetExportsObject(&exports);

            JXValue prop;
            JX_GetNamedProperty(&exports, name.c_str(), &prop);
            if (!JX_IsUndefined(&prop))
            {
                // An actual property was found. Set the value directly.
                JX_SetNamedProperty(&exports, name.c_str(), &jxInputParams[0]);
                callback(ERROR_SUCCESS);
            }
            else
            {
                // Check for a setProperty() method that implements the property, with appropriate casing.
                std::locale loc;
                std::string methodName = std::string("set") + std::toupper(name[0], loc) + name.substr(1);

                JX_GetNamedProperty(&exports, methodName.c_str(), &prop);
                if (JX_IsFunction(&prop))
                {
                    JXValue result;
                    bool completed = JX_CallFunction(&prop, jxInputParams.data(), 1, &result);
                    if (completed)
                    {
                        GetAsyncJxResult(&result, [=](bool success, JXResult* asyncResult)
                        {
                            callback(success ? ERROR_SUCCESS : ERROR_GEN_FAILURE);
                        });
                    }
                    else
                    {
                        callback(ERROR_GEN_FAILURE);
                    }

                    JX_Free(&result);
                }
                else
                {
                    callback(ERROR_NOT_FOUND);
                }
            }

            JX_Free(&prop);
            JX_Free(&exports);
        });
    }

    // Call a JavaScript method on the device module, and get the result, converting input and output parameters
    // to/from JXValue and PropertyValue. Device methods must be exported from the device node module.
    // This dispatches the call to the JxCore instance and returns status asynchronously via the callback.
    // The "insertDeviceObject" parameter, if true, adds the "device" object as the first parameter. The device object is the projected
    // version of the IAdapterDevice used to start this script host instance.
    void InvokeMethod(
        const std::string& name,
        const Bridge::AdapterValueVector& inParams,
        std::shared_ptr<Bridge::IAdapterValue> outParam,
        bool insertDeviceObject,
        std::function<void(uint32_t)> callback)
    {
        _dispatcher.Dispatch([=]()
        {
            // Convert the input parameters
            std::vector<JXValue> jxInputParams;
            ConvertAdapterValuesToJxValues(inParams, jxInputParams);

            // Should we insert the device object as the first parameter?
            if (insertDeviceObject)
            {
                jxInputParams.insert(jxInputParams.begin(), _deviceObject);
            }

            // Call the script function
            JXValue result;
            uint32_t status = InternalCallScriptFunction(&result, name, jxInputParams);
            if (status == ERROR_SUCCESS)
            {
                GetAsyncJxResult(&result, [=](bool success, JXResult* asyncResult)
                {
                    if (success)
                    {
                        ConvertJxValueToAdapterValue(asyncResult, outParam);
                        callback(ERROR_SUCCESS);
                    }
                    else
                    {
                        callback(ERROR_GEN_FAILURE);
                    }
                });
            }
            else
            {
                callback(status);
            }

            JX_Free(&result);
        });
    }

private:

    static void ConvertAdapterValueToJxValue(const std::shared_ptr<Bridge::IAdapterValue>& valueIn, JXValue* valueOut)
    {
        Bridge::PropertyValue ipv = (valueIn.get())->Data();
        switch (ipv.Type())
        {
        case Bridge::PropertyType::Boolean:
            JX_SetBoolean(valueOut, ipv.Get<bool>());
            break;
        case Bridge::PropertyType::UInt8:
            JX_SetInt32(valueOut, ipv.Get<uint8_t>());
            break;
        case Bridge::PropertyType::Int16:
            JX_SetInt32(valueOut, ipv.Get<int16_t>());
            break;
        case Bridge::PropertyType::Int32:
            JX_SetInt32(valueOut, ipv.Get<int32_t>());
            break;
        case Bridge::PropertyType::Int64:
            // Lossy conversion
            JX_SetInt32(valueOut, ipv.Get<int32_t>());
            break;
        case Bridge::PropertyType::UInt16:
            JX_SetInt32(valueOut, ipv.Get<uint16_t>());
            break;
        case Bridge::PropertyType::UInt32:
            JX_SetInt32(valueOut, ipv.Get<uint32_t>());
            break;
        case Bridge::PropertyType::UInt64:
            // Lossy conversion
            JX_SetInt32(valueOut, ipv.Get<uint32_t>());
            break;
        case Bridge::PropertyType::Double:
            JX_SetDouble(valueOut, ipv.Get<double>());
            break;
        case Bridge::PropertyType::String:
            JX_SetString(valueOut, ipv.Get<std::string>().c_str(), (const int32_t)ipv.Get<std::string>().length());
            break;
        case Bridge::PropertyType::Int32Array:
        {
            JX_CreateArrayObject(valueOut);
            std::vector<int32_t> array = ipv.Get<std::vector<int32_t>>();
            for (unsigned int i = 0; i < array.size(); i++)
            {
                JXValue elementValue;
                JX_New(&elementValue);
                JX_SetInt32(&elementValue, array[i]);
                JX_SetIndexedProperty(valueOut, i, &elementValue);
                JX_Free(&elementValue);
            }
            break;
        }
        // TODO: Other array types
        case Bridge::PropertyType::StringArray:
        {
            JX_CreateArrayObject(valueOut);
            std::vector<std::string> array = ipv.Get<std::vector<std::string>>();
            for (unsigned int i = 0; i < array.size(); i++)
            {
                JXValue elementValue;
                JX_New(&elementValue);
                JX_SetString(&elementValue, array[i].c_str(), (const int32_t)array[i].size());
                JX_SetIndexedProperty(valueOut, i, &elementValue);
                JX_Free(&elementValue);
            }
            break;
        }
        case Bridge::PropertyType::StringDictionary:
        {
            JX_CreateEmptyObject(valueOut);
            auto dict = ipv.Get<std::unordered_map<std::string, std::string>>();
            for (const std::pair<std::string, std::string>& dictElement : dict)
            {
                JXValue elementValue;
                JX_New(&elementValue);
                JX_SetString(&elementValue, dictElement.second.c_str(), (const int32_t)dictElement.second.size());
                JX_SetNamedProperty(valueOut, dictElement.first.c_str(), &elementValue);
                JX_Free(&elementValue);
            }
            break;
        }
        default:
            std::string errorString = "Not implemented: Cannot convert AdapterValue to JXValue";
            JX_SetError(valueOut, errorString.c_str(), (const uint32_t)errorString.length());
            break;
        }
    }

    static void ConvertAdapterValuesToJxValues(const Bridge::AdapterValueVector& valuesIn, std::vector<JXValue>& valuesOut)
    {
        for (auto& valueIn : valuesIn)
        {
            if (valueIn != nullptr)
            {
                JXValue valueOut;
                JX_New(&valueOut);
                ConvertAdapterValueToJxValue(valueIn, &valueOut);
                valuesOut.push_back(valueOut);
            }
        }
    }

    static void ConvertJxValueToAdapterValue(JXValue* valueIn, std::shared_ptr<Bridge::IAdapterValue> valueOut)
    {
        if (valueOut == nullptr)
        {
            // The caller is ignoring the return value.
            return;
        }

        Bridge::PropertyValue value = nullptr;
        Bridge::PropertyValue ipv = valueOut->Data();
        switch (ipv.Type())
        {
        case Bridge::PropertyType::Boolean:
            value = JX_GetBoolean(valueIn);
            break;
        case Bridge::PropertyType::UInt8:
            value = (uint8_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::Int16:
            value = (int16_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::Int32:
            value = (int32_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::Int64:
            // Lossy conversion
            value = (int64_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::UInt16:
            value = (uint16_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::UInt32:
            value = (uint32_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::UInt64:
            // Lossy conversion
            value = (uint64_t)JX_GetInt32(valueIn);
            break;
        case Bridge::PropertyType::Double:
            value = JX_GetDouble(valueIn);
            break;
        case Bridge::PropertyType::String:
            value = std::move(std::string(JX_GetString(valueIn)));
            break;
        case Bridge::PropertyType::Int32Array:
        {
            std::vector<int> array;
            JXValue arrayLength;
            JX_GetNamedProperty(valueIn, ScriptHostConstants::Length, &arrayLength);
            if (JX_IsInt32(&arrayLength))
            {
                int length = (unsigned int)JX_GetInt32(&arrayLength);
                for (int i = 0; i < length; i++)
                {
                    JXValue elementValue;
                    JX_GetIndexedProperty(valueIn, i, &elementValue);
                    if (JX_IsInt32(&elementValue))
                    {
                        int elementInt = JX_GetInt32(&elementValue);
                        array.push_back(elementInt);
                    }
                    else
                    {
                        array.push_back(0);
                    }

                    JX_Free(&elementValue);
                }
            }

            JX_Free(&arrayLength);
            value = std::move(array);
            break;
        }
        // TODO: Other array types
        case Bridge::PropertyType::StringArray:
        {
            std::vector<std::string> array;
            JXValue arrayLength;
            JX_GetNamedProperty(valueIn, ScriptHostConstants::Length, &arrayLength);
            if (JX_IsInt32(&arrayLength))
            {
                int length = (unsigned int)JX_GetInt32(&arrayLength);
                for (int i = 0; i < length; i++)
                {
                    JXValue elementValue;
                    JX_GetIndexedProperty(valueIn, i, &elementValue);
                    if (JX_IsString(&elementValue))
                    {
                        char* elementString = JX_GetString(&elementValue);
                        array.emplace_back(elementString);
                    }
                    else
                    {
                        array.emplace_back("");
                    }

                    JX_Free(&elementValue);
                }
            }

            JX_Free(&arrayLength);
            value = std::move(array);
            break;
        }
        case Bridge::PropertyType::StringDictionary:
        {
            std::unordered_map<std::string, std::string> dict;
            if (JX_IsObject(valueIn))
            {
                JXValue keysFunc;
                JX_Evaluate("Object.keys", nullptr, &keysFunc);
                if (JX_IsFunction(&keysFunc))
                {
                    JXValue keysArray;
                    JX_CallFunction(&keysFunc, valueIn, 1, &keysArray);

                    JXValue arrayLength;
                    JX_GetNamedProperty(&keysArray, ScriptHostConstants::Length, &arrayLength);

                    if (JX_IsInt32(&arrayLength))
                    {
                        int length = (unsigned int)JX_GetInt32(&arrayLength);
                        for (int i = 0; i < length; i++)
                        {
                            JXValue key;
                            JX_GetIndexedProperty(&keysArray, i, &key);

                            if (JX_IsString(&key))
                            {
                                char* keyString = JX_GetString(&key);

                                JXValue value;
                                JX_GetNamedProperty(valueIn, keyString, &value);
                                if (JX_IsString(&value))
                                {
                                    char* valueString = JX_GetString(&value);
                                    dict.emplace(keyString, valueString);
                                }
                                else
                                {
                                    dict.emplace(keyString, "");
                                }

                                JX_Free(&value);
                            }

                            JX_Free(&key);
                        }
                    }

                    JX_Free(&keysArray);
                    JX_Free(&arrayLength);
                }

                JX_Free(&keysFunc);
            }

            value = std::move(dict);
            break;
        }
        default:
            value = std::move(std::string("Not implemented: Cannot convert JXValue to AdapterValue"));
            break;
        }

        valueOut->Data() = value;
    }

    static void ConvertJxValuesToAdapterValues(std::vector<JXValue>& valuesIn, Bridge::AdapterValueVector& valuesOut)
    {
        Bridge::PropertyValue value = nullptr;
        int i = 0;
        for (auto& valueIn : valuesIn)
        {
            std::shared_ptr<Bridge::IAdapterValue> adapterValue;
            if (valuesOut.size() > i)
            {
                adapterValue = valuesOut[i];
            }
            else
            {
                adapterValue = std::make_shared<Bridge::AdapterValue>("", value);
                valuesOut.push_back(adapterValue);
            }

            ConvertJxValueToAdapterValue(&valueIn, adapterValue);
            i++;
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

    uint32_t InternalCallScriptFunction(JXValue* result, const std::string& name, std::vector<JXValue>& params)
    {
        JXValue exports;
        GetExportsObject(&exports);

        JXValue func;
        JX_GetNamedProperty(&exports, name.c_str(), &func);
        if (JX_IsFunction(&func))
        {
            bool completed = JX_CallFunction(&func, params.data(), (const int)params.size(), result);
            JX_Loop();
            JX_Free(&func);
            return completed ? ERROR_SUCCESS : ERROR_GEN_FAILURE;
        }
        else
        {
            JX_Free(&func);
            return ERROR_NOT_FOUND;
        }
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

    // Get the object that contains the script's exported properties and methods.
    static void GetExportsObject(JXValue* exports)
    {
        JXValue process;
        JX_GetProcessObject(&process);

        JXValue module;
        JX_GetNamedProperty(&process, ScriptHostConstants::MainModule, &module);
        JX_Free(&process);

        JX_GetNamedProperty(&module, ScriptHostConstants::Exports, exports);
        JX_Free(&module);

        if (!JX_IsObject(exports))
        {
            // Fallback to the global object.
            JX_Free(exports);
            JX_GetGlobalObject(exports);
        }
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

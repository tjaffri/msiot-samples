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

namespace Bridge
{
    static const int WAIT_TIMEOUT_FOR_ADAPTER_OPERATION = 20000;  // ms

    class DeviceMain;

    class AllJoynHelper
    {
    public:
        AllJoynHelper();

        static QStatus SetMsgArg(_In_ std::shared_ptr<IAdapterValue> adapterValue, _Inout_ alljoyn_msgarg msgArg);

        template<typename T>
        static QStatus SetMsgArg(_Inout_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _In_ std::vector<T>& arrayArg);
        static QStatus SetMsgArg(_Inout_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _In_ std::vector<bool>& arrayArg);

        static QStatus SetMsgArgFromAdapterObject(_In_ std::shared_ptr<IAdapterValue> adapterValue, _Inout_ alljoyn_msgarg msgArg, _In_ DeviceMain *deviceMain);

        template<typename T>
        static QStatus GetArrayFromMsgArg(_In_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _Out_ std::vector<T>& arrayArg);
        static QStatus GetArrayFromMsgArg(_In_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _Out_ std::vector<bool>& arrayArg);

        static QStatus GetAdapterValueFromMsgArg(_In_ alljoyn_msgarg msgArg, _Out_ std::shared_ptr<IAdapterValue>& adapterValue);
        static QStatus GetPropertyTypeFromMsgArg(_In_ alljoyn_msgarg msgArg, _Out_ PropertyType& propertyType);
        static QStatus GetAdapterValue(_Inout_ std::shared_ptr<IAdapterValue> adapterValue, _In_ alljoyn_msgarg msgArg);
        static QStatus GetAdapterObject(_Inout_ std::shared_ptr<IAdapterValue> adapterValue, _In_ alljoyn_msgarg msgArg, _In_ DeviceMain *deviceMain);
        static QStatus GetSignature(_In_ PropertyType propertyType, _Out_ std::string &signature);
        static std::shared_ptr<IAdapterValue> GetDefaultAdapterValue(_In_ const char* signature);

        static void EncodeBusObjectName(_In_ std::string inString, _Inout_ std::string &builtName);
        static void EncodePropertyOrMethodOrSignalName(_In_ std::string inString, _Inout_ std::string &builtName);
        static void EncodeStringForInterfaceName(_In_ std::string inString, _Out_ std::string &encodeString);
        static void EncodeStringForServiceName(_In_ std::string inString, _Out_ std::string &encodeString);
        static void EncodeStringForRootServiceName(_In_ std::string inString, _Out_ std::string &encodeString);
        static void EncodeStringForAppName(_In_ std::string inString, _Out_ std::string &encodeString);
        static std::string TrimChar(const std::string& inString, char ch);
    };
}


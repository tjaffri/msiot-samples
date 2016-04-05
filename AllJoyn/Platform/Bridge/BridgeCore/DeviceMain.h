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

#include "AdapterConstants.h"
#include "BridgeDevice.h"

namespace Bridge
{
    class DeviceMethod;
    class DeviceSignal;

    class DeviceMain
    {
    public:
        DeviceMain();
        virtual ~DeviceMain();

        QStatus Initialize(_In_ std::shared_ptr<BridgeDevice> parent);
        bool IsMethodNameUnique(_In_ std::string name);
        bool IsSignalNameUnique(_In_ std::string name);
        void HandleSignal(std::shared_ptr<IAdapterSignal> adapterSignal);

        inline alljoyn_busobject GetBusObject()
        {
            return m_AJBusObject;
        }
        inline alljoyn_interfacedescription GetInterfaceDescription()
        {
            return m_interfaceDescription;
        }
        inline uint32_t GetIndexForMethod()
        {
            return m_indexForMethod++;
        }
        inline uint32_t GetIndexForSignal()
        {
            return m_indexForSignal++;
        }
        inline std::shared_ptr<IAdapterProperty> GetAdapterProperty(_In_ std::string busObjectPath)
        {
            return m_parent->GetAdapterProperty(busObjectPath);
        }
        inline std::string GetBusObjectPath(_In_ std::shared_ptr<IAdapterProperty> adapterProperty)
        {
            return m_parent->GetBusObjectPath(adapterProperty);
        }
        inline std::shared_ptr<IAdapter> GetAdapter()
        {
            return m_parent->GetAdapter();
        }
        inline std::shared_ptr<IAdapterDevice> GetAdapterDevice()
        {
            return m_parent->GetAdapterDevice();
        }
    private:
        void Shutdown();
        QStatus CreateMethodsAndSignals();

        static DeviceMain *GetInstance(_In_ alljoyn_busobject busObject);
        static void AJ_CALL AJMethod(_In_ alljoyn_busobject busObject, _In_ const alljoyn_interfacedescription_member* member, _In_ alljoyn_message msg);
        static QStatus AJ_CALL GetPropertyHandler(_In_ const void* context, _In_z_ const char* ifcName, _In_z_ const char* propName, _Out_ alljoyn_msgarg val);
        static QStatus AJ_CALL SetPropertyHandler(_In_ const void* context, _In_z_ const char* ifcName, _In_z_ const char* propName, _In_ alljoyn_msgarg val);
        QStatus GetPropertyValue(_In_z_ const char* ifcName, _In_z_ const char* propName, _Out_ alljoyn_msgarg val);
        QStatus SetPropertyValue(_In_z_ const char* ifcName, _In_z_ const char* propName, _In_ alljoyn_msgarg val);

        // list of device method
        std::map<std::string, DeviceMethod *> m_deviceMethods;
        uint32_t m_indexForMethod;

        // list of Signals
        std::map<std::string, DeviceSignal *> m_deviceSignals;
        uint32_t m_indexForSignal;

        // alljoyn related
        alljoyn_busobject m_AJBusObject;
        alljoyn_interfacedescription m_interfaceDescription;

        bool m_registeredOnAllJoyn;
        std::string m_busObjectPath;
        std::string m_interfaceName;

        // parent object
        std::shared_ptr<BridgeDevice> m_parent;
    };
}

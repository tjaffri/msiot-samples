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

#include <vector>
#include "AdapterConstants.h"
#include "BridgeAuthHandler.h"
#include "AllJoynAbout.h"
#include "AllJoynAboutIcon.h"
#include "IAdapter.h"

namespace Bridge
{
    class DeviceMain;
    class DeviceProperty;
    class PropertyInterface;
    class ControlPanel;
    class LSF;

    class BridgeDevice final : public IAdapterSignalListener, public std::enable_shared_from_this<BridgeDevice>
    {
    public:
        BridgeDevice();
        virtual ~BridgeDevice();

        // IAdapterSignalListener implementation
        void AdapterSignalHandler(_In_ std::shared_ptr<IAdapter> sender, _In_ std::shared_ptr<IAdapterSignal> signal, _In_opt_ intptr_t context = 0) override;

        inline std::shared_ptr<IAdapterDevice> GetAdapterDevice()
        {
            return m_device;
        }

        void RaiseSignal(std::shared_ptr<IAdapterSignal> signal);

    //internal:
        QStatus Initialize(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterDevice> device);
        void Shutdown();

        std::shared_ptr<IAdapterProperty> GetAdapterProperty(_In_ std::string busObjectPath);
        std::string GetBusObjectPath(_In_ std::shared_ptr<IAdapterProperty> adapterProperty);
        bool IsEqual(_In_ std::shared_ptr<IAdapterDevice> device);
        bool IsBusObjectPathUnique(std::string &path);
        inline uint32_t GetUniqueIdForInterface()
        {
            return m_uniqueIdForInterfaces++;
        }
        inline alljoyn_busattachment GetBusAttachment()
        {
            return m_AJBusAttachment;
        }
        inline std::string &GetRootNameForInterface()
        {
            return m_RootStringForAllJoynNames;
        }
        inline DeviceMain *GetDeviceMainObject()
        {
            return m_deviceMain;
        }
        inline bool IsCOVSupported()
        {
            return m_supportCOVSignal;
        }

        inline std::shared_ptr<IAdapter> GetAdapter()
        {
            return m_adapter;
        }
    private:
        void VerifyCOVSupport();
        QStatus RegisterSignalHandlers(_In_ bool isRegister);
        void HandleCOVSignal(_In_ std::shared_ptr<IAdapterSignal> signal);
        QStatus InitializeAllJoyn();
        QStatus ConnectToAllJoyn();
        void ShutdownAllJoyn();

        QStatus CreateDeviceProperties();
        QStatus GetInterfaceProperty(_In_ std::shared_ptr<IAdapterProperty> adapterProperty, _Out_ PropertyInterface **propertyInterface);
        PropertyInterface *FindMatchingInterfaceProperty(_In_ std::string & interfaceName);
        PropertyInterface *FindMatchingInterfaceProperty(_In_ std::shared_ptr<IAdapterProperty> adapterProperty);
        QStatus CreateInterfaceProperty(_In_ std::shared_ptr<IAdapterProperty> adapterProperty, _In_ std::string & interfaceName, _Out_ PropertyInterface **propertyInterface);

        QStatus BuildServiceName();

        QStatus InitControlPanel();

        // callback for session listener
        static QCC_BOOL AJ_CALL AcceptSessionJoinerCallback(_In_ const void* context, _In_ alljoyn_sessionport sessionPort, _In_z_ const char* joiner, _In_ const alljoyn_sessionopts opts);
        static void AJ_CALL SessionJoined(_In_ void *context, _In_ alljoyn_sessionport sessionPort, _In_ alljoyn_sessionid id, _In_z_ const char *joiner);
        static void AJ_CALL MemberRemoved(_In_ void* context, _In_ alljoyn_sessionid sessionid, _In_z_ const char* uniqueName);

        // list of active sessions
        std::vector<alljoyn_sessionid> m_activeSessions;

        // list of device properties
        std::map<std::string, DeviceProperty *> m_deviceProperties;

        // list of AllJoyn interfaces that a device property can expose
        std::vector<PropertyInterface *>    m_propertyInterfaces;

        uint32_t m_uniqueIdForInterfaces;

        std::shared_ptr<IAdapter> m_adapter;

        // main interface
        // that handles methods and signals defined at IAdapterDevice level
        DeviceMain *m_deviceMain;

        // authentication management
        BridgeAuthHandler m_authHandler;

        // corresponding adapter device
        std::shared_ptr<IAdapterDevice > m_device;
        bool m_supportCOVSignal;

        // AllJoyn related data
        alljoyn_busattachment m_AJBusAttachment;
        alljoyn_buslistener m_AJBusListener;
        alljoyn_sessionportlistener m_AJSessionPortListener;
        alljoyn_sessionlistener m_AJsessionListener;

        std::string m_RootStringForAllJoynNames;
        std::string m_ServiceName;

        // about service
        AllJoynAbout m_about;

        // About Icon
        AllJoynAboutIcon m_icon;

        // An optional Alljoyn Control Panel
        ControlPanel* m_pControlPanel;
    };
}

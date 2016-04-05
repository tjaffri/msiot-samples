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

#include <mutex>
#include "IAdapter.h"
#include "IJsAdapter.h"

using namespace Bridge;

namespace AdapterLib
{
    //
    // Adapter class.
    // Description:
    // The class that implements the IAdapter.
    //
    class JsAdapter : public IJsAdapter
    {
    public:
        JsAdapter();
        virtual ~JsAdapter();

		static std::shared_ptr<JsAdapter> SingleInstance();

        //
        // Adapter information
        //
        std::string Vendor() override { return this->vendor; }
        std::string AdapterName() override { return this->adapterName; }
        std::string Version() override { return this->version; }
        std::string ExposedAdapterPrefix() override { return this->exposedAdapterPrefix; }
        std::string ExposedApplicationName() override { return this->exposedApplicationName; }
        std::string ExposedApplicationGuid() override { return this->exposedApplicationGuid; }

        // Adapter signals
        AdapterSignalVector Signals() override { return this->signals; }

        uint32_t SetConfiguration(_In_ const std::vector<uint8_t>& configurationData) override;
        uint32_t GetConfiguration(_Out_ std::vector<uint8_t>& configurationDataPtr) override;

        uint32_t Initialize() override;
        uint32_t Shutdown() override;

        uint32_t EnumDevices(
            _In_ ENUM_DEVICES_OPTIONS options,
            _Out_ AdapterDeviceVector& deviceListPtr,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) override;

        uint32_t GetProperty(
            _Inout_ std::shared_ptr<IAdapterProperty>& aProperty,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) override;
        uint32_t SetProperty(
            _In_ std::shared_ptr<IAdapterProperty> aProperty,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) override;

        uint32_t GetPropertyValue(
            _In_ std::shared_ptr<IAdapterDevice> device,
            _In_ std::shared_ptr<IAdapterProperty> aProperty,
            _In_ std::string attributeName,
            _Out_ std::shared_ptr<IAdapterValue>& valuePtr,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) override;
        uint32_t SetPropertyValue(
			_In_ std::shared_ptr<IAdapterDevice> device,
			_In_ std::shared_ptr<IAdapterProperty> aProperty,
			_In_ std::shared_ptr<IAdapterValue> value,
			_Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
			) override;

        uint32_t CallMethod(
			_In_ std::shared_ptr<IAdapterDevice> device,
			_Inout_ std::shared_ptr<IAdapterMethod>& method,
			_Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
			) override;

        uint32_t RegisterSignalListener(
			_In_ std::shared_ptr<IAdapterSignal> signal,
			_In_ std::shared_ptr<IAdapterSignalListener> listener,
			_In_opt_ intptr_t listenerContext = 0
			) override;
        uint32_t UnregisterSignalListener(
			_In_ std::shared_ptr<IAdapterSignal> signal,
			_In_ std::shared_ptr<IAdapterSignalListener> listener
			) override;

        // Load the given javascript and associate it with the given device by calling 'initDevice' function
        void AddDevice(_In_ std::shared_ptr<IAdapterDevice>& device, _In_ const std::string& baseTypeXml, _In_ const std::string& jsScript, _In_ const std::string& jsModulesPath) override;

		void SetAdapterError(std::shared_ptr<IJsAdapterError>& adaptErr) override;

    protected:
        //
        //  Routine Description:
        //      NotifySignalListener is called by the Adapter to notify a registered
        //      signal listener of an intercepted signal.
        //
        //  Arguments:
        //
        //      Signal - The signal object to notify listeners.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid signal object.
        //
        uint32_t NotifySignalListener(
            _In_ std::shared_ptr<IAdapterSignal> signal
            );

        //
        //  Routine Description:
        //      NotifyDeviceArrived is called by the Adapter to notify arrival
        //      of a new device.
        //
        //  Arguments:
        //
        //      Device - The object for the device that recently arrived.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid device object
        //
        uint32_t NotifyDeviceArrival(
            _In_ std::shared_ptr<IAdapterDevice> Device
            );

        //
        //  Routine Description:
        //      NotifyDeviceRemoval is called by the Adapter to notify removal
        //      of a device.
        //
        //  Arguments:
        //
        //      Device - The object for the device that is recently removed.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid device object
        //
        uint32_t NotifyDeviceRemoval(
            _In_ std::shared_ptr<IAdapterDevice> Device
            );

    private:
        void CreateSignals();

		static std::shared_ptr<JsAdapter> g_TheOneOnlyInstance;
		
		std::string vendor;
        std::string adapterName;
        std::string version;
        // the prefix for AllJoyn service should be something like
        // com.mycompany (only alpha num and dots) and is used by the Device System Bridge
        // as root string for all services and interfaces it exposes
        std::string exposedAdapterPrefix;

        // name and GUID of the DSB/Adapter application that will be published on AllJoyn
        std::string exposedApplicationName;
        std::string exposedApplicationGuid;

        // Devices
        std::vector<IAdapterDevice> devices;

        // Signals
        std::vector<std::shared_ptr<IAdapterSignal> > signals;

        // Sync object
        std::mutex _mutex;

        //
        // Signal listener entry
        //
        struct SIGNAL_LISTENER_ENTRY
        {
            SIGNAL_LISTENER_ENTRY(
				std::shared_ptr<IAdapterSignal> SignalToRegisterTo,
				std::shared_ptr<IAdapterSignalListener> ListenerObject,
				intptr_t ListenerContext
                )
                : Signal(SignalToRegisterTo)
                , Listener(ListenerObject)
                , Context(ListenerContext)
            {
            }

            // The  signal object
			std::shared_ptr<IAdapterSignal> Signal;

            // The listener object
			std::shared_ptr<IAdapterSignalListener> Listener;

            //
            // The listener context that will be
            // passed to the signal handler
            //
			intptr_t Context;
        };

        // A map of signal handle (object's hash code) and related listener entry
        std::multimap<int, SIGNAL_LISTENER_ENTRY> signalListeners;

        // Device Arrival and Device Removal Signal Indices
        static const int DEVICE_ARRIVAL_SIGNAL_INDEX = 0;
        static const int DEVICE_ARRIVAL_SIGNAL_PARAM_INDEX = 0;
        static const int DEVICE_REMOVAL_SIGNAL_INDEX = 1;
        static const int DEVICE_REMOVAL_SIGNAL_PARAM_INDEX = 0;

		std::shared_ptr<IJsAdapterError> adapterError;
    };
} // namespace AdapterLib

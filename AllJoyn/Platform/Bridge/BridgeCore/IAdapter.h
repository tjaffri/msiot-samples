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

#include "AdapterDefinitions.h"
#include "IControlPanelHandler.h"
#include "PropertyValue.h"

namespace Bridge
{
    //Access type enum
    enum class E_ACCESS_TYPE
    {
        ACCESS_READ,
        ACCESS_WRITE,
        ACCESS_READWRITE
    };

    //Type of Change of Value Signal emitted
    enum class SignalBehavior
    {
        Unspecified,         //COV signal emit unreliable
        Never,               //Never emits COV
        Always,              //Emits COV on value change
        AlwaysWithNoValue    //emits COV without the value
    };

    //
    // IAdapterValue interface.
    // Description:
    //  Interface of an Adapter Value information.
    //
    class IAdapterValue
    {
    public:
        // Object name
        virtual std::string Name() = 0;

        virtual PropertyValue& Data() = 0;
    };

    //
    // IAdapterAttribute interface.
    // Description:
    //  Interface of an Adapter Attribute information.
    //
    class IAdapterAttribute
    {
    public:
        // Adapter Value Object
        virtual std::shared_ptr<IAdapterValue> Value() = 0;

        // Annotations for the property
        //
        // Annotations are the list of name-value pairs that represent
        // the metadata or additional information for the attribute
        // Example: min value, max value, range, units, etc.
        //
        // These values do not change for different instances of the same
        // property interface.
        //
        // If the value remains unchanged for the lifetime of an object, however,
        // can be different for different objects, then the value cannot be
        // represented as an annotation. In such case, it must be a separate attribute
        // for the same property interface with the COVBehavior as SignalBehavior::Never
        //
        virtual AnnotationMap Annotations() = 0;

        // Access
        virtual E_ACCESS_TYPE Access() = 0;
        virtual void Access(E_ACCESS_TYPE accessType) = 0;

        // Change of Value (COV) signal supported
        //
        // The following values are supported:
        //
        // Unspecified      : The COV signal behavior is not consistent.
        //                    Used when the signal is raised sometimes but not always.
        // Never            : The COV signal is never raised.
        //                    Used for constant/read only attributes.
        // Always           : The COV signal is raised whenever the attribute changes.
        //                    The new value is included in the payload for the signal.
        // AlwaysWithNoValue : The COV signal is raised whenever the attribute changes.
        //                    However, the new value is not included as part of the signal payload.
        //
        virtual SignalBehavior COVBehavior() = 0;
        virtual void COVBehavior(SignalBehavior covBehavior) = 0;
    };

    //
    // IAdapterProperty interface.
    // Description:
    //  Interface of an Adapter Property information.
    //
    class IAdapterProperty
    {
    public:
        // Object name
        virtual std::string Name() = 0;

        // hint for the interface name
        virtual std::string InterfaceHint() = 0;

        // The bag of attributes
        virtual AdapterAttributeVector Attributes() = 0;
    };

    class IAdapter;

    //
    // IAdapterMethod interface.
    // Description:
    //  Interface of an Adapter Method information.
    //
    class IAdapterMethod
    {
    public:
        // Object name
        virtual std::string Name() = 0;

        // Method description
        virtual std::string Description() = 0;
        virtual void Description(std::string value) = 0;

        // The input parameters
        virtual AdapterValueVector& InputParams() = 0;

        // The output parameters
        virtual AdapterValueVector& OutputParams() = 0;

        virtual std::shared_ptr<IAdapter> Parent() = 0;
        virtual void Parent(std::shared_ptr<IAdapter> value) = 0;

        // Optional addition data associated with this method (serialized)
        virtual intptr_t Context() = 0;
        virtual void Context(intptr_t value) = 0;
    };

    //
    // IAdapterSignal interface.
    // Description:
    //  Interface of an Adapter Signal information.
    //
    class IAdapterSignal
    {
    public:
        // Object name
        virtual std::string Name() = 0;

        // The signal parameters
        virtual AdapterValueVector Params() = 0;

        // Add paramter to the signal.
        virtual void AddParamsValue(std::shared_ptr<IAdapterValue> value) = 0;

        // Clear parameters from signal
		virtual void ClearParams() = 0;

		virtual int GetHashCode() = 0;
    };

    //
    // IAdapterIcon interface.
    // Description:
    //      Supporting interface for the AllJoyn About Icon.  An ICON is associated with AdapterDevice.
    //      An ICON may consist of a small image not exceeding 128K bytes, or and URL or both.
    //      The MIME Type *MUST* be present.  If both an image and and URL are present, then the
    //      MIME type MUST be the same for both.
    //
    //
    class IAdapterIcon
    {
    public:
        // An Array of Image Data
        virtual std::vector<uint8_t> GetImage() = 0;

        // The Mime Type of the Image data e.g. image/png, etc
        // Return empty-string/null if not used.
        virtual std::string MimeType() = 0;

        // An Optional URL to that points to an About Icon.
        // Return empty-string/null if not used
        virtual std::string Url() = 0;
    };

    //
    // IAdapterDevice interface.
    // Description:
    //  Interface of an Adapter Device information.
    //
    class IAdapterDevice
    {
    public:
        // Object name
        virtual std::string Name() = 0;
        virtual void Name(std::string value) = 0;

        //
        // Device information
        //
        virtual std::string Vendor() = 0;
        virtual void Vendor(std::string value) = 0;

        virtual std::string Model() = 0;
        virtual void Model(std::string value) = 0;

        virtual std::string Version() = 0;
        virtual void Version(std::string value) = 0;

        virtual std::string FirmwareVersion() = 0;
        virtual void FirmwareVersion(std::string value) = 0;

        virtual std::string SerialNumber() = 0;
        virtual void SerialNumber(std::string value) = 0;

        virtual std::string Description() = 0;
        virtual void Description(std::string value) = 0;

        // Additional values for devices as a JSON string
        virtual std::string Props() = 0;
        virtual void Props(std::string value) = 0;

        // used internally to pass interface XML around
        virtual std::string BaseTypeXML() = 0;
        virtual void BaseTypeXML(std::string value) = 0;

        // used internally to pass context to script
        virtual intptr_t HostContext() = 0;
        virtual void HostContext(intptr_t value) = 0;

        // Device properties
        virtual AdapterPropertyVector Properties() = 0;

        // Device methods
        virtual AdapterMethodVector Methods() = 0;

        // Device signals
        virtual AdapterSignalVector Signals() = 0;
        virtual void Signals(AdapterSignalVector value) = 0;

        virtual std::shared_ptr<IAdapterIcon> Icon() = 0;
        virtual void Icon(std::shared_ptr<IAdapterIcon> value) = 0;

        void AddMethod(std::shared_ptr<IAdapterMethod> method);
        virtual void RaiseSignal(std::string signalName) = 0;
        virtual void RaiseSignal(std::string signalName, PropertyValue param) = 0;

        void* GetInterface(int id);

		virtual int GetHashCode() = 0;
	};

    //
    // IAdapterDeviceControlPanel interface.
    // Description:
    //  IAdapterDevice extension for supporting
    //  a Control Panel
    //
    class IAdapterDeviceControlPanel
    {
    public:
        virtual std::shared_ptr<IControlPanelHandler> ControlPanelHandler() = 0;
    };

    //
    // IAdapterIoRequest interface.
    // Description:
    //  Interface of an Adapter IoRequest.
    //
    class IAdapterIoRequest
    {
    public:
        //
        //  Routine Description:
        //      Status is called by the Bridge component to get the current IO
        //      request status.
        //
        //  Arguments:
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid request handle.
        //      ERROR_IO_PENDING: The request is pending.
        //      ERROR_REQUEST_ABORTED: Request was canceled, or wait was aborted.
        //
        virtual uint32_t Status() = 0;

        //
        //  Routine Description:
        //      Wait is called by the Bridge component to wait for a pending
        //      request.
        //
        //  Arguments:
        //
        //      TimeoutMsec - Timeout period to wait for request completion in mSec.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid request handle.
        //      ERROR_TIMEOUT: Timeout period has expired.
        //      ERROR_REQUEST_ABORTED: Request was canceled, or wait was aborted.
        //
        virtual uint32_t Wait(uint32_t TimeoutMsec) = 0;

        //
        //  Routine Description:
        //      Cancel is called by the Bridge component to cancel a pending
        //      request.
        //
        //  Arguments:
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid request handle.
        //      ERROR_NOT_CAPABLE: Cannot cancel the request at this point.
        //
        virtual uint32_t Cancel() = 0;

        //
        //  Routine Description:
        //      Release is called by the Bridge component to release a request.
        //
        //  Arguments:
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid request handle.
        //
        virtual uint32_t Release() = 0;
    };

    //
    // IAdapterAsyncRequest interface.
    // Description:
    //  Extension of IAdapterIoRequest that supports continuation for improved asyncronicity.
    //
    class IAdapterAsyncRequest : public IAdapterIoRequest
    {
    public:
        //
        //  Routine Description:
        //      Continue is called by the Bridge component to schedule a callback to be
        //      invoked when the request fails or completes.
        //
        //      Only a single continuation may be set on a request. If the continuation is set after
        //      the request has completed or failed perhaps because the request completed or failed
        //      synchronously, then the continuation will still get invoked (immediately). The
        //      continuation will NOT get invoked if request is cancelled.
        //
        //  Arguments:
        //      continuationHandler - a callback invoked when the request fails or completes.
        //
        //    Callback Argument:
        //      The status of the request so far; the value that would have been returned by Status()
        //      if not for the continuation.
        //
        //    Callback Return Value:
        //      The updated status of the request; the new value that will be returned by Status().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_NOT_CAPABLE: Cannot set a continuation at this time.
        //
        virtual uint32_t Continue(std::function<uint32_t(uint32_t)> continuationHandler) = 0;
    };

    //
    // ENUM_DEVICES_OPTIONS EnumDevice options.
    // Description:
    //  CACHE_ONLY - Get cached device list.
    //  FORCE_REFRESH - Re-enumerate device list.
    //
    enum class ENUM_DEVICES_OPTIONS
    {
        CACHE_ONLY = 0,
        FORCE_REFRESH
    };

    class IAdapterSignalListener;

    //
    // IAdapter interface class.
    // Description:
    //  A pure virtual calls that defines the DSB Adapter interface.
    //  All current and future DSB Adapters implement this interface
    //
    class IAdapter
    {
    public:
        //
        // Adapter information:
        //
        virtual std::string Vendor() = 0;
        virtual std::string AdapterName() = 0;
        virtual std::string Version() = 0;
        virtual std::string ExposedAdapterPrefix() = 0;
        virtual std::string ExposedApplicationName() = 0;
        virtual std::string ExposedApplicationGuid() = 0;

        // Adapter signals
        virtual AdapterSignalVector Signals() = 0;

        //
        //  Routine Description:
        //      SetConfiguration is called by the Bridge component to set the adapter configuration.
        //
        //  Arguments:
        //
        //      ConfigurationData - byte array that holds the new adapter configuration.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_BAD_CONFIGURATION: Bad configuration.
        //      ERROR_SUCCESS_RESTART_REQUIRED: Changes were successfully updated, but will
        //          take affect after adapter has been restarted.
        //      ERROR_SUCCESS_REBOOT_REQUIRED:Changes were successfully updated, but will
        //          take affect after machine has been rebooted.
        //
        virtual uint32_t SetConfiguration(_In_ const std::vector<uint8_t>& configurationData) = 0;

        //
        //  Routine Description:
        //      GetConfiguration is called by the Bridge component to get the current adapter configuration.
        //
        //  Arguments:
        //
        //      ConfigurationDataPtr - Address of an Array var to receive the current
        //          adapter configuration.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation.
        //
        virtual uint32_t GetConfiguration(_Out_ std::vector<uint8_t>& configurationDataPtr) = 0;

        //
        //  Routine Description:
        //      Initialize is called by the Bridge component to initialize the adapter.
        //
        //  Arguments:
        //
        //
        //  Return Value:
        //      ERROR_SUCCESS, or error code specific to the cause the failed the
        //          initialization process.
        //
        virtual uint32_t Initialize() = 0;

        //
        //  Routine Description:
        //      Shutdown is called by the Bridge component to shutdown the adapter.
        //      All adapter resources are freed.
        //
        //  Arguments:
        //
        //
        //  Return Value:
        //      ERROR_SUCCESS, or error code specific to the cause the failed the
        //          shutdown process.
        //
        virtual uint32_t Shutdown() = 0;

        //
        //  Routine Description:
        //      EnumDevices is called by the Bridge component to get/enumerate adapter device list.
        //      The caller can either get the cached list, which the adapter maintains, or force the
        //      adapter to refresh its device list.
        //
        //  Arguments:
        //
        //      Options - Please refer to ENUM_DEVICES_OPTIONS supported values.
        //
        //      DeviceListPtr - Reference to a caller IAdapterDeviceVector^ car to
        //          receive the list of devices references.
        //
        //      RequestPtr - Reference to a caller IAdapterIoRequest^ variable to receive the
        //          request object, or nullptr for synchronous IO.
        //          If the request cannot be completed immediately, the caller can wait for
        //          its completion using WaitRequest() method, or cancel it using CancelRequest().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_IO_PENDING: The request is still in progress.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to complete the operation,
        //      or other error code related to the underlying stack.
        //
        virtual uint32_t EnumDevices(
            _In_ ENUM_DEVICES_OPTIONS options,
            _Out_ AdapterDeviceVector& deviceListPtr,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) = 0;

        //
        //  Routine Description:
        //      GetProperty is called by the Bridge component to re-read an adapter device property.
        //
        //  Arguments:
        //
        //      Property - The target property, that the caller
        //          has previously acquired by using EnumDevices().
        //          The entire property (information and attributes) is re-read from the
        //          from the device.
        //
        //      RequestPtr - Reference to a caller IAdapterIoRequest^ variable to receive the
        //          request object, or nullptr for synchronous IO.
        //          If the request cannot be completed immediately, the caller can wait for
        //          its completion using WaitRequest() method, or cancel it using CancelRequest().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_IO_PENDING: The request is still in progress.
        //      ERROR_INVALID_HANDLE: Invalid device/property handle.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation,
        //      or other error code related to the underlying stack.
        //
        virtual uint32_t GetProperty(
            _Inout_ std::shared_ptr<IAdapterProperty>& aProperty,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) = 0;

        //
        //  Routine Description:
        //      SetProperty is called by the Bridge component to set/write an adapter device property.
        //
        //  Arguments:
        //
        //      Property- The target property, that the caller
        //          has previously acquired by using EnumDevices().
        //
        //      ValueString - Property value in string format
        //
        //      RequestPtr - Reference to a caller IAdapterIoRequest^ variable to receive the
        //          request object, or nullptr for synchronous IO.
        //          If the request cannot be completed immediately, the caller can wait for
        //          its completion using WaitRequest() method, or cancel it using CancelRequest().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_IO_PENDING: The request is still in progress.
        //      ERROR_INVALID_HANDLE: Invalid device/property handle.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation,
        //      or other error code related to the underlying stack.
        //
        virtual uint32_t SetProperty(
            _In_ std::shared_ptr<IAdapterProperty> aProperty,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) = 0;

        //
        //  Routine Description:
        //      GetPropertyValue is called by the Bridge component to read a specific value of
        //      an adapter device property.
        //
        //  Arguments:
        //
        //      PropertyHandle - The desired property handle, that the caller
        //          has previously acquired by using EnumDevices().
        //
        //      AttributeName - The name for the desired attribute (IAdapterValue) of the Property.
        //
        //      ValuePtr - Reference to a caller IAdapterValue to receive the
        //          current value from the device.
        //
        //      RequestPtr - Reference to a caller IAdapterIoRequest^ variable to receive the
        //          request object, or nullptr for synchronous IO.
        //          If the request cannot be completed immediately, the caller can wait for
        //          its completion using WaitRequest() method, or cancel it using CancelRequest().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_IO_PENDING: The request is still in progress.
        //      ERROR_INVALID_HANDLE: Invalid property handle.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation,
        //      or other error code related to the underlying stack.
        //
        virtual uint32_t GetPropertyValue(
            _In_ std::shared_ptr<IAdapterDevice> device,
            _In_ std::shared_ptr<IAdapterProperty> aProperty,
            _In_ std::string attributeName,
            _Out_ std::shared_ptr<IAdapterValue>& valuePtr,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) = 0;

        //
        //  Routine Description:
        //      SetPropertyValue is called by the Bridge component to write a specific value of
        //      an adapter device property.
        //
        //  Arguments:
        //
        //      Property - The target property, that the caller
        //          has previously acquired by using EnumDevices().
        //
        //      Value - The value to be set.
        //
        //      RequestPtr - Reference to a caller IAdapterIoRequest^ variable to receive the
        //          request object, or nullptr for synchronous IO.
        //          If the request cannot be completed immediately, the caller can wait for
        //          its completion using WaitRequest() method, or cancel it using CancelRequest().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_IO_PENDING: The request is still in progress.
        //      ERROR_INVALID_HANDLE: Invalid device/property handle.
        //      ERROR_INVALID_PARAMETER: Invalid adapter property value information.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation,
        //      or other error code related to the underlying stack.
        //
        virtual uint32_t SetPropertyValue(
            _In_ std::shared_ptr<IAdapterDevice> device,
            _In_ std::shared_ptr<IAdapterProperty> aProperty,
            _In_ std::shared_ptr<IAdapterValue> value,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) = 0;

        //
        //  Routine Description:
        //      CallMethod is called by the Bridge component to call a device method
        //
        //  Arguments:
        //
        //      Method - The method to call.
        //          Caller needs to set the input parameters before calling CallMethod().
        //
        //      RequestPtr - Address of a caller allocated IAdapterIoRequest variable to receive the
        //          request handle, or nullptr for synchronous IO.
        //          If the request cannot be completed immediately, the caller can wait for
        //          its completion using WaitRequest() method, or cancel it using CancelRequest().
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_IO_PENDING: The request is still in progress.
        //      ERROR_INVALID_HANDLE: Invalid device.
        //      ERROR_INVALID_PARAMETER: Invalid adapter method information.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation,
        //      or other error code related to the underlying stack.
        //
        virtual uint32_t CallMethod(
            _In_ std::shared_ptr<IAdapterDevice> device,
            _Inout_ std::shared_ptr<IAdapterMethod>& method,
            _Out_opt_ std::shared_ptr<IAdapterIoRequest>& requestPtr
            ) = 0;

        //
        //  Routine Description:
        //      RegisterSignalListener is called by the Bridge component to register for
        //      intercepting device generated signals.
        //
        //  Arguments:
        //
        //      Signal - The device signal the caller wishes to listen to,
        //          that was previously acquired by using EnumDevices().
        //
        //      Listener - The listener object.
        //
        //      ListenerContext - An optional context that will be passed to the signal
        //          handler.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_HANDLE: Invalid device/signal handles.
        //      ERROR_NOT_ENOUGH_MEMORY : Not enough resources to completed the operation.
        //
        virtual uint32_t RegisterSignalListener(
            _In_ std::shared_ptr<IAdapterSignal> signal,
            _In_ std::shared_ptr<IAdapterSignalListener> listener,
            _In_opt_ intptr_t listenerContext = 0
            ) = 0;

        //
        //  Routine Description:
        //      UnregisterSignalListener is called by the Bridge component to unregister from
        //      intercepting device generated signals.
        //
        //  Arguments:
        //
        //      Signal - The signal the caller wishes to stop listening to.
        //
        //      ListenerPtr - The listener object.
        //
        //  Return Value:
        //      ERROR_SUCCESS,
        //      ERROR_INVALID_PARAMETER: The caller is not registered as a signal listener.
        //
        virtual uint32_t UnregisterSignalListener(
            _In_ std::shared_ptr<IAdapterSignal> signal,
            _In_ std::shared_ptr<IAdapterSignalListener> listener
            ) = 0;
    };

    //
    // IAdapterSignalListener interface.
    // Description:
    //  Any object who needs to intercept adapter signals implements IAdapterSignalListener
    //  and implements AdapterSignalHandler() method.
    //
    class IAdapterSignalListener
    {
    public:
        virtual void AdapterSignalHandler(
            _In_ std::shared_ptr<IAdapter> sender,
            _In_ std::shared_ptr<IAdapterSignal> signal,
            _In_opt_ intptr_t context = 0
            ) = 0;
    };
}

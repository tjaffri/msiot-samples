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

namespace Bridge
{
    // TODO: This file should not be used by BridgeNET, there should be an interface instead for the small pieces it needs
    class BridgeDevice;

    //
    // AdapterValue.
    // Description:
    // The class that implements Bridge::IAdapterValue.
    //
    class AdapterValue : public Bridge::IAdapterValue
    {
    public:
        AdapterValue() = default;
        AdapterValue(
            std::string name,
            PropertyValue data);

        AdapterValue(const AdapterValue&) = default;
        AdapterValue(AdapterValue&&) = default;

        AdapterValue& operator=(const AdapterValue&) = default;
        AdapterValue& operator=(AdapterValue&&) = default;

        //
        // Generic for Adapter objects
        //
        virtual std::string Name() { return this->name; }

        // Data
        virtual PropertyValue& Data() { return this->data; }

    // internal:

        uint32_t Set(IAdapterValue* Other);

    private:
        // Generic
        std::string name;
        PropertyValue data;
    };


    //
    // AdapterProperty.
    // Description:
    // The class that implements Bridge::IAdapterProperty.
    //
    class AdapterProperty : public Bridge::IAdapterProperty
    {
    public:
        AdapterProperty() = default;
        AdapterProperty(std::string ObjectName, std::string IfHint);

        AdapterProperty(const AdapterProperty&) = default;
        AdapterProperty(AdapterProperty&&) = default;

        AdapterProperty& operator=(const AdapterProperty&) = default;
        AdapterProperty& operator=(AdapterProperty&&) = default;

        //
        // Generic for Adapter objects
        //
        virtual std::string Name() { return this->name; }

        virtual std::string InterfaceHint() { return this->interfaceHint; }

        // Attributes
        virtual AdapterAttributeVector Attributes() { return AdapterAttributeVector(this->attributes); }

    // internal:

        uint32_t Set(Bridge::IAdapterProperty* Other);


        AdapterProperty& AddAttribute(std::shared_ptr<IAdapterAttribute> attr) { this->attributes.emplace_back(attr); return *this; }

    private:
        // Generic
        std::string name;
        std::string interfaceHint;

        AdapterAttributeVector attributes;
    };

    //
    // AdapterAttribute.
    // Description:
    //  The class that implements Bridge::IAdapterAttribute.
    //
    class AdapterAttribute : public Bridge::IAdapterAttribute
    {
    public:
        AdapterAttribute() = default;
        AdapterAttribute(
            std::string ObjectName,
            Bridge::E_ACCESS_TYPE accessType,
            PropertyValue data
            );

        AdapterAttribute(const AdapterAttribute&) = default;
        AdapterAttribute(AdapterAttribute&&) = default;

        AdapterAttribute& operator=(const AdapterAttribute&) = default;
        AdapterAttribute& operator=(AdapterAttribute&&) = default;

        //Value
        virtual std::shared_ptr<Bridge::IAdapterValue> Value() { return this->value; }

        // Annotations
        virtual AnnotationMap Annotations() { return this->annotations; }

        // Access
        virtual Bridge::E_ACCESS_TYPE Access() { return access; }
        virtual void Access(E_ACCESS_TYPE accessType) { access = accessType; }

        //Change of Value signal supported
        virtual Bridge::SignalBehavior COVBehavior() { return this->covBehavior; }
        virtual void COVBehavior(SignalBehavior behavior) { this->covBehavior = behavior; }

    private:
        // Generic
        std::shared_ptr<AdapterValue> value;

        AnnotationMap annotations;
        Bridge::E_ACCESS_TYPE access = Bridge::E_ACCESS_TYPE::ACCESS_READ;    // By default - Read access only
        Bridge::SignalBehavior covBehavior = Bridge::SignalBehavior::Never;
    };

    //
    // AdapterMethod.
    // Description:
    // The class that implements Bridge::IAdapterMethod.
    //
    class AdapterMethod : public Bridge::IAdapterMethod
    {
    public:
        AdapterMethod() = default;
        AdapterMethod(std::string ObjectName);

        AdapterMethod(const AdapterMethod&) = default;
        AdapterMethod(AdapterMethod&&) = default;

        AdapterMethod& operator=(const AdapterMethod&) = default;
        AdapterMethod& operator=(AdapterMethod&&) = default;

        // Object name
        virtual std::string Name() { return name; }

        // Method description
        virtual std::string Description() { return this->description; }
        virtual void Description(std::string value) { description = value; }

        // The input parameters
        virtual AdapterValueVector& InputParams() { return inParams; }

        // The output parameters
        virtual AdapterValueVector& OutputParams() { return outParams; }

        // Optional runtime object associated with this method
        virtual intptr_t Context() { return context; }
        virtual void Context(intptr_t value) { context = value; }

        // Optional parent adapter in case we want to redirect who handles the Invoke.
        virtual std::shared_ptr<IAdapter> Parent() { return parent; }
        virtual void Parent(std::shared_ptr<IAdapter> value) { parent = value; }

        // Adding parameters
        void AddInputParam(std::shared_ptr<Bridge::IAdapterValue> InParameter)
        {
            this->inParams.emplace_back(InParameter);
        }
        void AddOutputParam(std::shared_ptr<Bridge::IAdapterValue> OutParameter)
        {
            this->outParams.emplace_back(OutParameter);
        }

    private:
        // Generic
        std::string name;

        // Method information
        std::string description;

        // Method parameters
        AdapterValueVector inParams;
        AdapterValueVector outParams;
		intptr_t context;
        std::shared_ptr<IAdapter> parent;
    };


    //
    // AdapterSignal.
    // Description:
    // The class that implements Bridge::IAdapterSignal.
    //
    class AdapterSignal : public Bridge::IAdapterSignal
    {
    public:
        AdapterSignal() = default;
        explicit AdapterSignal(std::string ObjectName);

        AdapterSignal(const AdapterSignal&) = default;
        AdapterSignal(AdapterSignal&&) = default;

        AdapterSignal& operator=(const AdapterSignal&) = default;
        AdapterSignal& operator=(AdapterSignal&&) = default;

        //
        // Generic for Adapter objects
        //
        virtual std::string Name() { return name; }

        // Signal parameters
        virtual AdapterValueVector Params() { return params; }

        // Add paramter to the signal.
        virtual void AddParamsValue(std::shared_ptr<IAdapterValue> value) { AddParameter(value); }

        // Clear parameters from signal
        virtual void ClearParams()
        {
            params.clear();
        }

        // Adding signal parameters
        void AddParameter(std::shared_ptr<IAdapterValue> Parameter)
        {
            this->params.push_back(Parameter);
        }

		virtual int GetHashCode()
		{
			std::hash<std::string> str_hash;
			size_t hash = str_hash(Name());
			return (int)hash;
		}
    private:
        // Generic
        std::string name;

        AdapterValueVector params;
    };


    //
    // DEVICE_DESCRIPTOR
    // Description:
    // A device descriptor.
    //
    struct DEVICE_DESCRIPTOR
    {
        // The device name
        std::string           Name;

        // The device manufacturer name
        std::string           VendorName;

        // The device model name
        std::string           Model;

        // The device version number
        std::string           Version;

        // The device serial number
        std::string           SerialNumer;

        // The specific device description
        std::string           Description;
    };


    //
    // AdapterDevice.
    // Description:
    // The class that implements Bridge::IAdapterDevice.
    //
    class AdapterDevice : public Bridge::IAdapterDevice, public Bridge::IAdapterDeviceControlPanel
    {
    public:
        AdapterDevice() { hostContext = 0; };
        explicit AdapterDevice(std::string ObjectName);
        explicit AdapterDevice(const DEVICE_DESCRIPTOR* DeviceDescPtr);
        //explicit AdapterDevice(const AdapterDevice* Other);

        AdapterDevice(const AdapterDevice&) = default;
        AdapterDevice(AdapterDevice&&) = default;

        AdapterDevice& operator=(const AdapterDevice&) = default;
        AdapterDevice& operator=(AdapterDevice&&) = default;

        //
        // Generic for Adapter objects
        //
        std::string Name() override { return this->name; }
        void Name(std::string value) override { this->name = value; }

        //
        // Device information
        //
        std::string Vendor() override { return this->vendor; }
        void Vendor(std::string value) override { vendor = value; }

        std::string Model() override { return this->model; }
        void Model(std::string value) override { this->model = value; }

        std::string Version() override { return this->firmwareVersion; }
        void Version(std::string value) override { firmwareVersion = value; }

        std::string FirmwareVersion() override { return this->firmwareVersion; }
        void FirmwareVersion(std::string value) override { firmwareVersion = value; }

        virtual std::string ID() { return this->serialNumber; }
        virtual void ID(std::string value) { serialNumber = value; }

        std::string SerialNumber() override { return this->serialNumber; }
        void SerialNumber(std::string value) override { serialNumber = value; }

        std::string Description() override { return this->description; }
        void Description(std::string value) override { description = value; }

        // Additional values for devices as a JSON string
        std::string Props() override { return this->props; }
        void Props(std::string value) override { props = value; }

        // used internally to pass interface XML around
        std::string BaseTypeXML() override { return this->baseTypeXML; }
        void BaseTypeXML(std::string value) override { baseTypeXML = value; }

        intptr_t HostContext() override { return this->hostContext; }
        void HostContext(intptr_t value) override { this->hostContext = value; }

        // Device properties
        AdapterPropertyVector Properties() override { return this->properties; }

        // Adding Methods
        virtual void AddMethod(std::shared_ptr<Bridge::IAdapterMethod> method) { this->methods.push_back(method); }

        virtual void RaiseSignal(std::string signalName) override;
        virtual void RaiseSignal(std::string signalName, PropertyValue param) override;

		int GetHashCode() override
		{
			std::hash<std::string> str_hash;
			size_t hash = str_hash(Name()) +
							str_hash(Vendor()) +
							str_hash(Model()) +
							str_hash(Version()) +
							str_hash(FirmwareVersion()) +
							str_hash(SerialNumber()) +
							str_hash(Description());
			return (int)hash;
		}

        virtual void* GetInterface(int /*id*/) { return nullptr; }

        // Device methods
        AdapterMethodVector Methods() override { return this->methods; }

        // Device signals
        AdapterSignalVector Signals() override { return this->signals; }
        void Signals(AdapterSignalVector value) override { signals = value; }

        // Control Panel Handler
        std::shared_ptr<IControlPanelHandler> ControlPanelHandler() override { return nullptr; }

        std::shared_ptr<Bridge::IAdapterIcon> Icon() override { return icon; }
        void Icon(std::shared_ptr<Bridge::IAdapterIcon> value) override { icon = value; }

    //internal:
        virtual std::shared_ptr<Bridge::BridgeDevice> BridgeDevice() { return this->bridgeDevice;  }
        virtual void BridgeDevice(std::shared_ptr<Bridge::BridgeDevice> value) { this->bridgeDevice = value; }

        // Adding Properties
        void AddProperty(std::shared_ptr<Bridge::IAdapterProperty> Property)
        {
            this->properties.push_back(Property);
        }

        // Adding Signals
        void AddSignal(std::shared_ptr<Bridge::IAdapterSignal> Signal)
        {
            this->signals.push_back(Signal);
        }

        // Adding Change_Of_Value Signal
        void AddChangeOfValueSignal(
            std::shared_ptr<Bridge::PropertyValue> Property,
            std::shared_ptr<Bridge::IAdapterValue> Attribute);

    private:
        // Generic
        std::string name;

        // Device information
        std::string vendor;
        std::string model;
        std::string firmwareVersion;
        std::string serialNumber;
        std::string description;
        std::string props;
        std::shared_ptr<Bridge::BridgeDevice> bridgeDevice;
        std::shared_ptr<IAdapterIcon> icon;
        std::string baseTypeXML;
        intptr_t hostContext;

        // Device properties
        AdapterPropertyVector properties;

        // Device methods
        AdapterMethodVector methods;

        // Device signals
        AdapterSignalVector signals;
    };
} // namespace AdapterLib

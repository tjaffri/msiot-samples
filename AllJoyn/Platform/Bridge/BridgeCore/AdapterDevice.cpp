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
#include "AdapterDevice.h"
#include "AdapterConstants.h"
#include "BridgeDevice.h"

namespace Bridge
{
    //
    // AdapterValue.
    // Description:
    // The class that implements Bridge::IAdapterValue.
    //
    AdapterValue::AdapterValue(
        std::string ObjectName,
        PropertyValue DefaultData
        )
        : name(ObjectName)
        , data(DefaultData)
    {
    }

    uint32_t AdapterValue::Set(IAdapterValue* Other)
    {
        this->name = Other->Name();
        this->data = Other->Data();

        return ERROR_SUCCESS;
    }


    //
    // AdapterProperty.
    // Description:
    // The class that implements Bridge::IAdapterProperty.
    //
    AdapterProperty::AdapterProperty(
        std::string ObjectName,
        std::string IfHint
        )
        : name(ObjectName)
        , interfaceHint(IfHint)
    {
    }

    
    uint32_t
    AdapterProperty::Set(IAdapterProperty* Other)
    {
        this->name = Other->Name();

        auto otherAttributes = Other->Attributes();

        if (this->attributes.size() != otherAttributes.size())
        {
            throw std::invalid_argument("Incompatible Adapter Properties");
        }

        for (size_t attrInx = 0; attrInx < otherAttributes.size(); ++attrInx)
        {
            auto attr = static_cast<AdapterValue*>(attributes[attrInx]->Value().get());

            attr->Set(otherAttributes[attrInx]->Value().get());
        }

        return ERROR_SUCCESS;
    }

    //
    // AdapterAttribute.
    // Description:
    //  The class that implements Bridge::IAdapterAttribute.
    //
    AdapterAttribute::AdapterAttribute(std::string ObjectName, E_ACCESS_TYPE accessType, PropertyValue DefaultData)
    {
		AdapterValue* aval = new AdapterValue(ObjectName, DefaultData);
		std::shared_ptr<AdapterValue> sval(aval);
		value = sval;

        Access(accessType);
    }

    //
    // AdapterMethod.
    // Description:
    // The class that implements Bridge::IAdapterMethod.
    //
    AdapterMethod::AdapterMethod(
        std::string ObjectName
        )
        : name(ObjectName)
    {
    }
    
    //
    // AdapterSignal.
    // Description:
    // The class that implements Bridge::IAdapterSignal.
    //
    AdapterSignal::AdapterSignal(
        std::string ObjectName
        )
        : name(ObjectName)
    {
    }



    //
    // AdapterDevice.
    // Description:
    // The class that implements Bridge::IAdapterDevice.
    //
    AdapterDevice::AdapterDevice(
        std::string ObjectName
        )
        : name(ObjectName)
    {
    }


    AdapterDevice::AdapterDevice(
        const DEVICE_DESCRIPTOR* DeviceDescPtr
        )
        : name(DeviceDescPtr->Name)
        , vendor(DeviceDescPtr->VendorName)
        , model(DeviceDescPtr->Model)
        , firmwareVersion(DeviceDescPtr->Version)
        , serialNumber(DeviceDescPtr->SerialNumer)
        , description(DeviceDescPtr->Description)
    {
    }

    // TODO: This leaks abstraction and should use the interface value instead
    // it should also use proper shared_ptrs throughout
    void
    AdapterDevice::AddChangeOfValueSignal(
        std::shared_ptr<PropertyValue> Property,
        std::shared_ptr<IAdapterValue> Attribute)
    {
		AdapterSignal* asig = new AdapterSignal(Constants::CHANGE_OF_VALUE_SIGNAL());
		std::shared_ptr<AdapterSignal> ssig(asig);
		auto covSignal = ssig;
		AdapterValue* aval = new AdapterValue(Constants::COV__PROPERTY_HANDLE(), *Property);
		std::shared_ptr<AdapterValue> sval(aval);
		covSignal->AddParameter(sval);
		AdapterValue* aval2 = new AdapterValue(Constants::COV__PROPERTY_HANDLE(), *Property);
		std::shared_ptr<AdapterValue> sval2(aval2);
		covSignal->AddParameter(sval2);

        this->signals.push_back(covSignal);
    }

    void AdapterDevice::RaiseSignal(
        std::string signalName)
    {
        RaiseSignal(signalName, {});
    }

    void AdapterDevice::RaiseSignal(
        std::string signalName,
        PropertyValue param)
    {
        // Raise it back to bridge device
        for (auto signal : signals)
        {
            if (signalName == signal->Name())
            {
                if (this->bridgeDevice)
                {
                    signal->ClearParams();
					AdapterValue* aval = new AdapterValue("signalParam", param);
					std::shared_ptr<AdapterValue> sval(aval);
					signal->AddParamsValue(sval);

                    this->bridgeDevice->RaiseSignal(signal);
                }
                break;
            }
        }
    }

} // namespace Bridge

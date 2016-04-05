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

#include <sstream>

#include "DeviceMain.h"
#include "DeviceSignal.h"
#include "AllJoynHelper.h"

using namespace Bridge;
using namespace std;

DeviceSignal::DeviceSignal()
    : m_adapterSignal(nullptr)
{
}

DeviceSignal::~DeviceSignal()
{
}

QStatus DeviceSignal::Initialize(_In_ DeviceMain *parent, _In_ std::shared_ptr<IAdapterSignal> adapterSignal, _In_ alljoyn_interfacedescription_member& signalDescription)
{
    QStatus status = ER_OK;

    // sanity check
    if (nullptr == parent)
    {
        status = ER_BAD_ARG_1;
        goto leave;
    }
    if (nullptr == adapterSignal)
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    m_parent = parent;
    m_adapterSignal = adapterSignal;
    m_signalDescription = signalDescription;

    // build signal name
    status = SetName(m_adapterSignal->Name());
    if (ER_OK != status)
    {
        goto leave;
    }

    /*
    // build signatures and parameter list
    status = BuildSignature();
    if (ER_OK != status)
    {
        goto leave;
    }*/

leave:
    return status;
}

void DeviceSignal::SendSignal(_In_ std::shared_ptr<IAdapterSignal> adapterSignal)
{
    QStatus status = ER_OK;
    size_t nbOfArgs = 0;
    alljoyn_msgarg args = nullptr;

    if (nullptr == adapterSignal)
    {
        // can't do anything
        return;
    }

    // create out arguments if necessary
    if ((adapterSignal->Params()).size() != 0)
    {
        args = alljoyn_msgarg_array_create((adapterSignal->Params()).size());
        if (nullptr == args)
        {
            goto leave;
        }
    }

    // set arguments
    for (auto signalParam : adapterSignal->Params())
    {
        //check if the param is of type IPropertyValue
        auto propertyValue = &signalParam->Data();

        if (PropertyType::Object == propertyValue->Type())
        {
            status = AllJoynHelper::SetMsgArgFromAdapterObject(signalParam, alljoyn_msgarg_array_element(args, nbOfArgs), m_parent);
        }
        else if (PropertyType::Empty == propertyValue->Type())
        {
            goto leave;
        }
        else
        {
            status = AllJoynHelper::SetMsgArg(signalParam, alljoyn_msgarg_array_element(args, nbOfArgs));
        }

        if (ER_OK != status)
        {
            goto leave;
        }
        nbOfArgs++;
    }

    // send signal on AllJoyn
    status = alljoyn_busobject_signal(m_parent->GetBusObject(), nullptr, ALLJOYN_SESSION_ID_ALL_HOSTED, m_signalDescription, args, nbOfArgs, 0, 0, nullptr);

leave:
    if (nullptr != args)
    {
        alljoyn_msgarg_destroy(args);
    }
}

QStatus DeviceSignal::SetName(std::string name)
{
    QStatus status = ER_OK;

    m_exposedName.clear();

    if (name.empty())
    {
        status = ER_INVALID_DATA;
        goto leave;
    }

    AllJoynHelper::EncodePropertyOrMethodOrSignalName(name, m_exposedName);
    if (!m_parent->IsSignalNameUnique(m_exposedName))
    {
        // append unique id
        std::ostringstream tempString;
        m_exposedName += '_';
        tempString << m_parent->GetIndexForSignal();
        m_exposedName += tempString.str();
    }

leave:
    return status;
}

QStatus DeviceSignal::BuildSignature()
{
    QStatus status = ER_OK;

    m_signature.clear();
    m_parameterNames.clear();

    for (auto signalParam : m_adapterSignal->Params())
    {
        std::string tempSignature;
        std::string hint;

        auto propertyValue = signalParam->Data();

		//check if the value is of type PropertyValue
		if (PropertyType::Object == propertyValue.Type())
		{
			std::shared_ptr<IAdapterProperty> tempProperty = propertyValue.Get<std::shared_ptr<IAdapterProperty>>();
			if (nullptr == tempProperty)
			{
				// wrong object type
				status = ER_BUS_BAD_SIGNATURE;
				goto leave;
			}

			// adapter object are exposed as string on AllJoyn
			tempSignature += "s";
			hint = " (bus object path)";
		}
		else if (PropertyType::Empty == propertyValue.Type())
		{
			status = ER_BUS_BAD_SIGNATURE;
			goto leave;
		}
		else
        {
            status = AllJoynHelper::GetSignature(propertyValue.Type(), tempSignature);
            if (ER_OK != status)
            {
                goto leave;
            }
            m_signature += tempSignature;
        }

        // add parameter name to parameter list
        if (0 != m_parameterNames.length())
        {
            m_parameterNames = ",";
        }
        m_parameterNames += signalParam->Name();
        m_parameterNames += hint;
    }

leave:
    return status;
}

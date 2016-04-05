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

namespace Bridge
{
    class BridgeDevice;
    class AllJoynProperty;

    class PropertyInterface
    {
    public:
        PropertyInterface();
        virtual ~PropertyInterface();

        QStatus Create(_In_ std::shared_ptr<IAdapter> adapter, _In_ std::shared_ptr<IAdapterProperty> adapterProperty, _In_ std::string &name, _In_ std::shared_ptr<BridgeDevice> device);
        bool InterfaceMatchWithAdapterProperty(_In_ std::shared_ptr<IAdapterProperty> adapterProperty);

        inline alljoyn_interfacedescription GetInterfaceDescription()
        {
            return m_interfaceDescription;
        }
        inline std::string *GetInterfaceName()
        {
            return &m_interfaceName;
        }

        bool IsAJPropertyNameUnique(_In_ std::string name);
        inline uint32_t GetIndexForAJProperty()
        {
            return m_indexForAJProperty++;
        }

        inline std::vector<AllJoynProperty *> &GetAJProperties()
        {
            return m_AJProperties;
        }

    private:
        std::shared_ptr<IAdapter> m_adapter;

        // alljoyn related
        alljoyn_interfacedescription m_interfaceDescription;
        std::string m_interfaceName;
        std::vector<AllJoynProperty *> m_AJProperties;

        uint32_t m_indexForAJProperty;
    };
}


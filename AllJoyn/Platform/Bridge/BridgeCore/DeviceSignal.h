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
    class DeviceMain;

    class DeviceSignal
    {
    public:
        DeviceSignal();
        virtual ~DeviceSignal();

        QStatus Initialize(_In_ DeviceMain *parent, std::shared_ptr<IAdapterSignal> adapterSignal, alljoyn_interfacedescription_member& signalDescription);
        void SendSignal(_In_ std::shared_ptr<IAdapterSignal> adapterSignal);
        inline std::string &GetName()
        {
            return m_exposedName;
        }

    private:
        QStatus SetName(_In_ std::string name);
        QStatus BuildSignature();

        // adapter signal
        std::shared_ptr<IAdapterSignal> m_adapterSignal;

        // alljoyn related
        std::string m_exposedName;
        std::string m_signature;
        std::string m_parameterNames;
        alljoyn_interfacedescription_member m_signalDescription;

        // parent
        DeviceMain *m_parent;
    };
}


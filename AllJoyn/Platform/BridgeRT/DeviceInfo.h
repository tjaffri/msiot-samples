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

using namespace Platform;

#define DECLARE_PROPERTY(typeName, name, store) \
    property typeName name \
    { \
        typeName get() \
        {\
            return store; \
        }\
        void set(typeName value) \
        {\
            store = value; \
        }\
    } 

namespace BridgeRT
{
    public ref class DeviceInfo sealed
    {
    public:
        DeviceInfo() {};

        DECLARE_PROPERTY(String^, Name, _Name);
        DECLARE_PROPERTY(String^, Vendor, _Vendor);
        DECLARE_PROPERTY(String^, Model, _Model);
        DECLARE_PROPERTY(String^, Version, _Version);
        DECLARE_PROPERTY(String^, FirmwareVersion, _FirmwareVersion);
        DECLARE_PROPERTY(String^, SerialNumber, _SerialNumber);
        DECLARE_PROPERTY(String^, Description, _Description);
        DECLARE_PROPERTY(String^, Props, _Props);

    private:
        String^ _Name;
        String^ _Vendor;
        String^ _Model;
        String^ _Version;
        String^ _FirmwareVersion;
        String^ _SerialNumber;
        String^ _Description;
        String^ _Props;
    };

}
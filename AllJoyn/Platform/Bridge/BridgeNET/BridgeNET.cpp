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

#include "stdafx.h"
#include <msclr\marshal_cppstd.h>
#include "BridgeNET.h"
#include "IBridge.h"
#include "AdapterDevice.h"
#include "IJsAdapter.h"

// This is defined here to avoid a build warning only.
namespace Bridge
{
    class BridgeDevice final
    {
    public:
        BridgeDevice();
    };
}

using namespace Bridge;
using namespace AdapterLib;

#define CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(left, right) \
    { \
        if (right != nullptr) \
        { \
            left(msclr::interop::marshal_as<std::string>(right)); \
        } \
    } \

namespace BridgeNet
{

    std::shared_ptr<Bridge::IAdapterDevice> CopyDeviceInfoForAdapter(DeviceInfo^ device)
    {
        Bridge::AdapterDevice* jsDevice = new Bridge::AdapterDevice();
    
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->Name, device->Name);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->Vendor, device->Vendor);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->Model, device->Model);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->Version, device->Version);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->FirmwareVersion, device->FirmwareVersion);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->SerialNumber, device->SerialNumber);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->Description, device->Description);
        CONVERT_AND_ASSIGN_STRING_IF_NOT_NULL(jsDevice->Props, device->Props);
    
        return std::shared_ptr<Bridge::IAdapterDevice>(reinterpret_cast<Bridge::IAdapterDevice*>(jsDevice));
    }

    Int32 BridgeJs::Initialize()
    {
        std::shared_ptr<IJsAdapter> scriptAdapter = IJsAdapter::Get();
        std::shared_ptr<IBridge> bridge = IBridge::Get();
        bridge->AddAdapter(scriptAdapter);
        bridge->Initialize();
    
        return S_OK;
    }

    void BridgeJs::AddDevice(DeviceInfo^ info, 
        String^ baseTypeXml, 
        String^ script, 
        String^ modulesPath)
    {
        std::shared_ptr<IJsAdapter> scriptAdapter = IJsAdapter::Get();
        std::string scriptStr = msclr::interop::marshal_as<std::string>(script);
        std::string baseXml = msclr::interop::marshal_as<std::string>(baseTypeXml);
        std::string modPath = msclr::interop::marshal_as<std::string>(modulesPath);
        std::shared_ptr<IAdapterDevice> deviceToAdd = CopyDeviceInfoForAdapter(info);
    
        return scriptAdapter->AddDevice(deviceToAdd, baseXml, scriptStr, modPath);
    }

    void BridgeJs::Shutdown()
    {
        std::shared_ptr<IJsAdapter> scriptAdapter = IJsAdapter::Get();
        std::shared_ptr<IBridge> bridge = IBridge::Get();
        scriptAdapter->Shutdown();
        bridge->Shutdown();
    }

}

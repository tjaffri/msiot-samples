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
#include "BridgeJs.h"
#include "CommonUtils.h"
#include "IBridge.h"
#include "AdapterDevice.h"
#include "IJsAdapter.h"

using namespace Bridge;
using namespace AdapterLib;

BridgeRT::BridgeJs^ g_BridgeJs = nullptr;
std::shared_ptr<IJsAdapterError> g_adaptErr = nullptr;

namespace BridgeRT
{
	// utility to copy device between RT and native
	std::shared_ptr<Bridge::IAdapterDevice> CopyAdapterDeviceForJS(_In_ DeviceInfo ^device)
	{
		Bridge::AdapterDevice* jsDevice = new Bridge::AdapterDevice();

		jsDevice->Name(StringUtils::ToUtf8(device->Name->Data()));
		jsDevice->Vendor(StringUtils::ToUtf8(device->Vendor->Data()));
		jsDevice->Model(StringUtils::ToUtf8(device->Model->Data()));
		jsDevice->Version(StringUtils::ToUtf8(device->Version->Data()));
		jsDevice->FirmwareVersion(StringUtils::ToUtf8(device->FirmwareVersion->Data()));
		jsDevice->ID(StringUtils::ToUtf8(device->SerialNumber->Data()));
		jsDevice->SerialNumber(StringUtils::ToUtf8(device->SerialNumber->Data()));
		jsDevice->Description(StringUtils::ToUtf8(device->Description->Data()));
		jsDevice->Props(StringUtils::ToUtf8(device->Props->Data()));

		return std::shared_ptr<Bridge::IAdapterDevice>(reinterpret_cast<Bridge::IAdapterDevice*>(jsDevice));
	}

	BridgeJs::BridgeJs()
	{
		g_BridgeJs = this;
	}

	int32_t BridgeJs::Initialize()
	{
		std::shared_ptr<IJsAdapter> adapter = IJsAdapter::Get();
        std::shared_ptr<IBridge> bridge = IBridge::Get();

        g_adaptErr = std::make_shared<AdapterError>();
		adapter->SetAdapterError(g_adaptErr);

		bridge->AddAdapter(adapter);
		bridge->Initialize();

		return S_OK;
	}

	void BridgeJs::AddDevice(_In_ DeviceInfo^ device, _In_ Platform::String^ baseTypeXml, _In_ Platform::String^ jsScript, _In_ Platform::String^ modulesPath)
	{
        std::shared_ptr<IJsAdapter> adapter = IJsAdapter::Get();
		std::string script(StringUtils::ToUtf8(jsScript->Data()));
        std::string baseXml(StringUtils::ToUtf8(baseTypeXml->Data()));
        std::string modPath(StringUtils::ToUtf8(modulesPath->Data()));
        std::shared_ptr<IAdapterDevice> mydevice = CopyAdapterDeviceForJS(device);

		return adapter->AddDevice(mydevice, baseXml, script, modPath);
	}

	void BridgeJs::Shutdown()
	{
        std::shared_ptr<IJsAdapter> adapter = IJsAdapter::Get();
        std::shared_ptr<IBridge> bridge = IBridge::Get();
		adapter->Shutdown();
		bridge->Shutdown();
	}

	void BridgeJs::RaiseEvent(Platform::String^ msg)
	{
		Error(this, msg);
	}

    void BridgeJs::AdapterError::ReportError(_In_ const std::string& msg)
    {
        if (g_BridgeJs != nullptr)
        {
            std::wstring wmsg(StringUtils::ToWstring(msg));
            Platform::String^ pmsg = ref new Platform::String(wmsg.c_str());
            g_BridgeJs->RaiseEvent(pmsg);
        }
    }
}
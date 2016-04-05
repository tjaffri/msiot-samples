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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "BridgeCommon.h"
#include "Bridge.h"
#include "DsbServiceNames.h"
#include "CspBridge.h"
#include "ConfigManager.h"
#include "FileAccess.h"

using namespace Bridge;
using namespace BridgePAL;

CspBridge::CspBridge()
    : AllJoynFileTransfer(),
	m_configManager(nullptr)
{
    m_tempConfigFilePath.clear();
}

CspBridge::~CspBridge()
{
}


//************************************************************************************************
//
// Initialize
//
// Creates an AllJoyn Object for this Bridge that handles Bridge Specific Configuration data.
// The bridge implements the Microsoft standard alljoyn management configuration interface.
//
//************************************************************************************************
QStatus CspBridge::Initialize(_In_ alljoyn_busattachment* bus, _In_ ConfigManager *configManager, _In_ BridgeConfig& bridgeConfig)
{
    QStatus status = ER_OK;

	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	try
    {
        m_busObjectPath = "/BridgeConfig";
    }
    catch (...)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

	m_configManager = configManager;

    try
    {
		std::string cfgFolder = fileAccess->GetConfigurationFolderPath();
		if (!cfgFolder.empty())
		{
			m_srcConfigFilePath = fileAccess->AppendPath(cfgFolder, bridgeConfig.FileName());
		}
    }
    catch (...)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    status = AllJoynFileTransfer::Initialize(bus, m_busObjectPath.c_str(), configManager);

leave:
	return status;
}

/************************************************************************************************
*
* Post File Write
*
* We transfer the bridge configuration file directly.  Not temporary file is necessary
*
************************************************************************************************/
QStatus CspBridge::PostFileWriteAction(_In_ std::wstring &appRelativeFileName, _Out_ std::shared_ptr<DsbBridge>& finalEvent)
{
	return m_configManager->SetDeviceConfig(appRelativeFileName, finalEvent);
}

//************************************************************************************************
//
// Pre File Read Handler for Bridge
//
// For the bridge, transfer the bridge configuration file directly.
// No temporary file is necessary
//
//************************************************************************************************
QStatus CspBridge::PreFileReadAction(_Out_ std::wstring &appRelativeFileName)
{
    QStatus hr = ER_OK;
	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	try
    {
        boost::uuids::uuid guid = boost::uuids::random_generator()();
        std::string strGuid = boost::uuids::to_string(guid);

		std::string cfgFolder = fileAccess->GetConfigurationFolderPath();
		if (!cfgFolder.empty())
		{
			m_tempConfigFilePath = fileAccess->AppendPath(cfgFolder, strGuid);

            appRelativeFileName = StringUtils::ToWstring(strGuid);

            std::lock_guard<std::recursive_mutex> lock(DsbBridge::SingleInstance()->GetLock());
			hr = fileAccess->CopyFile(m_srcConfigFilePath, m_tempConfigFilePath);
			if (hr != ER_OK)
			{
				fileAccess->DelFile(m_tempConfigFilePath);
			}
		}
    }
    catch (...)
    {
        hr = ER_OUT_OF_MEMORY;
    }

    return hr;
}

//************************************************************************************************
//
// Post File Read Handler.
//
//************************************************************************************************
QStatus CspBridge::PostFileReadAction(void)
{
	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	fileAccess->DelFile(m_tempConfigFilePath);
    return ER_OK;
}

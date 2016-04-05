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

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "BridgeCommon.h"
#include "DsbServiceNames.h"
#include "Bridge.h"
#include "ConfigManager.h"
#include "CspAdapter.h"
#include "FileAccess.h"

using namespace Bridge;
using namespace BridgePAL;

CspAdapter::CspAdapter()
    : AllJoynFileTransfer(),
	m_configManager(nullptr)
{
}

CspAdapter::~CspAdapter()
{
}


//************************************************************************************************
//
// Initialize
//
// Creates an AllJoyn Objects for this Bridge that handles Adapter Specific Configuration data.
// The adapter implements the microsoft standard alljoyn management configuration interface.
//
//************************************************************************************************
QStatus CspAdapter::Initialize(_In_ alljoyn_busattachment* bus, _In_ ConfigManager *configManager)
{
    QStatus status = ER_OK;

    try
    {
        m_busObjectPath = "/AdapterConfig";
    }
    catch (std::bad_alloc&)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    m_configManager = configManager;

    status = AllJoynFileTransfer::Initialize(bus, m_busObjectPath.c_str(), configManager);

leave:
    return status;
}


//************************************************************************************************
//
// Post-File-Write Handler for the Adapter
//
// Converts the temporary file to an array of bytes and pass it to the adapter
//
//************************************************************************************************
QStatus CspAdapter::PostFileWriteAction(_In_ std::wstring &appRelativeFileName, _Out_ std::shared_ptr<DsbBridge>& finalEvent)
{
    QStatus hr = ER_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    uint32_t numBytesRead = 0;
    std::vector<uint8_t> adapterConfigData;

	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	std::string cfgFolder = fileAccess->GetConfigurationFolderPath();
	std::string absoluteFileName("");

    // default post file write event to nothing
    finalEvent.reset();

    // Create the full file name and path to the source directory
    absoluteFileName = cfgFolder;
    absoluteFileName += StringUtils::ToUtf8(appRelativeFileName);

    // Figure out how large the new temporary config file is (also verifies that the file is present)
	int64_t filesize = fileAccess->GetFileSize(absoluteFileName);
    if (filesize == -1L)
    {
        hr = ER_FAIL;
        goto leave;
    }

	// assume that size fits in int32
    // Create a temporary array for holding the adapter configuration data
	adapterConfigData.reserve(filesize);

    // Open a handle to the temporary data file
    hFile = fileAccess->OpenFileForRead(absoluteFileName);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = ER_FAIL;
        goto leave;
    }

    // Read the entire contents of the temporary data file
    if (!fileAccess->ReadFromFile(hFile, adapterConfigData.data(), filesize, &numBytesRead))
    {
        hr = ER_FAIL;
        goto leave;
    }

    // If the number of bytes read does not equal the expected number of bytes then this was unexpected.
    if (numBytesRead != filesize)
    {
        hr = ER_FAIL;
        goto leave;
    }

    // Pass the configuration data to the adapter.
    hr = m_configManager->SetAdapterConfig(adapterConfigData);

leave:
    if (INVALID_HANDLE_VALUE != hFile)
    {
		fileAccess->CloseFile(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }
	return hr;
}



//************************************************************************************************
//
// Pre-File-Read Handler for the Adapter
//
// Make a temporary file of the current adapter configuration settings and provide filename
// to the caller
//
//************************************************************************************************
QStatus CspAdapter::PreFileReadAction(_Out_ std::wstring &appRelativeFileName)
{
    QStatus hr = ER_OK;
    boost::uuids::uuid guid = boost::uuids::random_generator()();
    std::string strGuid = boost::uuids::to_string(guid);

	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	std::string cfgFolder = fileAccess->GetConfigurationFolderPath();
	HANDLE hFile = INVALID_HANDLE_VALUE;
	std::vector<uint8_t> adapterConfigData;
    uint32_t numBytesWritten=0;
    appRelativeFileName.clear();
    m_readFileNameFullPath.clear();

    // Get the adapter configuration file
    hr = m_configManager->GetAdapterConfig(adapterConfigData);
    if (QS_FAILED(hr))
    {
        goto leave;
    }

    appRelativeFileName = StringUtils::ToWstring(strGuid);
    // Create the full file name and path to the source directory
    m_readFileNameFullPath = cfgFolder;
    m_readFileNameFullPath += strGuid;

    // create 'new' temp file
    hFile = fileAccess->OpenFileForWrite(m_readFileNameFullPath, CREATE_ALWAYS);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = ER_FAIL;
        goto leave;
    }

    // Write all adapter data to the temporary file
    if (!fileAccess->WriteToFile(hFile, adapterConfigData.data(), adapterConfigData.size(), &numBytesWritten))
    {
        hr = ER_FAIL;
        goto leave;
    }

leave:

    if (hFile != INVALID_HANDLE_VALUE)
    {
		fileAccess->CloseFile(hFile);
        hFile = INVALID_HANDLE_VALUE;

        if (QS_FAILED(hr))
        {
			fileAccess->DelFile(m_readFileNameFullPath);
            appRelativeFileName.clear();
            m_readFileNameFullPath.clear();
        }
    }

    return hr;
}


//************************************************************************************************
//
// Delete the temporary configuration file
//
//************************************************************************************************
QStatus CspAdapter::PostFileReadAction(void)
{
	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());
	fileAccess->DelFile(m_readFileNameFullPath);
    return ER_OK;
}

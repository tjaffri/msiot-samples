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
#include "BridgeConfig.h"
#include "FileAccess.h"
#include "XmlUtil.h"

using namespace BridgePAL;

// required for ndk-build (Android)
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(x) (void)(x)
#endif

#pragma warning(disable:4127)

//static const char BRIDGE_CONFIG_PATH[] = "/BridgeConfig";
static const char OBJECTS_NODE_PATH[] = "/BridgeConfig/AdapterDevices";

static const char ALLJOYN_OBJECT_PATH_PREFIX[] = "/BridgeConfig/AdapterDevices/Device[@Id=\"";
static const char ALLJOYN_OBJECT_PATH_SUFFIX[] = "\"]";
static const char OBJECT_NODE[] = "Device";
static const char VISIBLE_ATTR[] = "Visible";
static const char OBJECT_ID_ATTR[] = "Id";
static const char DESC_NODE[] = "Desc";
//static const char ROOT_NODE[] = "./";
//static const char TEXT_NODE[] = "/text()";

//static const char SETTINGS_NODE_PATH[] = "/BridgeConfig/Settings";
//static const char SETTINGS_NODE_NAME[] = "Settings";
static const char SETTINGS_BRIDGE_KEYX_PATH[] = "/BridgeConfig/Settings/Bridge/KEYX";
static const char SETTINGS_DEVICE_DEFAULT_VIS_PATH[] = "/BridgeConfig/Settings/Device/DefaultVisibility";
static const char SETTINGS_DEVICE_KEYX_PATH[] = "/BridgeConfig/Settings/Device/KEYX";
static const char SETTINGS_DEVICE_USERNAME_PATH[] = "/BridgeConfig/Settings/Device/USERNAME";
static const char SETTINGS_DEVICE_PASSWORD_PATH[] = "/BridgeConfig/Settings/Device/PASSWORD";
static const char SETTINGS_DEVICE_ECDHE_ECDSA_PRIVATEKEY_PATH[] = "/BridgeConfig/Settings/Device/ECDHEECDSAPRIVATEKEY";
static const char SETTINGS_DEVICE_ECDHE_ECDSA_CERTCHAIN_PATH[] = "/BridgeConfig/Settings/Device/ECDHEECDSACERTCHAIN";

// The bridge does not use a persisted configuration
// This may be added in the future

// for now, specify a fixed XML, though it is ignored
static const char DEFAULT_BRIDGE_CFG_XML[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<BridgeConfig>\n"
"  <Settings>\n"
"    <Bridge>\n"
"       <KEYX></KEYX>\n"
"    </Bridge>\n"
"    <Device>\n"
"       <DefaultVisibility>true</DefaultVisibility>\n"
"       <KEYX></KEYX>\n"
"       <USERNAME></USERNAME>\n"
"       <PASSWORD></PASSWORD>\n"
"       <ECDHEECDSAPRIVATEKEY></ECDHEECDSAPRIVATEKEY>\n"
"       <ECDHEECDSACERTCHAIN></ECDHEECDSACERTCHAIN>\n"
"    </Device>\n"
"  </Settings>\n"
"  <AdapterDevices>\n"
"  </AdapterDevices>\n"
"</BridgeConfig>\n";

BridgeConfig::BridgeConfig()
{
}

BridgeConfig::~BridgeConfig()
{
}

_Use_decl_annotations_
QStatus
BridgeConfig::Init(std::string pFileName, bool failIfNotPresent)
{
    QStatus hr = ER_OK;

	FileAccessFactory faf;
	std::unique_ptr<IFileAccess> fileAccess(faf.GetFileAccess());

    // If a filename was specified, try to initialize the configuration document from the specified name
    if (!pFileName.empty())
    {
        //  Does the specified file exist?
		std::string cfgFolder = fileAccess->GetConfigurationFolderPath();
		if (!cfgFolder.empty())
		{
			std::string cfgpath = fileAccess->AppendPath(cfgFolder, pFileName);
			if (!fileAccess->IsFilePresent(cfgpath))
			{
				//Not present. see if it is present in appxpackage
				std::string insFolder = fileAccess->GetPackageInstallFolderPath();
				if (!insFolder.empty())
				{
					std::string inspath = fileAccess->AppendPath(insFolder, pFileName);
					if (fileAccess->IsFilePresent(inspath))
					{
						//copy the file
						auto srcFolder = insFolder;
						auto destFolder = cfgFolder;
						// TODO: make async
						fileAccess->CopyFile(inspath, cfgpath);
					}
				}
			}

			if (!fileAccess->IsFilePresent(cfgpath))
			{
				// The file doesn't exist, so return a failure if caller wanted to fail, or initialize a default
				if (failIfNotPresent)
				{
					hr = ER_FAIL;
					goto CleanUp;
				}

				CHK_QS(initXml());
				CHK_QS(ToFile(pFileName));
			}
			// Otherwise the file is loaded
			else
			{
				// Try to read file
				CHK_QS(FromFile(pFileName));
			}
		}
    }

CleanUp:
    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::FromFile(std::string pFileName)
{
    QStatus hr = ER_OK;

    // init config from XML file
	m_XmlUtil.LoadFromFile(pFileName);

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::FromString(std::string xmlString)
{
    QStatus hr = ER_OK;

    // init config from XML string
    m_XmlUtil.LoadFromString(xmlString);

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::ToFile(std::string pFileName)
{
    QStatus hr = ER_OK;

    // save config as XML file
	// TODO: implement

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::ToString(std::string* xmlString)
{
    QStatus hr = ER_OK;

    // TODO: implement 
    UNREFERENCED_PARAMETER(xmlString);

    return hr;
}

QStatus
BridgeConfig::initXml()
{
    QStatus hr = ER_OK;
    std::string target(DEFAULT_BRIDGE_CFG_XML);

    m_XmlUtil.LoadFromString(DEFAULT_BRIDGE_CFG_XML);

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::FindObject(std::string id, DsbObjectConfig& object)
{
    QStatus hr = ER_OK;
    std::string objectPathIdAttr;
	std::string objectPathPrefix(ALLJOYN_OBJECT_PATH_PREFIX);
	std::string objectPathSuffix(ALLJOYN_OBJECT_PATH_SUFFIX);
	std::string visibleAttr(VISIBLE_ATTR);
	std::string descNode(DESC_NODE);

    // Get config values for object from XML

	// TODO: implement 
    UNREFERENCED_PARAMETER(object);

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::AddObject(const DsbObjectConfig& object)
{
    QStatus hr = ER_OK;
	std::string objectsRoot(OBJECTS_NODE_PATH);
	std::string visibleAttr(VISIBLE_ATTR);
	std::string objNodeName(OBJECT_NODE);
	std::string objIdAttr(OBJECT_ID_ATTR);
	std::string descNode(DESC_NODE);

    std::string objIdVal = object.id;

    // Add object to configuration XML
	// TODO: implement 

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::RemoveObject(std::string id)
{
    QStatus hr = ER_OK;
    std::string objectPathName;
	std::string objectsRoot(OBJECTS_NODE_PATH);
	std::string objectPathPrefix(ALLJOYN_OBJECT_PATH_PREFIX);
	std::string objectPathSuffix(ALLJOYN_OBJECT_PATH_SUFFIX);
	std::string visibleAttr(VISIBLE_ATTR);

    // Remove object from configuration XML

    // TODO: implement 

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::MergeFrom(BridgeConfig& src)
{
    QStatus hr = ER_OK;
    std::string failedSrcElemId;
	std::string objectsRoot(OBJECTS_NODE_PATH);
	std::string objIdAttr(OBJECT_ID_ATTR);

    // Merge configuration
	// TODO: implement 
    UNREFERENCED_PARAMETER(src);

    return hr;
}

_Use_decl_annotations_
QStatus
BridgeConfig::convertXsBooleanToBool(std::string varValue, bool &bValue)
{
    QStatus hr = ER_OK;
    const char NUM_TRUE[] = "1";
    const char WORD_TRUE[] = "true";
    const char WORD_FALSE[] = "false";

    if (strcmp(NUM_TRUE, varValue.c_str()) == 0 || strcmp(WORD_TRUE, varValue.c_str()) == 0)
    {
        bValue = true;
    }
    else if (strcmp(WORD_FALSE, varValue.c_str()) == 0 || strcmp(WORD_TRUE, varValue.c_str()) == 0)
    {
        bValue = false;
    }
    else
    {
        hr = ER_FAIL;
    }

    return hr;
}

std::string
BridgeConfig::BridgeKeyX()
{
    return GetCredentialValue(SETTINGS_BRIDGE_KEYX_PATH);
}

std::string
BridgeConfig::DeviceKeyX()
{
    return GetCredentialValue(SETTINGS_DEVICE_KEYX_PATH);
}

std::string
BridgeConfig::DeviceUsername()
{
    return GetCredentialValue(SETTINGS_DEVICE_USERNAME_PATH);
}

std::string
BridgeConfig::DevicePassword()
{
	return GetCredentialValue(SETTINGS_DEVICE_PASSWORD_PATH);
}

std::string
BridgeConfig::DeviceEcdheEcdsaPrivateKey()
{
    return GetCredentialValue(SETTINGS_DEVICE_ECDHE_ECDSA_PRIVATEKEY_PATH);
}

std::string
BridgeConfig::DeviceEcdheEcdsaCertChain()
{
    return GetCredentialValue(SETTINGS_DEVICE_ECDHE_ECDSA_CERTCHAIN_PATH);
}

std::string
BridgeConfig::GetCredentialValue(const char* xmlPath)
{
    std::string result("");
    // Create XPATH Query.
    // Try to find specified node relative to the current node
	std::string queryString(xmlPath);
	result = m_XmlUtil.GetNodeValue(xmlPath);

    return result;
}

bool BridgeConfig::DefaultVisibility()
{
    bool bDefaultVisibility = false;

    // Create XPATH Query.
    // Try to find specified node relative to the current node
	std::string queryString(SETTINGS_DEVICE_DEFAULT_VIS_PATH);
	std::string val = m_XmlUtil.GetNodeValue(queryString);
	if (!val.empty())
	{
		QStatus hr = convertXsBooleanToBool(val, bDefaultVisibility);
		if (QS_FAILED(hr))
		{
			bDefaultVisibility = false;
		}
	}

    return bDefaultVisibility;
}

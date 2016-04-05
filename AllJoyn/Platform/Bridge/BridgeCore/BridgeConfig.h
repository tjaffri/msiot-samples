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

#include "XmlUtil.h"

struct DsbObjectConfig
{
    std::string id;
    bool bVisible;
    std::string description;

    bool operator==(const DsbObjectConfig& rhs)
    {
        if (&rhs == this)
        {
            return true;
        }

        return ((id == rhs.id) &&
            (bVisible == rhs.bVisible) &&
            (description == rhs.description));
    }

    bool operator!=(const DsbObjectConfig& rhs)
    {
        if (&rhs == this)
        {
            return false;
        }

        return ((id != rhs.id) ||
            (bVisible != rhs.bVisible) ||
            (description != rhs.description));
    }
};


class BridgeConfig
{
public:
    BridgeConfig();
    virtual ~BridgeConfig();

    //******************************************************************************************************
    //
    //  Initialize this object
    //
    //  pFileName:			Relative path and filename to initialize from, or null.
    //
    //  faileIfNotPresent:  If a file name is specified and the file is not detected, fail, instead
    //                      of calling the default initialization method.  Ignored if filename is not
    //                      specified
    //
    //  If the file specified by pFileName is not found, this routine will try to create a default
    //  file with the specified name at the specified path.  If the file specified by pFileName is found,
    //  the file will be schema validated and loaded.  In either case, the filename is cached within
    //  this object and does not need to be specified in the ToFile function when the configuration is updated.
    //
    //  If the file specified by pFileName is null, a later call to FromFile or FromString is required.
    //
    //******************************************************************************************************
    QStatus Init(_In_ std::string pFileName, bool failIfNotPresent = false);

    //******************************************************************************************************
    //
    //  Loads this configuration object from the specified XML file using the
    //
    //  pFileName:   Fully qualified path and filename
    //
    //******************************************************************************************************
    QStatus FromFile(_In_ std::string pFileName);

    //******************************************************************************************************
    //
    //  Loads this configuration object from the specified String.
    //
    //  xmlString:  A complete XML configuration.
    //
    //******************************************************************************************************
    QStatus FromString(_In_ std::string xmlString);

    //******************************************************************************************************
    //
    //  Saves the current in memory XML file to disk
    //
    //  pFileName:  The file name to save this configuration object to.  Can be null if filename was
    //              specified by earlier call to Init.
    //
    //******************************************************************************************************
    QStatus ToFile(_In_ std::string pFileName = nullptr);

    //******************************************************************************************************
    //
    //  Converts the in memory XML configuration to a string
    //
    //  xmlString:  The returned string-ized version of the current XML document
    //
    //******************************************************************************************************
    QStatus ToString(_Out_ std::string* xmlString);

    //******************************************************************************************************
    //
    //	Find the object with the given id.
    //
    //	id: ID of the device object
    //
    //	object: corresponding device object returned from the xml
    //
    //******************************************************************************************************
    QStatus FindObject(_In_ std::string id, _Out_ DsbObjectConfig& object);

    //******************************************************************************************************
    //
    //	Add object into the xml configurations.
    //
    //	object: the device object to be added
    //
    //******************************************************************************************************
    QStatus AddObject(_In_ const DsbObjectConfig& object);

    //******************************************************************************************************
    //
    //	Remove the object with the given id.
    //
    //	id: ID of the device object
    //
    //******************************************************************************************************
    QStatus RemoveObject(_In_ std::string id);


    //******************************************************************************************************
    //
    //	Merge an input xml document into the current in-memory xml document
    //
    //	src: source xml document
    //
    //******************************************************************************************************
    QStatus MergeFrom(_In_ BridgeConfig& src);


    //******************************************************************************************************
    //
    //	Returns App Local relative file name managed by this Bridge Configuration XML File
    //
    //	returns: null if not initialized, full Path and File Name of Configuration file managed by this
    //           instance
    //
    //******************************************************************************************************
    std::string FileName()
    {
        return m_fileName;
    }

	//******************************************************************************************************
	//
	//	Returns the Security KeyX for configuration/bridge (if configured)
	//
	//	returns: empty string if not configured, a KeyX value if present
	//
	//******************************************************************************************************
    std::string BridgeKeyX();

    //******************************************************************************************************
    //
    //	Returns the Security KeyX for device (if configured)
    //
    //	returns: empty string if not configured, a KeyX value if present
    //
    //******************************************************************************************************
    std::string DeviceKeyX();

    //******************************************************************************************************
	//
	//	Returns the Security Username for device (if configured)
	//
	//	returns: empty string if not configured, a username value if present
	//
	//******************************************************************************************************
	std::string DeviceUsername();

	//******************************************************************************************************
	//
	//	Returns the Security Password for device (if configured)
	//
	//	returns: empty string if not configured, a password value if present
	//
	//******************************************************************************************************
	std::string DevicePassword();

    //******************************************************************************************************
    //
    //	Returns the Security ECDHE ECDSA private key for device (if configured)
    //
    //	returns: empty string if not configured, a ECDHE ECDSA private key value if present
    //
    //******************************************************************************************************
    std::string DeviceEcdheEcdsaPrivateKey();

    //******************************************************************************************************
    //
    //	Returns the Security ECDHE ECDSA CERT chain for device (if configured)
    //
    //	returns: empty string if not configured, a ECDHE ECDSA CERT chain value if present
    //
    //******************************************************************************************************
    std::string DeviceEcdheEcdsaCertChain();

    //******************************************************************************************************
    //
    //	Returns the default visibility to apply for objects found by the adapter
    //
    //	returns: true if new objects found by the adapter should appear on the alljoyn bus be default,
    //			 false if not set or if configured as false.
    //
    //******************************************************************************************************
    bool DefaultVisibility();


private:
    //******************************************************************************************************
    //
    //	Loads default adapter configuration xml.
    //
    //******************************************************************************************************
    QStatus initXml();

    //******************************************************************************************************
    //
    //  Helper method for handling boolean XML data vailes
    //
    //  varValue:   Variant value read from the XML file.
    //  bool:       The boolean value read from the file.
    //
    //******************************************************************************************************
    QStatus convertXsBooleanToBool(_In_ std::string varValue, _Out_ bool &bValue);

    //******************************************************************************************************
    //
    //	Returns value of a specific credential item, e.g.: PIN, USERNAME, PASSWORD... (if configured)
    //
    //	returns: empty string if not configured, a pin value if present
    //
    //******************************************************************************************************
    std::string GetCredentialValue(const char* xmlPath);

    //
    //  The current XML Document
    //
#ifdef USINGASYNC
	XmlDocument m_pXmlDocument;
#else
	XmlUtil m_XmlUtil;
#endif

    //
    //  Relative Path and File Name
    //
    std::string m_fileName;
};
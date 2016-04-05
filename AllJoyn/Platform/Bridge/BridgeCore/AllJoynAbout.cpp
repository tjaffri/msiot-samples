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
#include "PropertyValue.h"
#include "DsbServiceNames.h"
#include "AllJoynAbout.h"

#include "alljoyn_c/AjAPI.h"

using namespace Bridge;

#define APPDATA_CONTAINER_DSB_SETTINGS  "DSBSettings"
#define DSB_SETTING_DEVICE_ID           "ID"

static const char DEFAULT_LANGUAGE_FOR_ABOUT[] = "en";
static const char UNKNOWN_ADAPTER[] = "Unknown device";
static const char UNKNOWN_MANUFACTURER[] = "Unknown";
static const char UNKNOWN_VERSION[] = "0.0.0.0";
static const char DSB_DEFAULT_APP_NAME[] = "Device System Bridge";
static const char DSB_DEFAULT_MODEL[] = "DSB";
static const char DSB_DEFAULT_DESCRIPTION[] = "Device System Bridge";
static const char DSB_DEFAULT_APP_GUID[] = "EF116A26-9888-47C2-AE85-B77142F24EFA";

AllJoynAbout::AllJoynAbout()
    : m_aboutData(nullptr),
    m_aboutObject(nullptr),
    m_isAnnounced(false)
{
}

AllJoynAbout::~AllJoynAbout()
{
    ShutDown();
}

QStatus AllJoynAbout::Initialize(_In_ alljoyn_busattachment bus)
{
    QStatus status = ER_OK;

    // sanity check
    if (nullptr == bus)
    {
        status = ER_BAD_ARG_1;
        goto leave;
    }

    // create the about object that is used to communicate about data
    // note that the about interface won't be part of the announce
    m_aboutObject = alljoyn_aboutobj_create(bus, UNANNOUNCED);
    if (nullptr == m_aboutObject)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    // create about data with default language
    m_aboutData = alljoyn_aboutdata_create(DEFAULT_LANGUAGE_FOR_ABOUT);
    if (nullptr == m_aboutData)
    {
        status = ER_OUT_OF_MEMORY;
        goto leave;
    }

    // fill about data with default value
    status = SetDefaultAboutData();

leave:
    if (ER_OK != status)
    {
        ShutDown();
    }

    return status;
}

void AllJoynAbout::ShutDown()
{
    if (nullptr != m_aboutObject)
    {
        if (m_isAnnounced)
        {
            alljoyn_aboutobj_unannounce(m_aboutObject);
        }
        alljoyn_aboutobj_destroy(m_aboutObject);
        m_aboutObject = nullptr;
        m_isAnnounced = false;
    }
    if (nullptr != m_aboutData)
    {
        alljoyn_aboutdata_destroy(m_aboutData);
        m_aboutData = nullptr;
    }
}

QStatus AllJoynAbout::Announce(alljoyn_sessionport sp)
{
    QStatus status = ER_OK;

    // sanity check
    if (nullptr == m_aboutObject)
    {
        status = ER_INIT_FAILED;
        goto leave;
    }

    if (!alljoyn_aboutdata_isvalid(m_aboutData, DEFAULT_LANGUAGE_FOR_ABOUT))
    {
        status = ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
        goto leave;
    }

    status = alljoyn_aboutobj_announce(m_aboutObject, sp, m_aboutData);
    if (ER_OK != status)
    {
        goto leave;
    }

    m_isAnnounced = true;

leave:
    return status;
}

QStatus AllJoynAbout::AddObject(_In_ alljoyn_busobject busObject, _In_ const alljoyn_interfacedescription interfaceDescription)
{
    return alljoyn_busobject_setannounceflag(busObject, interfaceDescription, ANNOUNCED);
}

QStatus AllJoynAbout::RemoveObject(_In_ alljoyn_busobject busObject, _In_ const alljoyn_interfacedescription interfaceDescription)
{
    return alljoyn_busobject_setannounceflag(busObject, interfaceDescription, UNANNOUNCED);
}

QStatus AllJoynAbout::SetManufacturer(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setmanufacturer(m_aboutData, value, nullptr);
}

QStatus AllJoynAbout::SetDeviceName(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setdevicename(m_aboutData, value, nullptr);
}

QStatus AllJoynAbout::SetSWVersion(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setsoftwareversion(m_aboutData, value);
}

QStatus AllJoynAbout::SetHWVersion(_In_z_ const char* value)
{
    return alljoyn_aboutdata_sethardwareversion(m_aboutData, value);
}

QStatus AllJoynAbout::SetDeviceId(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setdeviceid(m_aboutData, value);
}

QStatus AllJoynAbout::SetModel(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setmodelnumber(m_aboutData, value);
}

QStatus AllJoynAbout::SetDescription(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setdescription(m_aboutData, value, nullptr);
}

QStatus AllJoynAbout::SetApplicationName(_In_z_ const char* value)
{
    return alljoyn_aboutdata_setappname(m_aboutData, value, nullptr);
}

QStatus AllJoynAbout::SetApplicationGuid(_In_ const char* value)
{
    return alljoyn_aboutdata_setappid_fromstring(m_aboutData, value);
}

QStatus AllJoynAbout::SetDefaultAboutData()
{
    QStatus status = ER_OK;
    std::string deviceId;
    std::string deviceName;

    // only set the required fields:
    // - DefaultLanguage (already set upon m_aboutData creation)
    // - DeviceId
    // - DeviceName
    // - AppId
    // - AppName
    // - Manufacturer
    // - ModelNumber
    // - Description
    // - SoftwareVersion

    // default device ID to bridge device Id
    CHK_AJSTATUS( GetDeviceID(deviceId) );
    CHK_AJSTATUS( alljoyn_aboutdata_setdeviceid(m_aboutData, deviceId.c_str()));

    // default about data to bridge about data
    CHK_AJSTATUS(SetDeviceName(UNKNOWN_ADAPTER));
    CHK_AJSTATUS(SetSWVersion(UNKNOWN_VERSION));
    CHK_AJSTATUS(SetDescription(DSB_DEFAULT_DESCRIPTION));
    CHK_AJSTATUS(SetApplicationGuid(DSB_DEFAULT_APP_GUID));
    CHK_AJSTATUS(SetApplicationName(DSB_DEFAULT_APP_NAME));
    CHK_AJSTATUS(SetManufacturer(UNKNOWN_MANUFACTURER));
    CHK_AJSTATUS(SetModel(DSB_DEFAULT_MODEL));


    if (!alljoyn_aboutdata_isvalid(m_aboutData, DEFAULT_LANGUAGE_FOR_ABOUT))
    {
        status = ER_ABOUT_ABOUTDATA_MISSING_REQUIRED_FIELD;
    }

leave:
    return status;
}

QStatus AllJoynAbout::ReadDeviceID(_Out_ std::string &deviceId)
{
    // The bridge does not use a persisted configuration
    // This may be added in the future
    // return a failure. Handled by the calling routine.

    QStatus status = ER_FAIL;

    // empty output string
    deviceId.clear();

    return status;
}

QStatus AllJoynAbout::CreateAndSaveDeviceID(_Out_ std::string &deviceId)
{
    // The bridge does not use a persisted configuration
    // This may be added in the future

    QStatus status = ER_OK;

    // empty output string
    deviceId.clear();

    boost::uuids::uuid guid = boost::uuids::random_generator()();
    deviceId = boost::uuids::to_string(guid);

    if (ER_OK != status)
    {
        // reset device Id in case of error
        deviceId.clear();
    }
    return status;
}

QStatus AllJoynAbout::GetDeviceID(_Out_ std::string &deviceId)
{
    QStatus status = ER_OK;
    std::string tempId;

    // reset out param
    deviceId.clear();

    // read device Id (create it if necessary)
    status = ReadDeviceID(tempId);
    if (status != ER_OK)
    {
        status = CreateAndSaveDeviceID(tempId);
        if (status != ER_OK)
        {
            goto leave;
        }
    }

    //convert types
    deviceId = tempId;

leave:
    return status;
}


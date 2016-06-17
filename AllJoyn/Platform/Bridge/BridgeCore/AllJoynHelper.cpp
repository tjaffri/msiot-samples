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
#include "AllJoynHelper.h"
#include "DeviceMain.h"
#include "AdapterDevice.h"
#include "PropertyValue.h"

#include <numeric>

using namespace Bridge;
using namespace std;

AllJoynHelper::AllJoynHelper()
{
}

string AllJoynHelper::TrimChar(const string& inString, char ch)
{
    auto posL = inString.find_first_not_of(ch);
    if (posL == string::npos)
    {
        posL = 0;
    }

    auto posR = inString.find_last_not_of(ch);

    return inString.substr(posL, posR - posL + 1);
}

void AllJoynHelper::EncodeBusObjectName(_In_ std::string inString, _Inout_ std::string &builtName)
{
    builtName.clear();
    builtName.reserve(inString.size());
    // only use a-z A-Z 0-9 char
    // translate ' ' into '_'
    for (size_t index = 0; index < inString.size(); index++)
    {
        auto origChar = inString[index];
        if (::isalnum(origChar))
        {
            builtName += origChar;
        }
        else if (::isspace(origChar) || '_' == origChar)
        {
            builtName += '_';
        }
        else if ('.' == origChar || '/' == origChar)
        {
            builtName += '/';
        }
    }

    //Trim '/' from start and end
    builtName = TrimChar(builtName, '/');
}

void AllJoynHelper::EncodePropertyOrMethodOrSignalName(_In_ std::string inString, _Inout_ std::string &builtName)
{
    builtName.clear();
    builtName.reserve(inString.size());
    // disabling this case changing: 1st char must be upper case => default to true
    bool upperCaseNextChar = false;
    bool is1stChar = true;

    // only use a-z A-Z 0-9
    // upper case 1st letter of each word (non alpha num char are considered as word separator)
    // 1st char must be a letter
    for (size_t index = 0; index < inString.size(); index++)
    {
        auto origChar = inString[index];
        if ((::isalnum(origChar) && !is1stChar) ||
            (::isalpha(origChar) && is1stChar))
        {
            if (upperCaseNextChar && ::isalpha(origChar) && ::islower(origChar))
            {
                builtName += (char)::toupper(origChar);
            }
            else
            {
                builtName += origChar;
            }
            upperCaseNextChar = false;
            is1stChar = false;
        }
        else
        {
            // disabling this case changing: new word coming => upper case its 1st letter
            upperCaseNextChar = false;
        }
    }
}

void AllJoynHelper::EncodeStringForInterfaceName(std::string  inString, std::string & encodeString)
{
    encodeString.clear();

    // only keep alpha numeric char and '.'
    for (size_t index = 0; index < inString.size(); index++)
    {
        auto origChar = inString[index];
        if (::isalnum(origChar) || '.' == origChar)
        {
            encodeString += origChar;
        }
    }

    //Trim '.' from start and end
    encodeString = TrimChar(encodeString, '.');
}

void AllJoynHelper::EncodeStringForServiceName(_In_ std::string inString, _Out_ std::string &encodeString)
{
    string tempString;

    encodeString.clear();

    // only use alpha numeric char
    for (size_t index = 0; index < inString.size(); index++)
    {
        auto origChar = inString[index];
        if (::isalnum(origChar))
        {
            tempString += origChar;
        }
    }

    if (!tempString.empty())
    {
        // add '_' at the beginning if encoded value start with a number
        if (::isdigit(tempString[0]))
        {
            encodeString += '_';
        }

        encodeString += tempString;
    }
}

void AllJoynHelper::EncodeStringForRootServiceName(std::string  inString, std::string & encodeString)
{
    char currentChar = '\0';

    encodeString.clear();

    // only keep alpha numeric char and '.'
    for (size_t index = 0; index < inString.size(); index++)
    {
        auto origChar = inString[index];
        if (::isalpha(origChar) || ('.' == origChar))
        {
            encodeString += origChar;
            currentChar = origChar;
        }
        else if (::isdigit(origChar))
        {
            // add '_' before digit if digit immediately follow '.'
            if ('.' == currentChar)
            {
                encodeString += "_";
            }
            encodeString += origChar;
            currentChar = origChar;
        }
    }
    //Trim '.' from start and end
    encodeString = TrimChar(encodeString, '.');
}

void AllJoynHelper::EncodeStringForAppName(std::string  inString, std::string & encodeString)
{
    encodeString.clear();

    // only use alpha numeric char
    for (size_t index = 0; index < inString.size(); index++)
    {
        auto origChar = inString[index];
        if (::isalnum(origChar))
        {
            encodeString += origChar;
        }
    }
}

QStatus AllJoynHelper::SetMsgArg(_In_ std::shared_ptr<IAdapterValue> adapterValue, _Inout_ alljoyn_msgarg msgArg)
{
    QStatus status = ER_OK;
    string signature;
    auto& propertyValue = adapterValue->Data();
    if (propertyValue.Type() == PropertyType::Empty)
    {
        status = ER_BUS_BAD_VALUE;
        goto leave;
    }

    //get the signature
    status = GetSignature(propertyValue.Type(), signature);
    if (status != ER_OK)
    {
        goto leave;
    }

    switch (propertyValue.Type())
    {
    case PropertyType::Boolean:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<bool>());
        break;

    case PropertyType::UInt8:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<uint8_t>());
        break;

    case PropertyType::Char16:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), static_cast<int16_t>(propertyValue.Get<char16_t>()));
        break;

    case PropertyType::Int16:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<int16_t>());
        break;

    case PropertyType::UInt16:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<uint16_t>());
        break;

    case PropertyType::Int32:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<int32_t>());
        break;

    case PropertyType::UInt32:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<uint32_t>());
        break;

    case PropertyType::Int64:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<int64_t>());
        break;

    case PropertyType::UInt64:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<uint64_t>());
        break;

    case PropertyType::Double:
        status = alljoyn_msgarg_set(msgArg, signature.c_str(), propertyValue.Get<double>());
        break;

    case PropertyType::String:
    {
        string tempString = propertyValue.Get<std::string>();
        if (0 != tempString.length())
        {
            status = alljoyn_msgarg_set_and_stabilize(msgArg, signature.c_str(), tempString.c_str());
        }
        else
        {
            // set empty string
            status = alljoyn_msgarg_set(msgArg, signature.c_str(), "");
        }
        break;
    }
    case PropertyType::BooleanArray:
    {
        auto boolArray = propertyValue.Get<std::vector<bool>>();
        status = SetMsgArg(msgArg, signature.c_str(), boolArray);
        break;
    }
    case PropertyType::UInt8Array:
    {
        auto byteArray = propertyValue.Get<std::vector<uint8_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), byteArray);
        break;
    }
    case PropertyType::Char16Array:
    {
        auto charArray = propertyValue.Get<std::vector<char16_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), charArray);
        break;
    }
    case PropertyType::Int16Array:
    {
        auto intArray = propertyValue.Get<std::vector<int16_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), intArray);
        break;
    }
    case PropertyType::UInt16Array:
    {
        auto uintArray = propertyValue.Get<std::vector<uint16_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), uintArray);
        break;
    }
    case PropertyType::Int32Array:
    {
        auto intArray = propertyValue.Get<std::vector<int32_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), intArray);
        break;
    }
    case PropertyType::UInt32Array:
    {
        auto uintArray = propertyValue.Get<std::vector<uint32_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), uintArray);
        break;
    }
    case PropertyType::Int64Array:
    {
        auto intArray = propertyValue.Get<std::vector<int64_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), intArray);
        break;
    }
    case PropertyType::UInt64Array:
    {
        auto uintArray = propertyValue.Get<std::vector<uint64_t>>();
        status = SetMsgArg(msgArg, signature.c_str(), uintArray);
        break;
    }
    case PropertyType::DoubleArray:
    {
        auto doubleArray = propertyValue.Get<std::vector<double>>();
        status = SetMsgArg(msgArg, signature.c_str(), doubleArray);
        break;
    }
    case PropertyType::StringArray:
    {
        auto strArray = propertyValue.Get<std::vector<std::string>>();
        status = SetMsgArg(msgArg, signature.c_str(), strArray);
        break;
    }
    case PropertyType::StringDictionary:
    {
        auto strDict = propertyValue.Get<std::unordered_map<std::string, std::string>>();
        status = SetMsgArg(msgArg, signature.c_str(), strDict);
        break;
    }
    default:
        status = ER_NOT_IMPLEMENTED;
        break;
    }

leave:
    return status;
}

template<typename T>
QStatus AllJoynHelper::SetMsgArg(
    _Inout_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _In_ std::vector<T>& arrayArg)
{
    QStatus status = ER_OK;

    if (ajSignature.length() != 2 || ajSignature[0] != 'a')
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    if (!arrayArg.empty())
    {
        status = alljoyn_msgarg_set_and_stabilize(msgArg, ajSignature.c_str(), arrayArg.size(), arrayArg.data());
    }
    else
    {
        status = alljoyn_msgarg_set(msgArg, ajSignature.c_str(), 0, nullptr);
    }

leave:
    return status;
}

QStatus AllJoynHelper::SetMsgArg(
    _Inout_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _In_ std::vector<bool>& arrayArg)
{
    QStatus status = ER_OK;

    if (ajSignature != "ab")
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    if (!arrayArg.empty())
    {
        auto tempBuffer = std::make_unique<bool[]>(arrayArg.size());
#if !defined(_MSC_VER)
        std::copy(arrayArg.begin(), arrayArg.end(), tempBuffer.get());
#else
        std::copy(arrayArg.begin(), arrayArg.end(), stdext::make_checked_array_iterator(tempBuffer.get(), arrayArg.size()));
#endif
        status = alljoyn_msgarg_set_and_stabilize(msgArg, ajSignature.c_str(), arrayArg.size(), tempBuffer.get());
    }
    else
    {
        status = alljoyn_msgarg_set(msgArg, ajSignature.c_str(), 0, nullptr);
    }

leave:
    return status;
}

QStatus AllJoynHelper::SetMsgArg(
    _Inout_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _In_ std::vector<std::string>& arrayArg)
{
    QStatus status = ER_OK;

    if (ajSignature != "as")
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    if (!arrayArg.empty())
    {
        std::vector<const char*> strings;
        strings.resize(arrayArg.size());
        std::transform(arrayArg.begin(), arrayArg.end(), strings.begin(), [](const std::string& s) {return s.c_str(); });
        status = alljoyn_msgarg_set_and_stabilize(msgArg, ajSignature.c_str(), strings.size(), strings.data());
    }
    else
    {
        status = alljoyn_msgarg_set(msgArg, ajSignature.c_str(), 0, nullptr);
    }

leave:
    return status;
}

QStatus AllJoynHelper::SetMsgArg(
    _Inout_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _In_ std::unordered_map<std::string, std::string>& dictArg)
{
    QStatus status = ER_OK;

    if (ajSignature != "a{ss}")
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    if (!dictArg.empty())
    {
        alljoyn_msgarg entries = alljoyn_msgarg_array_create(dictArg.size());

        int i = 0;
        std::for_each(dictArg.begin(), dictArg.end(), [&](const std::pair<std::string, std::string>& entry) {
            if (status == ER_OK)
            {
                status = alljoyn_msgarg_set_and_stabilize(
                    alljoyn_msgarg_array_element(entries, i++), "{ss}", entry.first.c_str(), entry.second.c_str());
            }
        });

        if (status == ER_OK)
        {
            status = alljoyn_msgarg_set_and_stabilize(msgArg, ajSignature.c_str(), dictArg.size(), entries);
        }
    }
    else
    {
        status = alljoyn_msgarg_set(msgArg, ajSignature.c_str(), 0, nullptr);
    }

leave:
    return status;
}

QStatus AllJoynHelper::GetPropertyTypeFromMsgArg(_In_ alljoyn_msgarg msgArg, _Out_ PropertyType& propertyType)
{
    QStatus status = ER_OK;
    alljoyn_typeid argType = alljoyn_msgarg_gettype(msgArg);

    switch (argType)
    {
    case ALLJOYN_BOOLEAN:
        propertyType = PropertyType::Boolean;
        break;
    case ALLJOYN_DOUBLE:
        propertyType = PropertyType::Double;
        break;
    case ALLJOYN_INT32:
        propertyType = PropertyType::Int32;
        break;
    case ALLJOYN_INT16:
        propertyType = PropertyType::Int16;
        break;
    case ALLJOYN_UINT16:
        propertyType = PropertyType::UInt16;
        break;
    case ALLJOYN_STRING:
        propertyType = PropertyType::String;
        break;
    case ALLJOYN_UINT64:
        propertyType = PropertyType::UInt64;
        break;
    case ALLJOYN_UINT32:
        propertyType = PropertyType::UInt32;
        break;
    case ALLJOYN_INT64:
        propertyType = PropertyType::Int64;
        break;
    case ALLJOYN_BYTE:
        propertyType = PropertyType::UInt8;
        break;

    case ALLJOYN_ARRAY:
        {
            char sig[10];
            if (alljoyn_msgarg_signature(msgArg, sig, _countof(sig)) == 3)
            {
                switch (sig[1])
                {
                case ALLJOYN_BOOLEAN:
                    propertyType = PropertyType::BooleanArray;
                    break;
                case ALLJOYN_DOUBLE:
                    propertyType = PropertyType::DoubleArray;
                    break;
                case ALLJOYN_INT32:
                    propertyType = PropertyType::Int32Array;
                    break;
                case ALLJOYN_INT16:
                    propertyType = PropertyType::Int16Array;
                    break;
                case ALLJOYN_UINT16:
                    propertyType = PropertyType::UInt16Array;
                    break;
                case ALLJOYN_STRING:
                    propertyType = PropertyType::StringArray;
                    break;
                case ALLJOYN_UINT64:
                    propertyType = PropertyType::UInt64Array;
                    break;
                case ALLJOYN_UINT32:
                    propertyType = PropertyType::UInt32Array;
                    break;
                case ALLJOYN_INT64:
                    propertyType = PropertyType::Int64Array;
                    break;
                case ALLJOYN_BYTE:
                    propertyType = PropertyType::UInt8Array;
                    break;
                default:
                    status = ER_NOT_IMPLEMENTED;
                    break;
                }
            }
            else if (strcmp(sig, "a{ss}") == 0)
            {
                propertyType = PropertyType::StringDictionary;
            }
            else
            {
                // Other array or dictionary types aren't implemented
                status = ER_NOT_IMPLEMENTED;
            }
        }
        break;

    default:
        status = ER_NOT_IMPLEMENTED;
        break;
    }

    return status;
}


QStatus AllJoynHelper::GetAdapterValueFromMsgArg(_In_ alljoyn_msgarg msgArg, _Out_ std::shared_ptr<IAdapterValue>& adapterValue)
{
    QStatus status = ER_OK;
    string signature;
    PropertyType propertyType;

    AdapterValue* aval = new AdapterValue(std::string(), nullptr);
    std::shared_ptr<AdapterValue> sval(aval);
    adapterValue = sval;

    // Get the property type
    status = GetPropertyTypeFromMsgArg(msgArg, propertyType);
    if (status != ER_OK)
    {
        goto leave;
    }

    //get the signature
    status = GetSignature(propertyType, signature);
    if (status != ER_OK)
    {
        goto leave;
    }

    switch (propertyType)
    {
        case PropertyType::Boolean:
        {
            bool tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::UInt8:
        {
            int8_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::Char16:  __fallthrough;
        case PropertyType::Int16:
        {
            int16_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::UInt16:
        {
            uint16_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::Int32:
        {
            int32_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::UInt32:
        {
            uint32_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::Int64:
        {
            int64_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::UInt64:
        {
            uint64_t tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::Double:
        {
            double tempVal;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
            if (ER_OK == status)
            {
                adapterValue->Data() = tempVal;
            }
            break;
        }

        case PropertyType::String:
        {
            char *tempChar = nullptr;
            status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempChar);
            if (ER_OK == status)
            {
                // convert char to wide char, create Platform::String
                // and update value
                adapterValue->Data() = std::string(tempChar);
            }
            break;
        }
        case PropertyType::BooleanArray:
        {
            std::vector<bool> boolArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, boolArray)) == ER_OK)
            {
                adapterValue->Data() = boolArray;
            }
            break;
        }
        case PropertyType::UInt8Array:
        {
            std::vector<uint8_t> byteArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, byteArray)) == ER_OK)
            {
                adapterValue->Data() = byteArray;
            }
            break;
        }
        case PropertyType::Char16Array: __fallthrough;
        case PropertyType::Int16Array:
        {
            std::vector<int16_t> intArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, intArray)) == ER_OK)
            {
                adapterValue->Data() = intArray;
            }
            break;
        }
        case PropertyType::UInt16Array:
        {
            std::vector<uint16_t> uintArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, uintArray)) == ER_OK)
            {
                adapterValue->Data() = uintArray;
            }
            break;
        }
        case PropertyType::Int32Array:
        {
            std::vector<int32_t> intArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, intArray)) == ER_OK)
            {
                adapterValue->Data() = intArray;
            }
            break;
        }
        case PropertyType::UInt32Array:
        {
            std::vector<uint32_t> uintArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, uintArray)) == ER_OK)
            {
                adapterValue->Data() = uintArray;
            }
            break;
        }
        case PropertyType::Int64Array:
        {
            std::vector<int64_t> intArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, intArray)) == ER_OK)
            {
                adapterValue->Data() = intArray;
            }
            break;
        }
        case PropertyType::UInt64Array:
        {
            std::vector<uint64_t> uintArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, uintArray)) == ER_OK)
            {
                adapterValue->Data() = uintArray;
            }
            break;
        }
        case PropertyType::DoubleArray:
        {
            std::vector<double> doubleArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, doubleArray)) == ER_OK)
            {
                adapterValue->Data() = doubleArray;
            }
            break;
        }
        case PropertyType::StringArray:
        {
            std::vector<std::string> stringArray;
            if ((status = GetArrayFromMsgArg(msgArg, signature, stringArray)) == ER_OK)
            {
                adapterValue->Data() = stringArray;
            }
            break;
        }
        case PropertyType::StringDictionary:
        {
            std::unordered_map<std::string, std::string> stringDict;
            if ((status = GetDictionaryFromMsgArg(msgArg, signature, stringDict)) == ER_OK)
            {
                adapterValue->Data() = stringDict;
            }
            break;
        }

        default:
            status = ER_NOT_IMPLEMENTED;
            break;
    }

leave:
    return status;
}


QStatus AllJoynHelper::GetAdapterValue(_Inout_ std::shared_ptr<IAdapterValue> adapterValue, _In_ alljoyn_msgarg msgArg)
{
    QStatus status = ER_OK;
    string signature;

    auto& propertyValue = adapterValue->Data();

    //get the signature
    status = GetSignature(propertyValue.Type(), signature);
    if (status != ER_OK)
    {
        goto leave;
    }

    switch (propertyValue.Type())
    {
    case PropertyType::Boolean:
    {
        bool tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::UInt8:
    {
        int8_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::Char16:  __fallthrough;
    case PropertyType::Int16:
    {
        int16_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::UInt16:
    {
        uint16_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::Int32:
    {
        int32_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::UInt32:
    {
        uint32_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::Int64:
    {
        int64_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::UInt64:
    {
        uint64_t tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::Double:
    {
        double tempVal;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempVal);
        if (ER_OK == status)
        {
            propertyValue = tempVal;
        }
        break;
    }

    case PropertyType::String:
    {
        char *tempChar = nullptr;
        status = alljoyn_msgarg_get(msgArg, signature.c_str(), &tempChar);
        if (ER_OK == status)
        {
            // convert char to wide char, create Platform::String
            // and update value
            propertyValue = std::string(tempChar);
        }
        break;
    }
    case PropertyType::BooleanArray:
    {
        std::vector<bool> boolArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, boolArray)) == ER_OK)
        {
            propertyValue = boolArray;
        }
        break;
    }
    case PropertyType::UInt8Array:
    {
        std::vector<uint8_t> byteArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, byteArray)) == ER_OK)
        {
            propertyValue = byteArray;
        }
        break;
    }
    case PropertyType::Char16Array: __fallthrough;
    case PropertyType::Int16Array:
    {
        std::vector<int16_t> intArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, intArray)) == ER_OK)
        {
            propertyValue = intArray;
        }
        break;
    }
    case PropertyType::UInt16Array:
    {
        std::vector<uint16_t> uintArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, uintArray)) == ER_OK)
        {
            propertyValue = uintArray;
        }
        break;
    }
    case PropertyType::Int32Array:
    {
        std::vector<int32_t> intArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, intArray)) == ER_OK)
        {
            propertyValue = intArray;
        }
        break;
    }
    case PropertyType::UInt32Array:
    {
        std::vector<uint32_t> uintArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, uintArray)) == ER_OK)
        {
            propertyValue = uintArray;
        }
        break;
    }
    case PropertyType::Int64Array:
    {
        std::vector<int64_t> intArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, intArray)) == ER_OK)
        {
            propertyValue = intArray;
        }
        break;
    }
    case PropertyType::UInt64Array:
    {
        std::vector<uint64_t> uintArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, uintArray)) == ER_OK)
        {
            propertyValue = uintArray;
        }
        break;
    }
    case PropertyType::DoubleArray:
    {
        std::vector<double> doubleArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, doubleArray)) == ER_OK)
        {
            propertyValue = doubleArray;
        }
        break;
    }
    case PropertyType::StringArray:
    {
        std::vector<std::string> stringArray;
        if ((status = GetArrayFromMsgArg(msgArg, signature, stringArray)) == ER_OK)
        {
            propertyValue = stringArray;
        }
        break;
    }

    default:
        status = ER_NOT_IMPLEMENTED;
        break;
    }

leave:
    return status;
}

template<typename T>
QStatus AllJoynHelper::GetArrayFromMsgArg(_In_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _Out_ std::vector<T>& arrayArg)
{
    QStatus status = ER_OK;
    alljoyn_msgarg entries;
    size_t numVals = 0;

    if (ajSignature.length() != 2 || ajSignature[0] != 'a')
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    status = alljoyn_msgarg_get(msgArg, ajSignature.c_str(), &numVals, &entries);
    if (ER_OK == status)
    {
        arrayArg.resize(numVals);

        for (size_t i = 0; i < numVals; i++)
        {
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(entries, i), &ajSignature[1], &arrayArg[i]);
            if (ER_OK != status)
            {
                goto leave;
            }
        }
    }

leave:
    return status;
}

QStatus AllJoynHelper::GetArrayFromMsgArg(
    _In_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _Out_ std::vector<bool>& arrayArg)
{
    QStatus status = ER_OK;
    alljoyn_msgarg entries;
    size_t numVals = 0;

    if (ajSignature != "ab")
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    status = alljoyn_msgarg_get(msgArg, ajSignature.c_str(), &numVals, &entries);
    if (ER_OK == status)
    {
        auto boolArray = std::make_unique<bool[]>(numVals);
        for (size_t i = 0; i < numVals; i++)
        {
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(entries, i), &ajSignature[1], boolArray.get() + i);
            if (ER_OK != status)
            {
                goto leave;
            }
        }

        arrayArg = std::vector<bool>();
        arrayArg.resize(numVals);
        std::copy(boolArray.get(), boolArray.get() + numVals, arrayArg.begin());
    }

leave:
    return status;
}

QStatus AllJoynHelper::GetArrayFromMsgArg(
    _In_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _Out_ std::vector<std::string>& arrayArg)
{
    QStatus status = ER_OK;
    alljoyn_msgarg entries;
    size_t numVals = 0;

    if (ajSignature != "as")
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    status = alljoyn_msgarg_get(msgArg, ajSignature.c_str(), &numVals, &entries);
    if (ER_OK == status)
    {
        arrayArg.reserve(numVals);

        for (size_t i = 0; i < numVals; i++)
        {
            char *tempBuffer = nullptr;
            status = alljoyn_msgarg_get(alljoyn_msgarg_array_element(entries, i), &ajSignature[1], &tempBuffer);
            if (ER_OK != status)
            {
                goto leave;
            }

            arrayArg.emplace_back(tempBuffer);
        }
    }

leave:
    return status;
}

QStatus AllJoynHelper::GetDictionaryFromMsgArg(
    _In_ alljoyn_msgarg msgArg, _In_ const std::string& ajSignature, _Out_ std::unordered_map<std::string, std::string>& dictArg)
{
    QStatus status = ER_OK;
    alljoyn_msgarg entries;
    size_t numVals = 0;

    if (ajSignature != "a{ss}")
    {
        status = ER_BAD_ARG_2;
        goto leave;
    }

    status = alljoyn_msgarg_get(msgArg, ajSignature.c_str(), &numVals, &entries);
    if (ER_OK == status)
    {
        dictArg.reserve(numVals);

        for (size_t i = 0; i < numVals; i++)
        {
            char *keyString = nullptr;
            char *valueString = nullptr;
            status = alljoyn_msgarg_get(
                alljoyn_msgarg_array_element(entries, i), &ajSignature[1], &keyString, &valueString);
            if (ER_OK != status)
            {
                goto leave;
            }
            dictArg.emplace(keyString, valueString);
        }
    }

leave:
    return status;
}

QStatus Bridge::AllJoynHelper::GetAdapterObject(std::shared_ptr<IAdapterValue> adapterValue, alljoyn_msgarg msgArg, DeviceMain *deviceMain)
{
    QStatus status = ER_OK;
    char *tempChar = nullptr;
    std::shared_ptr<IAdapterProperty> propertyParam = nullptr;

    // This doesn't seem to be used
    //if (TypeCode::Object != Type::GetTypeCode(adapterValue->Data->GetType()))
    //{
        // not an object => nothing to do
        // (this is not an error for this routine)
        //goto leave;
    //}

    // IAdapterProperty is the only
    // translate bus object path into IAdapterProperty
    status = alljoyn_msgarg_get(msgArg, "s", &tempChar);
    if (ER_OK != status)
    {
        // wrong signature
        status = ER_BUS_BAD_SIGNATURE;
        goto leave;
    }

    // find adapter object from string
    propertyParam = deviceMain->GetAdapterProperty(tempChar);
    if (nullptr == propertyParam)
    {
        // there is no matching IAdapterProperty for that argument
        status = ER_BUS_BAD_VALUE;
        goto leave;
    }
    adapterValue->Data() = propertyParam;

leave:
    return status;
}

QStatus AllJoynHelper::SetMsgArgFromAdapterObject(_In_ std::shared_ptr<IAdapterValue> adapterValue, _Inout_ alljoyn_msgarg msgArg, _In_ DeviceMain *deviceMain)
{
    QStatus status = ER_OK;
    string busObjectPath;
    std::shared_ptr<IAdapterProperty> adapterProperty;
    try {
        adapterProperty = adapterValue->Data().Get<std::shared_ptr<IAdapterProperty>>();
    }
    catch (boost::bad_get&) {}

    if (nullptr == adapterProperty)
    {
        // adapter Value doesn't contain a IAdapterProperty
        status = ER_BUS_BAD_VALUE;
        goto leave;
    }

    // get bus object path from IAdapterProperty and set message arg
    busObjectPath = deviceMain->GetBusObjectPath(adapterProperty);
    if (0 == busObjectPath.length())
    {
        status = alljoyn_msgarg_set(msgArg, "s", "");
    }
    else
    {
        // note that std::string must be copied in a temp buffer otherwise
        // alljoyn_msgarg_set will not be able to set the argument using c_str() method of std::string
        char* tempBuffer = new char[busObjectPath.length() + 1];
        memset(tempBuffer, 0, (busObjectPath.length() + 1));
        if (nullptr == tempBuffer)
        {
            status = ER_OUT_OF_MEMORY;
            goto leave;
        }
        busObjectPath.copy(tempBuffer, busObjectPath.length());
        status = alljoyn_msgarg_set(msgArg, "s", tempBuffer);
        delete[] tempBuffer;
    }

leave:
    return status;
}

QStatus AllJoynHelper::GetSignature(_In_ PropertyType propertyType, _Out_ std::string &signature)
{
    QStatus status = ER_OK;

    switch (propertyType)
    {
    case PropertyType::Boolean:
        signature = "b";
        break;

    case PropertyType::UInt8:
        signature = "y";
        break;

    case PropertyType::Char16:  __fallthrough;
    case PropertyType::Int16:
        signature = "n";
        break;

    case PropertyType::UInt16:
        signature = "q";
        break;

    case PropertyType::Int32:
        signature = "i";
        break;

    case PropertyType::UInt32:
        signature = "u";
        break;

    case PropertyType::Int64:
        signature = "x";
        break;

    case PropertyType::UInt64:
        signature = "t";
        break;

    case PropertyType::Double:
        signature = "d";
        break;

    case PropertyType::String:
        signature = "s";
        break;

    case PropertyType::BooleanArray:
        signature = "ab";
        break;

    case PropertyType::UInt8Array:
        signature = "ay";
        break;

    case PropertyType::Char16Array: __fallthrough;
    case PropertyType::Int16Array:
        signature = "an";
        break;

    case PropertyType::UInt16Array:
        signature = "aq";
        break;

    case PropertyType::Int32Array:
        signature = "ai";
        break;

    case PropertyType::UInt32Array:
        signature = "au";
        break;

    case PropertyType::Int64Array:
        signature = "ax";
        break;

    case PropertyType::UInt64Array:
        signature = "at";
        break;

    case PropertyType::DoubleArray:
        signature = "ad";
        break;

    case PropertyType::StringArray:
        signature = "as";
        break;

    case PropertyType::StringDictionary:
        signature = "a{ss}";
        break;

    default:
        status = ER_NOT_IMPLEMENTED;
        break;
    }

    return status;
}

std::shared_ptr<IAdapterValue> AllJoynHelper::GetDefaultAdapterValue(_In_ const char* signature)
{
    PropertyValue value;

    switch (*signature)
    {
    case 'b':
        value = false;
        break;

    case 'y':
        value = uint8_t(0);
        break;

    case 'n':
        value = int16_t(0);
        break;

    case 'q':
        value = uint16_t(0);
        break;

    case 'i':
        value = int32_t(0);
        break;

    case 'u':
        value = uint32_t(0);
        break;

    case 'x':
        value = int64_t(0);
        break;

    case 't':
        value = uint64_t(0);
        break;

    case 'd':
        value = double(0);
        break;

    case 's':
        value = std::string{};
        break;

    case 'a':
        signature++;
        switch (*signature)
        {
        case 'b':
            value = std::vector<bool>{};
            break;

        case 'y':
            value = std::vector<uint8_t>{};
            break;

        case 'n':
            value = std::vector<int16_t>{};
            break;

        case 'q':
            value = std::vector<uint16_t>{};
            break;

        case 'i':
            value = std::vector<int32_t>{};
            break;

        case 'u':
            value = std::vector<uint32_t>{};
            break;

        case 'x':
            value = std::vector<int64_t>{};
            break;

        case 't':
            value = std::vector<uint64_t>{};
            break;

        case 'd':
            value = std::vector<double>{};
            break;

        case 's':
            value = std::vector<std::string>{};
            break;

        case '{':
            if (strcmp(signature, "{ss}") == 0)
            {
                value = std::unordered_map<std::string, std::string>{};
            }
            break;
        }
    }

    if (value.Type() != PropertyType::Empty)
    {
        AdapterValue* aval = new AdapterValue(std::string(), value);
        std::shared_ptr<AdapterValue> sval(aval);
        return sval;
    }

    return nullptr;
}

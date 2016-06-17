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

#include <unordered_map>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/vector/vector40.hpp>
#include <boost/variant.hpp>
#include "CommonUtils.h"

namespace Bridge
{

class IAdapterProperty;
class IAdapterValue;
class IAdapterDevice;

enum class PropertyType
{
    Empty = 0,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Double,
    Char16,
    Boolean,
    String,
    UInt8Array = 1025,
    Int16Array,
    UInt16Array,
    Int32Array,
    UInt32Array,
    Int64Array,
    UInt64Array,
    DoubleArray,
    Char16Array,
    BooleanArray,
    StringArray,
    StringDictionary = 2048 + String,
    Object = 4096
};

class PropertyValue
{
    struct GetUnderlyingType : public boost::static_visitor<PropertyType>
    {
    private:
        // Embedded commas in the type cause problems with the macro.
        typedef std::unordered_map<std::string, std::string> _string_dictionary;

    public:

#define PROPERTY_TYPE_MAPPING(T1, T2) PropertyType operator()(T1) const { return PropertyType::T2; }
        PROPERTY_TYPE_MAPPING(boost::blank, Empty);
        PROPERTY_TYPE_MAPPING(uint8_t, UInt8);
        PROPERTY_TYPE_MAPPING(int16_t, Int16);
        PROPERTY_TYPE_MAPPING(uint16_t, UInt16);
        PROPERTY_TYPE_MAPPING(int32_t, Int32);
        PROPERTY_TYPE_MAPPING(uint32_t, UInt32);
        PROPERTY_TYPE_MAPPING(int64_t, Int64);
        PROPERTY_TYPE_MAPPING(uint64_t, UInt64);
        PROPERTY_TYPE_MAPPING(double, Double);
        PROPERTY_TYPE_MAPPING(char16_t, Char16);
        PROPERTY_TYPE_MAPPING(bool, Boolean);
        PROPERTY_TYPE_MAPPING(std::string, String);
        PROPERTY_TYPE_MAPPING(std::vector<bool>, BooleanArray);
        PROPERTY_TYPE_MAPPING(std::vector<uint8_t>, UInt8Array);
        PROPERTY_TYPE_MAPPING(std::vector<int16_t>, Int16Array);
        PROPERTY_TYPE_MAPPING(std::vector<uint16_t>, UInt16Array);
        PROPERTY_TYPE_MAPPING(std::vector<int32_t>, Int32Array);
        PROPERTY_TYPE_MAPPING(std::vector<uint32_t>, UInt32Array);
        PROPERTY_TYPE_MAPPING(std::vector<int64_t>, Int64Array);
        PROPERTY_TYPE_MAPPING(std::vector<uint64_t>, UInt64Array);
        PROPERTY_TYPE_MAPPING(std::vector<double>, DoubleArray);
        PROPERTY_TYPE_MAPPING(std::vector<char16_t>, Char16Array);
        PROPERTY_TYPE_MAPPING(std::vector<std::string>, StringArray);
        PROPERTY_TYPE_MAPPING(_string_dictionary, StringDictionary);
        PROPERTY_TYPE_MAPPING(std::shared_ptr<IAdapterProperty>, Object);
        PROPERTY_TYPE_MAPPING(std::shared_ptr<IAdapterValue>, Object);
		PROPERTY_TYPE_MAPPING(std::shared_ptr<IAdapterDevice>, Object);
#undef PROPERTY_TYPE_MAPPING
    };

    boost::make_variant_over<boost::mpl::vector27<
            boost::blank,
            uint8_t,
            int16_t,
            uint16_t,
            int32_t,
            uint32_t,
            int64_t,
            uint64_t,
            double,
            char16_t,
            bool,
            std::string,
            std::vector<bool>,
            std::vector<uint8_t>,
            std::vector<int16_t>,
            std::vector<uint16_t>,
            std::vector<int32_t>,
            std::vector<uint32_t>,
            std::vector<int64_t>,
            std::vector<uint64_t>,
            std::vector<double>,
            std::vector<char16_t>,
            std::vector<std::string>,
            std::unordered_map<std::string, std::string>,
            std::shared_ptr<IAdapterProperty>,
            std::shared_ptr<IAdapterValue>,
			std::shared_ptr<IAdapterDevice>
		>>::type _data;

    template<typename T> std::string Join() const
    {
        std::stringstream result;
        auto vectorVal = Get<std::vector<T>>();
        std::copy(vectorVal.begin(), vectorVal.end(), std::ostream_iterator<T>(result, ","));
        return std::move(result.str());
    }

public:
    PropertyValue() = default;
    PropertyValue(const PropertyValue&) = default;
    PropertyValue(PropertyValue&&) = default;

    //template<typename T, typename = std::enable_if_t<!std::is_same<std::decay_t<T>, PropertyValue>::value>> // C++14
    template<typename T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, PropertyValue>::value>::type>
    PropertyValue(T&& t) : _data(std::forward<T>(t)) {}

    PropertyValue& operator=(const PropertyValue&) = default;
    PropertyValue& operator=(PropertyValue&&) = default;
    //template<typename T, typename = std::enable_if_t<!std::is_same<std::decay_t<T>, PropertyValue>::value>> // C++14
    template<typename T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, PropertyValue>::value>::type>
    PropertyValue& operator=(T&& t)
    {
        _data = std::forward<T>(t);
        return *this;
    }

    PropertyType Type() const { return boost::apply_visitor(GetUnderlyingType{}, _data); }

    template <typename T>
    T Get() const { return boost::get<T>(this->_data); }

    std::string ToString() const
    {
        std::string out;

        switch (Type())
        {
        case PropertyType::Boolean: out = boost::lexical_cast<std::string>(Get<bool>()); break;
        case PropertyType::UInt8: out = boost::lexical_cast<std::string>(Get<uint8_t>()); break;
        case PropertyType::Char16:
        {
            // TODO: Probably better ways to handle this, re-implement
            std::u16string stru16;
            stru16 += Get<char16_t>();
            out = StringUtils::ToUtf8(stru16);
            break;
        }
        case PropertyType::Int16: out = boost::lexical_cast<std::string>(Get<int16_t>()); break;
        case PropertyType::UInt16: out = boost::lexical_cast<std::string>(Get<uint16_t>()); break;
        case PropertyType::Int32: out = boost::lexical_cast<std::string>(Get<int32_t>()); break;
        case PropertyType::UInt32: out = boost::lexical_cast<std::string>(Get<uint32_t>()); break;
        case PropertyType::Int64: out = boost::lexical_cast<std::string>(Get<int64_t>()); break;
        case PropertyType::UInt64: out = boost::lexical_cast<std::string>(Get<uint64_t>()); break;
        case PropertyType::Double: out = boost::lexical_cast<std::string>(Get<double>()); break;
        case PropertyType::String: out = boost::lexical_cast<std::string>(Get<std::string>()); break;

        case PropertyType::BooleanArray: out = Join<bool>(); break;
        case PropertyType::UInt8Array: out = Join<uint8_t>(); break;
        case PropertyType::Char16Array:
        {
            // TODO: Probably better ways to handle this, re-implement
            auto arrayVal = Get<std::vector<char16_t>>();
            for (const auto& item : arrayVal)
            {
                std::u16string stru16;
                stru16 += item;
                out += StringUtils::ToUtf8(stru16);
                out += ',';
            }
            if (out.length() > 0)
            {
                out.pop_back();
            }
            break;
        }
        case PropertyType::Int16Array: out = Join<int16_t>(); break;
        case PropertyType::UInt16Array: out = Join<uint16_t>(); break;
        case PropertyType::Int32Array: out = Join<int32_t>(); break;
        case PropertyType::UInt32Array: out = Join<uint32_t>(); break;
        case PropertyType::Int64Array: out = Join<int64_t>(); break;
        case PropertyType::UInt64Array: out = Join<uint64_t>(); break;
        case PropertyType::DoubleArray: out = Join<double>(); break;
        case PropertyType::StringArray: out = Join<std::string>(); break;

        case PropertyType::StringDictionary:
            // TODO: Convert dictionary to string
            out = "{}";
            break;

        default:
            break;
        }

        return std::move(out);
    }
};

}

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

namespace Bridge
{

    //
    // Constants class
    // Description:
    //  Adapter level signals and parameters names
    //
    class Constants final
    {
    public:

        // Device Arrival Signal
        static std::string DEVICE_ARRIVAL_SIGNAL()
        {
            return device_arrival_signal;
        }

        static std::string DEVICE_ARRIVAL__DEVICE_HANDLE()
        {
            return device_arrival__device_handle;
        }


        // Device Removal Signal
        static std::string DEVICE_REMOVAL_SIGNAL()
        {
            return device_removal_signal;
        }

        static std::string DEVICE_REMOVAL__DEVICE_HANDLE()
        {
            return device_removal__device_handle;
        }


        // Change of Value Signal
        static std::string CHANGE_OF_VALUE_SIGNAL()
        {
            return change_of_value_signal;
        }

        static std::string COV__PROPERTY_HANDLE()
        {
            return cov__property_handle;
        }

        static std::string COV__ATTRIBUTE_HANDLE()
        {
            return cov__attribute_handle;
        }


        // Lamp State Changed Signal
        static std::string LAMP_STATE_CHANGED_SIGNAL_NAME()
        {
            return lamp_state_changed_signal_name;
        }

        static std::string SIGNAL_PARAMETER__LAMP_ID__NAME()
        {
            return lamp_id__parameter_name;
        }

    private:
        static std::string device_arrival_signal;
        static std::string device_arrival__device_handle;
        static std::string device_removal_signal;
        static std::string device_removal__device_handle;
        static std::string change_of_value_signal;
        static std::string cov__property_handle;
        static std::string cov__attribute_handle;
        static std::string lamp_state_changed_signal_name;
        static std::string lamp_id__parameter_name;
    };

}

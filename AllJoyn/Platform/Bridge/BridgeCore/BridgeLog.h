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

#include "CommonUtils.h"

namespace BridgeLog
{
    void Log(const char* logStr);

    //*********************************************************************************************************************
    //
    // Logs an "Enter <functionName>" message
    //
    // functionName     name of the Bridge Function that was entered
    //
    //*********************************************************************************************************************
    inline void LogEnter(_In_z_ const char* funcName)
    {
        std::string enterString = StringUtils::Format("Enter %s", funcName);
        Log(enterString.c_str());
    }



    //*********************************************************************************************************************
    //
    // Logs an informational message to the in memory log file.  The log is not flushed to disk.
    //
    // LogMsg   A descriptive contextual message associated with the exception
    //
    //*********************************************************************************************************************
    inline void LogInfo(_In_ const std::string& LogMsg)
    {
        //std::shared_ptr<LoggingFields> fields = ref new LoggingFields();
        //m_channel->LogMessage(LogMsg);

        std::string msg = StringUtils::Format("Info: %s\n", LogMsg.c_str());
        Log(msg.c_str());
    }

    //*********************************************************************************************************************
    //
    // Logs an Exception with contextual message to the Bridge Log and flushes in-memory log to disk.
    //
    // LogMsg   A descriptive contextual message associated with the exception (Note that the exception's internal message
    //          will also be logged)
    // ex       The Exception to Log
    //
    //*********************************************************************************************************************
    inline void LogError(_In_ const std::string& LogMsg, _In_ std::exception& ex)
    {
        //std::shared_ptr<LoggingFields> fields = ref new LoggingFields();
        //fields->AddString(L"Message", LogMsg.c_str());
        //fields->AddInt32(L"HResult", ex->HResult);
        //fields->AddString(L"ExMessage", ex->Message);
        //m_channel->LogEvent(L"Exception", fields, LoggingLevel::Error);
        //Shutdown();
        std::string msg = StringUtils::Format("Error: exception:(%x), message: %s\n", ex.what(), LogMsg.c_str());
        Log(msg.c_str());
    }

    //*********************************************************************************************************************
    //
    // Logs an HRESULT with Message to the Bridge Log and flushes in-memory log to disk.
    //
    // LogMsg   A descriptive contextual message associated with the error
    // hr       The HRESULT error code to log
    //
    //*********************************************************************************************************************
    inline void LogError(_In_ const std::string& LogMsg, int hr)
    {
        //std::shared_ptr<LoggingFields> fields = ref new LoggingFields();
        //fields->AddString(L"Message", LogMsg.c_str());
        //fields->AddInt32(L"HResult", hr);
        //m_channel->LogEvent(L"Error", fields, LoggingLevel::Error);
        //Shutdown();
        std::string msg = StringUtils::Format("Error: hr:(%x), Message: %s\n", hr, LogMsg.c_str());
        Log(msg.c_str());
    }

    //*********************************************************************************************************************
    //
    // Logs a "Leave <functionName>" message
    //
    // functionName     name of the Bridge Function that was exited
    // hr               hresult to report on exit from a function.  If Non-Zero (not S_OK) this will log an Error and
    //                  stop the log
    //
    //
    //*********************************************************************************************************************
    inline void LogLeave(_In_z_ const char* funcName, int hr = 0)
    {
        std::string leaveString = StringUtils::Format("Leave %s.  Result=0x%X", funcName, hr);
        if (hr == ER_OK)
        {
            LogInfo(leaveString);
        }
        else
        {
            LogError(leaveString, hr);
        }
    }
};

//-----------------------------------------------------------------------------
// Desc: Outputs a formatted string to the BridgeLog
//-----------------------------------------------------------------------------
template <typename... Args>
void DebugLogInfo(_In_ const char* strMsg, Args&&... args)
{
#if defined(DEBUG) | defined(_DEBUG)
    std::string msg = StringUtils::Format(strMsg, std::forward<Args>(args)...);

    BridgeLog::LogInfo(msg);
#endif
}

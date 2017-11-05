/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LOG_COLLECTOR_H
#define _LOG_COLLECTOR_H
#pragma once

#include <chrono>

#include "LogCollectorDefinitions.h"
#include "LogCollectorConfiguration.h"
#include "OsHandler.h"

class Device;

namespace log_collector
{

    class LogCollector
    {
    public:
        // initialize a new instance of log collector with a refernce to the device its belongs to
        LogCollector(Device* device, CpuType tracerType);

        ~LogCollector() { if (!m_log_buf) free(m_log_buf); }

        /*
        * raise a flag for stop collecting logs
        */
        Status StopCollectingLogs();
        Status StartCollectingLogs();
        /******************** end of APIs for offline use (DmTools doesn't request for logs) *************/

        /******************** APIs for online use (DmTools sends requests) ******************************/
        /*
        * Perform all required operations before collecting logs considering the given configuration, including "cleaning" the buffer (distibute the obsolete logs)
        * @param: configuration for this collection session
        * @return: operation status
        */
        Status PrepareLogCollection();

        bool CollectionIsNeeded();

        bool IsInitialized() const { return m_initialized; }

        /*
        * distribute next logs chunck (without log lines that were already read)
        */
        Status GetNextLogs(std::vector<RawLogLine>& rawLogLines);

        /*
        * Perform all required operations after collecting logs (e.g. close the log file if applicable)
        */
        Status FinishLogCollection();

        bool SetConfigurationParamerter(const string& parameter, const string& value);
        string GetConfigurationParameterValue(const string& parameter, bool& success);
        string GetConfigurationDump();

        /******************** end of APIs for online use (DmTools sends requests) ***********************/

    private:
        // returns the size of logs buffer in fw/ucode
        size_t log_size(size_t entry_num) { return sizeof(struct log_table_header) + entry_num * 4; }

        bool ComputeLogStartAddress();

        // configurations
        bool SetModuleVerbosity();
        void ParseModuelLevel(string moduleString);
        string GetModuleVerbosityConfigurationValue();
        bool ReadConfigFile();
        bool ParseConfigLine(string line);

        // OS agnostic read log function
        bool ReadLogBuffer(void* logBuffer, size_t size);
        bool ReadLogWritePointer(void* logBuffer);

        int ParseLog(void* log_buf, size_t log_buf_entries, std::vector<RawLogLine>& rawLogLines);

        // output file methods
        string GetNextOutputFileFullName();
        bool CreateNewOutputFile();
        void CloseOutputFile();
        void WriteToOutputFile(const stringstream& logLine);

        //****************** attributes ******************/

        // log collector's context
        LogCollectorConfiguration m_configuration;
        Device* m_device;
        string m_deviceName;
        CpuType m_tracerType;
        string m_logErrorMessagePrefix;

        // managing collection
        bool m_continueCollectingLogs;
        std::chrono::system_clock::time_point m_lastPollingTimestamp;
        bool m_initialized;
        bool m_ongoingRecording;

        // output file
        ofstream m_outputFile;
        long m_currentOutputFileSize = 0;

        // log buffer members
        unique_ptr<log_table_header> m_logHeader; // content of log buffer header
        int m_logAddress; // log buffer start address
        void* m_log_buf; // log buffer content
        u32 m_rptr; // last read address
        u32 m_lastWptr; // last write ptr address (used for detecting buffer overflow)
                        //****************** end of attributes ************/
    };

}

#endif // !_LOG_COLLECTOR_H



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

#include <locale> // std::tolower
#include <sstream>

#include "LogCollector.h"
#include "Device.h"

namespace log_collector
{

    LogCollector::LogCollector(Device* device, CpuType tracerType) :
        m_device(device),
        m_deviceName(device->GetDeviceName()),
        m_tracerType(tracerType),
        m_continueCollectingLogs(false),
        m_initialized(false),
        m_ongoingRecording(false)
    {
        m_lastPollingTimestamp = std::chrono::system_clock::now();
        stringstream ss;
        ss << endl << "host_manager_11ad error: " << CPU_TYPE_TO_STRING[m_tracerType] << " log tracer error in device " << m_deviceName << " - ";
        m_logErrorMessagePrefix = ss.str();
    }

    bool LogCollector::ReadConfigFile()
    {
        // check exsitence of file
        string configFileName = FileSystemOsAbstraction::GetConfigurationFilesLocation() + m_configFileName;
        if (!FileSystemOsAbstraction::DoesFolderExist(configFileName)) // later - check user location
        {
            LOG_INFO << "No log recording configuration file, using the default configuration" << endl;
            return false;
        }

        // read file
        try
        {
            m_configuration.ResetValue();
            ifstream fd(configFileName.c_str());
            string line;
            while (std::getline(fd, line))
            {
                if (!ParseConfigLine(line))
                {
                    LOG_ERROR << "Failed to parse config file " << m_configFileName << ", problematic line: " << line << "\n. Use the defulat configuration" << endl;
                }
            }
            return true;

        }
        catch (...)
        {
            m_configuration.ResetValue();
            LOG_ERROR << "Got exception while trying to open config file " << configFileName << endl;
            return false;
        }

        LOG_INFO << "Log recording configuration file was taken from " << configFileName << endl;
        return true;
    }

    bool LogCollector::ParseConfigLine(string line)
    {
        LOG_VERBOSE << "configuration line = << " << line << endl;

        // Skip comments
        if (line.find("//") == 0)
        {
            return true;
        }

        std::size_t found = line.find(configuration_parameter_value_delimiter);
        if (std::string::npos != found)
        {
            return SetConfigurationParamerter(line.substr(0, found), line.substr(found + 1));
        }
        return true; // skipping this line
    }

    Status LogCollector::PrepareLogCollection()
    {
        if (!m_continueCollectingLogs)
        {
            return Status(true, "");
        }

        // initialize members
        m_rptr = 0;
        m_lastWptr = 0;
        m_currentOutputFileSize = 0;
        m_logAddress = 0;

        // setting configuration
        ReadConfigFile();

        // set log buffer header
        m_logHeader.reset(new log_table_header());
        if (!SetModuleVerbosity())
        {
            stringstream ss;
            ss << "Failed to set " << CPU_TYPE_TO_STRING[m_tracerType] << " log verbosity level for device " << m_deviceName << endl;
            return Status(false, ss.str());
        }

        // Allocate log buffer
        m_log_buf = malloc(log_size(fw_log_buf_entries));
        if (!m_log_buf)
        {
            stringstream ss;
            ss << "Unable to allocate log buffer " << log_size(fw_log_buf_entries) << " bytes";
            LOG_ERROR << ss.str() << endl;
            return Status(false, ss.str());
        }

        m_initialized = true;
        LOG_DEBUG << "Log collector prepareation was successfully finished" << endl;
        Status s(true, "");
        return s;
    }

    Status LogCollector::GetNextLogs(std::vector<RawLogLine>& rawLogLines)
    {
        if (!m_initialized)
        {
            stringstream ss;
            ss << CPU_TYPE_TO_STRING[m_tracerType] << " log tracer of device " << m_deviceName << " isn't initialized! Can't collect logs" << endl;
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " tracer isn't initialized" << endl;
            return Status(false, ss.str());
        }

        // handle creation/closure of output file if needed (if applicable)

        // Don't collect logs
        if (!m_ongoingRecording && !m_continueCollectingLogs)
        {
            return move(Status());
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " no request for logs" << endl;
        }

        // Asked to stop log collection
        if (m_ongoingRecording && !m_continueCollectingLogs)
        {
            return FinishLogCollection();
        }

        // Asked to start log collection
        else if (!m_ongoingRecording && m_continueCollectingLogs)
        {
            m_currentOutputFileSize = 0;
            if (m_configuration.m_recordToFile && !CreateNewOutputFile())
            {
                //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " failed to create output file" << endl;
                return move(Status(false, "failed to create a new output file"));
            }

            m_ongoingRecording = true;
        }

        // Collect logs and split output file if required
        else if (m_ongoingRecording)
        {
            if (m_currentOutputFileSize > m_configuration.m_fileFragmentSize)
            {
                m_currentOutputFileSize = 0;
                CloseOutputFile();
                if (m_configuration.m_recordToFile && !CreateNewOutputFile())
                {
                    //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " failed to create output file" << endl;
                    return move(Status(false, "failed to create a new output file"));
                }
            }
        }

        // Read log buffer
        if (ReadLogWritePointer(m_log_buf))
        {
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " there are new log lines" << endl;
            m_currentOutputFileSize += ParseLog(m_log_buf, fw_log_buf_entries, rawLogLines);
            LOG_DEBUG << "Current File Size = " << m_currentOutputFileSize << endl;
        }
        // else flow is still valid and happens when the interface isn't up yet

        m_lastPollingTimestamp = chrono::system_clock::now();
        return move(Status());
    }

    Status LogCollector::FinishLogCollection()
    {
        m_ongoingRecording = false;
        CloseOutputFile();
        m_currentOutputFileSize = 0;
        m_initialized = false;
        return move(Status());
    }

    bool LogCollector::CollectionIsNeeded()
    {
        return IsInitialized() && (m_configuration.m_pollingIntervalMs < (chrono::system_clock::now() - m_lastPollingTimestamp));
    }

    int LogCollector::ParseLog(void* log_buf, size_t log_buf_entries, std::vector<RawLogLine>& rawLogLines)
    {
        int sizeAdded = 0;

        // Prepare a header pointing to log buffer top
        struct log_table_header *h = (struct log_table_header*)log_buf; // h contains all the log buffer (including the log buffer header)

        u32 wptr = h->write_ptr;
        //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " wptr == " << wptr << ", rptr == " << m_rptr << endl;

        if (wptr == m_rptr)
        {
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " nothing to read" << endl;
            // Nothing to read.
            return 0;
        }

        {
            stringstream ss;
            ss << Utils::GetCurrentLocalTimeXml() << "<Content>";
            WriteToOutputFile(ss);
        }

        if (wptr < m_rptr)
        {
            // previously was just retun 0
            {
                //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " wptr < m_rptr => read more lines than were created" << endl;
                stringstream ss;
                ss << m_logErrorMessagePrefix << "device is restarting" << endl; // read more lines than were created
                WriteToOutputFile(ss);
            }
            rawLogLines.push_back(RawLogLine(RPTR_LARGER_THAN_WPTR, 0u));
            return 0;
        }

        // Re-Read the entire buffer
        ReadLogBuffer(log_buf, log_size(fw_log_buf_entries));
        // Update the write pointer
        wptr = h->write_ptr;

        //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " new re-read of wptr == " << wptr << ", rptr == " << m_rptr << ", log buf size == " << log_buf_entries << endl;

        if (wptr >= log_buf_entries + m_rptr)
        {
            // overflow; try to parse last wrap
            {
                stringstream ss;
                ss << m_logErrorMessagePrefix << "found buffer overrun - missed " << wptr - m_rptr << " DWORDS" << endl; // DWORD == uint32
                WriteToOutputFile(ss);
            }
            rawLogLines.push_back(RawLogLine(BUFFER_OVERRUN, (unsigned)(wptr - m_rptr - log_buf_entries))); // condition assures that the value is non-negative integer
            m_rptr = wptr - log_buf_entries;
        }

        LOG_DEBUG << "  wptr = " << wptr << ", rptr = " << m_rptr << endl;

        for (; wptr > m_rptr && (wptr != m_lastWptr); m_rptr++)
        {
            LOG_DEBUG << "wptr = " << wptr << ", rptr = " << m_rptr << endl;

            int i;
            u32 p[3] = { 0 }; // parameters array (each log line can have at most three parameters
            union log_event *evt = &h->evt[m_rptr % log_buf_entries]; // h->evt is the log line payload.

            if (evt->hdr.signature != 5)
            {
                {
                    stringstream ss;
                    ss << m_logErrorMessagePrefix << "got corrupted entry" << endl;
                    WriteToOutputFile(ss);
                }
                rawLogLines.push_back(RawLogLine(INVALID_SIGNATURE, 1));
                continue;
            }
            if (evt->hdr.parameters_num > 3)
            {
                LOG_DEBUG << "Parameter Num = " << evt->hdr.parameters_num << endl;
                continue;
            }

            vector<unsigned> params;
            for (i = 0; i < evt->hdr.parameters_num; i++)
            {
                p[i] = h->evt[(m_rptr + i + 1) % log_buf_entries].param;
                params.push_back(p[i]);
            }
            /*DebugPrint("%d,%s,%d:", evt->hdr.module,
            levels[evt->hdr.level],
            evt->hdr.strring_offset);

            DebugPrint("%d,%d,%d\n",
            (p[0]),
            (p[1]),
            (p[2]));*/

            {
                stringstream ss;
                ss << evt->hdr.module << "," << levels[evt->hdr.level] << "," << evt->hdr.strring_offset << ":" << p[0] << "," << p[1] << "," << p[2] << "\n";
                WriteToOutputFile(ss);
            }

            RawLogLine line = RawLogLine(evt->hdr.module, evt->hdr.level, evt->hdr.signature, evt->hdr.strring_offset, evt->hdr.is_string, params);

            // Gather new line for event
            rawLogLines.push_back(line);

            // (paramters) (verbosity type) (delimiters)
            sizeAdded += (5 * sizeof(int)) + (1 * sizeof(char)) + (4 * sizeof(char));

            m_rptr += evt->hdr.parameters_num;
        }

        m_lastWptr = wptr;

        {
            stringstream ss;
            ss << "</Content></Log_Content>";
            WriteToOutputFile(ss);
        }

        fflush(stdout);
        return sizeAdded;
    }



    Status LogCollector::StopCollectingLogs()
    {
        m_continueCollectingLogs = false;
        return move(Status());
    }

    Status LogCollector::StartCollectingLogs()
    {
        m_continueCollectingLogs = true;
        return move(Status());
    }

    string LogCollector::GetNextOutputFileFullName()
    {
        std::stringstream ss;
        ss << FileSystemOsAbstraction::GetConfigurationFilesLocation() << m_deviceName << "_" << CPU_TYPE_TO_STRING[m_tracerType] << "_" << time(0) << "_" << m_configuration.m_logFileSuffix << ".log";
        return ss.str();
    }

    bool LogCollector::CreateNewOutputFile()
    {
        string fullFileName = GetNextOutputFileFullName();
        LOG_INFO << "Creating output file: " << fullFileName << endl;

        m_outputFile.open(fullFileName.c_str());

        if (m_outputFile.fail())
        {
            LOG_ERROR << "Error opening output file: " << fullFileName << " Error: " << strerror(errno) << endl;
            return false;
        }

        std::ostringstream headerBuilder;

        const FwVersion fwVersion = m_device->GetFwVersion();
        const FwTimestamp fwTs = m_device->GetFwTimestamp();

        headerBuilder << "<LogFile>"
            << "<FW_Ver>"
            << "<Major>" << fwVersion.m_major << "</Major>"
            << "<Minor>" << fwVersion.m_minor << "</Minor>"
            << "<Sub>" << fwVersion.m_subMinor << "</Sub>"
            << "<Build>" << fwVersion.m_build << "</Build>"
            << "</FW_Ver>"
            << "<Compilation_Time>"
            << "<Hour>" << fwTs.m_hour << "</Hour>"
            << "<Minute>" << fwTs.m_min << "</Minute>"
            << "<Second>" << fwTs.m_sec << "</Second>"
            << "<Day>" << fwTs.m_day << "</Day>"
            << "<Month>" << fwTs.m_month << "</Month>"
            << "<Year>" << fwTs.m_year << "</Year>"
            << "</Compilation_Time>"
            << "<Logs>";

        m_outputFile << headerBuilder.str();
        return true;
    }

    void LogCollector::CloseOutputFile()
    {
        if (m_outputFile.is_open())
        {
            m_outputFile << "</Logs></LogFile>";
            m_outputFile.close();
            LOG_INFO << "Output file closed" << endl;
        }
    }

    void LogCollector::WriteToOutputFile(const stringstream& logLine)
    {
        if (m_configuration.m_recordToFile && m_outputFile.is_open())
        {
            m_outputFile << logLine.str();
        }
    }

    bool LogCollector::SetModuleVerbosity()
    {
        // Update FW & uCode log addresses
        if (!m_logAddress)
        {
            ComputeLogStartAddress();
        }

        //LOG_INFO << "Log buffer start address is 0x" << hex << m_logAddress << endl;

        // Write verbosity to the device
        for (int i = 0; i < NUM_MODULES; ++i)
        {
            m_logHeader->module_level_enable[i] = m_configuration.m_modulesVerbosity[i];
        }
        if (!m_device->GetDriver()->WriteBlock(m_logAddress + sizeof(m_logHeader->write_ptr), sizeof(m_logHeader->module_level_enable), (char*)m_logHeader->module_level_enable))
        {
            LOG_ERROR << "Failed to write module verbosity structure for " << CPU_TYPE_TO_STRING[m_tracerType] << "(address 0x" << hex << m_logAddress + sizeof(m_logHeader->write_ptr) << ", data " << ((char*)m_logHeader->module_level_enable) << ", size " << sizeof(m_logHeader->module_level_enable) << ")" << endl;
            return false;
        }
        else
        {
            LOG_DEBUG << "Module verbosity for " << CPU_TYPE_TO_STRING[m_tracerType] << "was set to " << CPU_TYPE_TO_STRING[m_tracerType] << GetModuleVerbosityConfigurationValue() << endl;
        }
        return true;
    }

    bool LogCollector::ReadLogWritePointer(void* logBuffer)
    {
        // Update FW & uCode log addresses
        if (!m_logAddress)
        {
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " try to set log address" << endl;
            ComputeLogStartAddress();
        }

        // Read the write pointer
        DWORD writePointer = 0;
        if (m_logAddress && !m_device->GetDriver()->Read(m_logAddress, writePointer))
        {
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " failed to read wptr" << endl;
            return false;
        }

        // Set the write pointer to the buffer
        memcpy(logBuffer, &writePointer, sizeof(writePointer));
        //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " new wptr is " << writePointer << endl;
        return true;
    }

    bool LogCollector::ReadLogBuffer(void* logBuffer, size_t size)
    {
        // Update FW & uCode log addresses
        if (!m_logAddress)
        {
            ComputeLogStartAddress();
        }

        if (!m_logAddress)
        {
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " log address isn't set" << endl;
        }

        // Read the actual log
        if (m_logAddress && !m_device->GetDriver()->ReadBlock(m_logAddress, size, (char*)logBuffer))
        {
            //cout << __FUNCTION__ << "," << __LINE__ << ": " << CPU_TYPE_TO_STRING[m_configuration.m_logType] << " failed to read buffer" << endl;
            return false;
        }
        return true;
    }

    bool LogCollector::ComputeLogStartAddress()
    {
        // retrieve BB type (for getting ahb start address)
        BasebandType bb = BASEBAND_TYPE_NONE;
        bb = m_device->GetBasebandType();

        // get ahb start address
        unsigned ahb_start_address;
        if (CPU_TYPE_FW == m_tracerType)
        {
            auto it = baseband_to_peripheral_memory_start_address_ahb.find(bb);
            if (baseband_to_peripheral_memory_start_address_ahb.end() == it)
            {
                LOG_ERROR << CPU_TYPE_TO_STRING[m_tracerType] << "Log collector doesn't support baseband type (type code is " << bb << ")" << endl;
                return false;
            }
            ahb_start_address = it->second;
        }
        else
        {
            auto it = baseband_to_ucode_dccm_start_address_ahb.find(bb);
            if (baseband_to_ucode_dccm_start_address_ahb.end() == it)
            {
                LOG_ERROR << CPU_TYPE_TO_STRING[m_tracerType] << "Log collector doesn't support baseband type (type code is " << bb << ")" << endl;
                return false;
            }
            ahb_start_address = it->second;
        }

        // get linker start address
        unsigned linker_start_address;
        if (CPU_TYPE_FW == m_tracerType)
        {
            auto it = baseband_to_peripheral_memory_start_address_linker.find(bb);
            if (baseband_to_peripheral_memory_start_address_linker.end() == it)
            {
                LOG_ERROR << CPU_TYPE_TO_STRING[m_tracerType] << "Log collector doesn't support baseband type (type code is " << bb << ")" << endl;
                return false;
            }
            linker_start_address = it->second;
        }
        else
        {
            auto it = baseband_to_ucode_dccm_start_address_linker.find(bb);
            if (baseband_to_ucode_dccm_start_address_linker.end() == it)
            {
                LOG_ERROR << CPU_TYPE_TO_STRING[m_tracerType] << "Log collector doesn't support baseband type (type code is " << bb << ")" << endl;
                return false;
            }
            linker_start_address = it->second;
        }

        // calculate the difference between the buffer start address as dirver exposes it to the buffer start address as fw/ucode exposes it
        unsigned ahbToLinkerDelta = ahb_start_address - linker_start_address;

        // retrieve fw/ucode offset from peripheral_memory_start_linker_address
        DWORD addressToReadLogOffset = logTracerTypeToLogOffsetAddress[m_tracerType];
        DWORD logOffset = 0;
        if (!m_device->GetDriver()->Read(addressToReadLogOffset, logOffset))
        {
            LOG_ERROR << "Log collector failed to read log offset address" << endl;
            return false;
        }

        // calculate first address of fw/ucode log buffer
        m_logAddress = ahbToLinkerDelta + logOffset;
        return true;
    }

    void LogCollector::ParseModuelLevel(string moduleString)
    {
        for (int i = 0; i < NUM_MODULES; i++)
        {
            if (moduleString.find(module_names[i]) == 0)
            {
                // + 1 for '='
                string levels = moduleString.substr(module_names[i].size() + 1);

                if (levels.find("V") != string::npos)
                {
                    m_configuration.m_modulesVerbosity[i].verbose_level_enable = 1;
                }
                if (levels.find("I") != string::npos)
                {
                    m_configuration.m_modulesVerbosity[i].info_level_enable = 1;
                }
                if (levels.find("E") != string::npos)
                {
                    m_configuration.m_modulesVerbosity[i].error_level_enable = 1;
                }
                if (levels.find("W") != string::npos)
                {
                    m_configuration.m_modulesVerbosity[i].warn_level_enable = 1;
                }
            }
        }
    }

    string LogCollector::GetModuleVerbosityConfigurationValue()
    {
        stringstream ss;
        for (int i = 0; i < NUM_MODULES; i++)
        {
            ss << module_names[i] << configuration_parameter_value_delimiter;
            if (m_configuration.m_modulesVerbosity[i].verbose_level_enable)
            {
                ss << "V";
            }
            if (m_configuration.m_modulesVerbosity[i].info_level_enable)
            {
                ss << "I";
            }
            if (m_configuration.m_modulesVerbosity[i].error_level_enable)
            {
                ss << "E";
            }
            if (m_configuration.m_modulesVerbosity[i].warn_level_enable)
            {
                ss << "W";
            }
            ss << configuration_modules_delimiter;
        }
        return ss.str();
    }

    bool LogCollector::SetConfigurationParamerter(const string& parameter, const string& value)
    {
        try
        {
            if (log_collector::polling_interval == parameter)
            {
                m_configuration.m_pollingIntervalMs = chrono::milliseconds(atoi(value.c_str()));
                LOG_DEBUG << "Polling Interval was set to " << m_configuration.m_pollingIntervalMs.count() << " ms" << endl;
            }
            else if (log_collector::result_file_suffix == parameter)
            {
                m_configuration.m_logFileSuffix = value;
                LOG_DEBUG << "Result path = " << m_configuration.m_logFileSuffix << endl;
            }
            else if (log_collector::module_level_prefix == parameter)
            {
                ParseModuelLevel(value);
            }
            else if (log_collector::record_to_file == parameter)
            {
                m_configuration.m_recordToFile = (value == true_str);
                LOG_DEBUG << "Record to file is set to " << (m_configuration.m_recordToFile ? true_str : false_str) << endl;
            }
            else if (log_collector::log_fragment_size == parameter)
            {
                m_configuration.m_fileFragmentSize = atoi(value.c_str()) * 1024 * 1024;
                LOG_DEBUG << "File fragment size is " << m_configuration.m_fileFragmentSize << endl;
            }
            else
            {
                LOG_WARNING << "No such configuration parameter" << endl;
            }
            return true;
        }
        catch (...)
        {
            LOG_ERROR << "Got exception while trying to set a value to parameter " + parameter << endl;
            return false;
        }

    }

    string LogCollector::GetConfigurationParameterValue(const string& parameter, bool& success)
    {
        success = true;
        if (log_collector::polling_interval == parameter)
        {
            stringstream ss;
            ss << m_configuration.m_pollingIntervalMs.count();
            return ss.str();
        }
        else if (log_collector::result_file_suffix == parameter)
        {
            return m_configuration.m_logFileSuffix;
        }
        else if (log_collector::module_level_prefix == parameter)
        {
            return GetModuleVerbosityConfigurationValue();
        }
        else if (log_collector::record_to_file == parameter)
        {
            return (m_configuration.m_recordToFile? true_str : false_str);
        }
        else if (log_collector::log_fragment_size == parameter)
        {
            stringstream ss;
            ss << m_configuration.m_fileFragmentSize;
            return ss.str();
        }
        else
        {
            success = false;
            return "No such configuration parameter";
        }
    }

    string LogCollector::GetConfigurationDump()
    {
        bool success;
        stringstream ss;
        ss << polling_interval << GetConfigurationParameterValue(polling_interval, success);
        ss << record_to_file << GetConfigurationParameterValue(record_to_file, success);
        ss << log_fragment_size << GetConfigurationParameterValue(log_fragment_size, success);
        ss << result_file_suffix << GetConfigurationParameterValue(result_file_suffix, success);
        ss << module_level_prefix << GetConfigurationParameterValue(module_level_prefix, success);
        return ss.str();
    }

}


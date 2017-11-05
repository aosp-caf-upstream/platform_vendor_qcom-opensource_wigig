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

#ifndef LOG_COLLECTOR_DEFINITIONS
#define LOG_COLLECTOR_DEFINITIONS

#pragma once

#ifndef _WINDOWS    // Linux
#include <unistd.h>
#include <getopt.h>
#include <err.h>
#else                // Windows
#include <windows.h>
#endif  //#ifdef _WINDOWS

#include <iostream>
#include <fstream>
#include <string>
#include <stdarg.h>
#include <ctime>
#include <map>
#include <vector>

#include "HostManagerDefinitions.h"

namespace log_collector
{
    //#pragma region type definitions

    enum MODULES
    {
        SYSTEM,
        DRIVERS,
        MAC_MON,
        HOST_CMD,
        PHY_MON,
        INFRA,
        CALIBS,
        TXRX,
        RAD_MGR,
        SCAN,
        MLME,
        L2_MGR,
        DISC,
        MGMT_SRV,
        SEC_PSM,
        WBE_MNGR,
        NUM_MODULES,
    };

    enum LogCollectorStatus
    {
        lcSuccess,
        lcFailedToCreateNewFile,
        lcFailedToAllocateLogBuffer,
        lcInvalidDevice,
        lcInvalidDriver,
        lcFailedToReadBuffer
    };

    typedef uint32_t u32;
    typedef int32_t s32;
    typedef unsigned int uint;

#ifdef _WINDOWS
    struct module_level_enable { /* Little Endian */
        char error_level_enable : 1;
        char warn_level_enable : 1;
        char info_level_enable : 1;
        char verbose_level_enable : 1;
        char reserved0 : 4;

        module_level_enable& operator=(const module_level_enable& other)
        {
            if (this == &other)
            {
                return *this;
            }
            error_level_enable = other.error_level_enable;
            warn_level_enable = other.warn_level_enable;
            info_level_enable = other.info_level_enable;
            verbose_level_enable = other.verbose_level_enable;
            reserved0 = other.reserved0;
            return *this;
        }
    };
    struct log_trace_header { /* Little Endian */
        uint strring_offset : 20;
        uint module : 4; /* module that outputs the trace */
        uint level : 2;
        uint parameters_num : 2; /* [0..3] */
        uint is_string : 1; /* indicate if the printf uses %s */
        uint signature : 3; /* should be 5 (2'101) in valid header */
    };
#else
    struct module_level_enable { /* Little Endian */
        uint error_level_enable : 1;
        uint warn_level_enable : 1;
        uint info_level_enable : 1;
        uint verbose_level_enable : 1;
        uint reserved0 : 4;

        module_level_enable& operator=(const module_level_enable& other)
        {
            if (this == &other)
            {
                return *this;
            }
            error_level_enable = other.error_level_enable;
            warn_level_enable = other.warn_level_enable;
            info_level_enable = other.info_level_enable;
            verbose_level_enable = other.verbose_level_enable;
            reserved0 = other.reserved0;
            return *this;
        }

    } __attribute__((packed));

    struct log_trace_header { /* Little Endian */
                              /* the offset of the trace string in the strings sections */
        uint strring_offset : 20;
        uint module : 4; /* module that outputs the trace */
                         /*    0 - Error
                         1- WARN
                         2 - INFO
                         3 - VERBOSE */
        uint level : 2;
        uint parameters_num : 2; /* [0..3] */
        uint is_string : 1; /* indicate if the printf uses %s */
        uint signature : 3; /* should be 5 (2'101) in valid header */
    } __attribute__((packed));
#endif

    union log_event {
        struct log_trace_header hdr;
        u32 param;
    };

    struct log_table_header {
        u32 write_ptr; /* incremented by trace producer every write */
        struct module_level_enable module_level_enable[NUM_MODULES];
        union log_event evt[0];
    };

    enum {
        str_mask = 0xFFFFF,
    };

    enum missing_log_lines_reason {
        NO_MISSED_LOGS = 0,
        RPTR_LARGER_THAN_WPTR,
        BUFFER_OVERRUN,
        INVALID_SIGNATURE
    };

    struct RawLogLine
    {
        RawLogLine(unsigned module, unsigned level, unsigned signature, unsigned strOffset, unsigned isString, const std::vector<unsigned>& params) :
            m_strOffset(strOffset),
            m_module(module),
            m_level(level),
            m_params(params),
            m_isString(isString),
            m_signature(signature),
            m_numOfMissingLogDwords(0),
            m_missingLogsReason(NO_MISSED_LOGS)
        {}

        RawLogLine(missing_log_lines_reason missingLogLinesReason, unsigned numOfMissingLogLines) :
            m_strOffset(0),
            m_module(0),
            m_level(0),
            m_isString(0),
            m_signature(0),
            m_numOfMissingLogDwords(numOfMissingLogLines),
            m_missingLogsReason(missingLogLinesReason)
        {}

        unsigned m_strOffset;
        unsigned m_module;
        unsigned m_level;
        std::vector<unsigned> m_params;
        unsigned m_isString;
        unsigned m_signature;
        unsigned m_numOfMissingLogDwords;
        missing_log_lines_reason m_missingLogsReason;
    };

    //#pragma endregion

    //#pragma region constants

    // 50 MB file size
    const int MAX_FILE_FRAGMENT_SIZE = 1024 * 1024 * 50;

    // RGFs containing log buffer addresses
    // FW log address
    const int REG_FW_USAGE_1 = 0x880004;
    // uCode log address
    const int REG_FW_USAGE_2 = 0x880008;

    // Firmware version RGFs
    const int FW_VERSION_MAJOR = 0x880a2c;
    const int FW_VERSION_MINOR = 0x880a30;
    const int FW_VERSION_SUB = 0x880a34;
    const int FW_VERSION_BUILD = 0x880a38;

    // Firmware Compilation Time RGFs
    const int FW_COMP_TIME_HOUR = 0x880a14;
    const int FW_COMP_TIME_MINUTE = 0x880a18;
    const int FW_COMP_TIME_SECOND = 0x880a1c;
    const int FW_COMP_TIME_DAY = 0x880a20;
    const int FW_COMP_TIME_MONTH = 0x880a24;
    const int FW_COMP_TIME_YEAR = 0x880a28;

    // Log buffer offsets
    // FW log address offset
    //const int peripheral_memory_start_linker_address = 0x840000;
    // uCode log address offset
    //const int ucode_dccm = 0x800000;
    //const int UCODE_LOG_ADDRESS_OFFSET = 0;

    // Entries in the fw log buf
    const size_t fw_log_buf_entries = 0x1000 / 4;

    const std::string module_names[NUM_MODULES] = { "SYSTEM", "DRIVERS", "MAC_MON", "HOST_CMD", "PHY_MON", "INFRA", "CALIBS", "TXRX", "RAD_MGR", "SCAN", "MLME", "L2_MGR", "DISC", "MGMT_SRV", "SEC_PSM", "WBE_MNGR" };
    const char *const levels[] = { "E", "W", "I", "V" };
    const std::string VIEW = "VIEW";
    static std::map<BasebandType, unsigned> baseband_to_peripheral_memory_start_address_linker = { { BASEBAND_TYPE_SPARROW, 0x840000 },{ BASEBAND_TYPE_TALYN, 0x840000 } };
    static std::map<BasebandType, unsigned> baseband_to_peripheral_memory_start_address_ahb = { { BASEBAND_TYPE_SPARROW, 0x908000 },{ BASEBAND_TYPE_TALYN, 0xA20000 } };
    static std::map<BasebandType, unsigned> baseband_to_ucode_dccm_start_address_linker = { { BASEBAND_TYPE_SPARROW, 0x800000 },{ BASEBAND_TYPE_TALYN, 0x800000 } };
    static std::map<BasebandType, unsigned> baseband_to_ucode_dccm_start_address_ahb = { { BASEBAND_TYPE_SPARROW, 0x940000 },{ BASEBAND_TYPE_TALYN, 0xA78000 } };
    static std::map<CpuType, int> logTracerTypeToLogOffsetAddress = { { CPU_TYPE_FW, REG_FW_USAGE_1 },{ CPU_TYPE_UCODE, REG_FW_USAGE_2 } };

    // Config file filename
#ifdef _WINDOWS
    const char* const DEFAULT_CONFIG_FILE_NAME = "wigig_logcollector.ini";
#else
    const char* const DEFAULT_CONFIG_FILE_NAME = "/etc/wigig_logcollector.ini";
#endif

    //#pragma endregion

    // configuration definitions
    const std::string polling_interval = "polling_interval_ms=";
    const std::string result_file_suffix = "result_file_suffix=";
    const std::string module_level_prefix = "MODULE_LEVEL_";
    const std::string record_to_file = "record_to_file=";
    const std::string log_fragment_size = "log_fragment_size=";

    const std::string m_configFileName = "LogConfig.txt";
    const std::string true_str = "TRUE";
    const std::string false_str = "FALSE";
    const std::string configuration_parameter_value_delimiter = "=";
    const std::string configuration_modules_delimiter = ",";
}

#endif // !LOG_COLLECTOR_DEFINITIONS


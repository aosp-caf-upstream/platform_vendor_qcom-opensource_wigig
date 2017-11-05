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

#ifndef _LOG_COLLECTOR_CONFIGURATION_H
#define _LOG_COLLECTOR_CONFIGURATION_H
#pragma once

#include <chrono>
#include <string>
#include <map>
#include <algorithm>
#include <string>
#include "FileSystemOsAbstraction.h"
#include "LogCollectorDefinitions.h"
using namespace std;

namespace log_collector
{
    struct LogCollectorConfiguration
    {
    public:

        void ResetValue()
        {
            m_pollingIntervalMs = chrono::milliseconds(100);
            m_recordToFile = false;
            m_fileFragmentSize = MAX_FILE_FRAGMENT_SIZE;
            m_logFileSuffix = "";
            m_logType = CPU_TYPE_FW;
            for (auto& module : m_modulesVerbosity)
            {
                module.verbose_level_enable = 0;
                module.info_level_enable = 1;
                module.error_level_enable = 1;
                module.warn_level_enable = 1;
                module.reserved0 = 0;
            }
        }

        LogCollectorConfiguration()
        {
            ResetValue();
        }

        std::chrono::milliseconds m_pollingIntervalMs; // interval between two consecutive log polling in milliseconsd
        bool m_recordToFile;
        int m_fileFragmentSize;
        string m_logFileSuffix;
        CpuType m_logType;
        module_level_enable m_modulesVerbosity[NUM_MODULES];
    };
}
#endif // ! _LOG_COLLECTOR_CONFIGURATION_H



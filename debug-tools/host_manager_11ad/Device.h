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

#ifndef _11AD_DEVICE_H_
#define _11AD_DEVICE_H_

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <memory>
#include <mutex>

#include "OperatingSystemConstants.h"
#include "access_layer_11ad/Definitions.h"
#include "../DebugLogger.h"
#include "../DriverAPI.h"
#include "AccessLayerAPI.h"
#include "EventsDefinitions.h"
#include "HostDefinitions.h"
#include "LogCollector.h"
#include <thread>

using namespace std;

class Device
{
public:
    explicit Device(const string& deviceName);

    virtual ~Device() {}

    // Functionality common to all devices

    bool Init();

    bool Fini();

    bool IsValid() const { return m_driver != nullptr; }

    const string& GetDeviceName(void) const { return m_deviceName; }

    bool SendWmi(DWORD command, vector<DWORD> payload)
    {
        //do something with params
        (void)command;
        (void)payload;
        return false;

    }

    vector<DWORD> GetWmiEvent()
    {

        vector<DWORD> ret;
        return ret;
    }

    std::vector<unique_ptr<HostManagerEventBase>> Poll(void);

    // ************************** [END] Device API **********************//

    BasebandType GetBasebandType();

    bool GetSilenceMode() const { return m_isSilent; }

    void SetSilenceMode(bool silentMode) { m_isSilent = silentMode;    }

    DriverAPI* GetDriver() const { return m_driver.get(); }

    //TODO: make private after blocking simultaneous driver actions
    mutable mutex m_mutex; // threads synchronization

    bool GetIsAlive(void) const { return m_isAlive; }

    const FwVersion& GetFwVersion(void) const { return m_fwVersion; }

    const FwTimestamp& GetFwTimestamp(void) const { return m_fwTimestamp; }

    DWORD GetCapabilities(void) const { return m_capabilitiesMask; }

    // ************************ Log Collector *************************//
    bool StartLogCollector(); // TODO - use GetLogCollector functions instead of this one and remove this function

    bool StopLogCollector(); // TODO - use GetLogCollector functions instead of this one and remove this function

    log_collector::LogCollector* GetLogCollector(CpuType type);

    // *********************** [END] Log Collector *******************//

    // ************************ Custom Regs *************************//
    bool AddCustomRegister(const string& name, int address);

    bool RemoveCustomRegister(const string& name);

    std::map<string, int>& GetCustomRegisters();
    // *********************** [END] Custom Regs *******************//

private:
    BasebandType m_basebandType;
    unique_ptr<DriverAPI> m_driver;
    bool m_isSilent;
    bool m_isAlive;
    string m_deviceName;
    FwVersion m_fwVersion;
    FwTimestamp m_fwTimestamp;
    map<CpuType,unique_ptr<log_collector::LogCollector>> m_logCollectors;

    BasebandType ReadBasebandType() const;

    // Custom registers
    std::map<std::string, int> m_customRegisters;

    bool RegisterDriverControlEvents();

    // Internal service for fetching the FW version and compile time
    bool ReadDeviceFwInfoInternal(FwVersion& fwVersion, FwTimestamp& fwTimestamp) const;

    void PollFwVersion(vector<unique_ptr<HostManagerEventBase>>& events);
    void PollLogs(vector<unique_ptr<HostManagerEventBase>>& events);

    enum FwInfoRegisters : DWORD
    {
        FW_VERSION_MAJOR_REGISTER        = 0x880a2c,
        FW_VERSION_MINOR_REGISTER        = 0x880a30,
        FW_VERSION_SUB_MINOR_REGISTER    = 0x880a34,
        FW_VERSION_BUILD_REGISTER        = 0x880a38,
        FW_TIMESTAMP_HOUR_REGISTER        = 0x880a14,
        FW_TIMESTAMP_MINUTE_REGISTER    = 0x880a18,
        FW_TIMESTAMP_SECOND_REGISTER    = 0x880a1c,
        FW_TIMESTAMP_DAY_REGISTER        = 0x880a20,
        FW_TIMESTAMP_MONTH_REGISTER        = 0x880a24,
        FW_TIMESTAMP_YEAR_REGISTER        = 0x880a28
    };

    // Capabilities region:
    DWORD m_capabilitiesMask;

    // each value in the enum represents a bit in the DWORD (the values would be 1,2,3,4...)
    enum CAPABILITIES : DWORD
    {
        DRIVER_CONTROL_EVENTS // capability of the driver to send and receive driver control commands and events
    };

    void SetCapability(CAPABILITIES capability, bool isTrue);
    bool IsCapabilitySet(CAPABILITIES capability) const;
};

#endif // !_11AD_DEVICE_H_

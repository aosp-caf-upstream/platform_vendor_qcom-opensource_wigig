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
#ifndef _COMMANDSEXECUTOR_H_
#define _COMMANDSEXECUTOR_H_

#include <string>
#include <vector>
#include "HostDefinitions.h"

struct RegisterData
{
    std::string m_name;
    std::string m_value;
};

struct DeviceData
{
    std::string m_deviceName;
    bool m_associated = false;
    int m_signalStrength = 0;
    int m_fwAssert = 0;
    int m_uCodeAssert = 0;
    int m_mcs = 0;
    int m_channel = 0;
    FwVersion m_fwVersion = {};
    std::string m_bootloaderVersion;
    string m_mode;
    FwTimestamp m_compilationTime = {};
    std::string m_hwType;
    std::string m_hwTemp;
    std::string m_rfType;
    std::string m_rfTemp;
    std::string m_boardFile;
    std::vector<int> m_rf;
    std::vector<RegisterData> m_fixedRegisters;
    std::vector<RegisterData> m_customRegisters;
};

struct HostData
{
    std::string m_hostManagerVersion; //should it be a structure like FwVersion??
    std::string m_hostAlias;
    std::string m_hostIP;
    std::vector<DeviceData> m_devices;
};

class Host;

class CommandsExecutor
{
public:
    CommandsExecutor(Host& host);

    bool GetHostData(HostData& data);
    bool SetHostAlias(const std::string& name);
    bool AddDeviceRegister(const std::string& deviceName,  const std::string& name, uint32_t address);
    bool RemoveDeviceRegister(const std::string& deviceName, const std::string& name);

private:
    Host& m_host; // a refernce to the host (enables access to deviceManager and hostInfo)

    //for testing purposes only
    bool GetTestHostData(HostData& data);
    bool SetTestHostAlias(const std::string& name);
    bool AddTestDeviceRegister(const std::string& deviceName, const std::string& name, uint32_t address);
    bool RemoveTestDeviceRegister(const std::string& deviceName, const std::string& name);
};


#endif // _COMMANDSEXECUTOR_H_

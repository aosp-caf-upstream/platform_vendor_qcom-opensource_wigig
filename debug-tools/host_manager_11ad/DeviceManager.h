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

#ifndef _DEVICEMANAGER_H_
#define _DEVICEMANAGER_H_

#include <string>
#include <set>
#include <unordered_map>
#include <thread>
#include <future>
#include <atomic>

#include "HostDefinitions.h"
#include "Device.h"
#include "CommandsExecutor.h"

enum DeviceManagerOperationStatus
{
    dmosSuccess,
    dmosNoSuchConnectedDevice, // the given device name is not part of m_conncectedDevices
    dmosFailedToReadFromDevice,
    dmosFailedToWriteToDevice,
    dmosFailedToResetInterface,
    dmosFailedToResetSw,
    dmosFailedToAllocatePmc,
    dmosFailedToDeallocatePmc,
    dmosFailedToCreatePmcFile,
    dmosFailedToSendWmi,
    dmosInvalidAddress,
    dmosFail, // general failure. try to avoid using it
    dmosSilentDevice
};

class DeviceManager
{
public:
    DeviceManager(std::promise<void>& eventsTCPServerReadyPromise);
    ~DeviceManager();
    string GetDeviceManagerOperationStatusString(DeviceManagerOperationStatus status);

    DeviceManagerOperationStatus GetDevices(std::set<std::string>& devicesNames);
    DeviceManagerOperationStatus Read(const std::string& deviceName, DWORD address, DWORD& value);
    DeviceManagerOperationStatus Write(const std::string& deviceName, DWORD address, DWORD value);
    DeviceManagerOperationStatus ReadBlock(const std::string& deviceName, DWORD address, DWORD blockSize, vector<DWORD>& values);
    DeviceManagerOperationStatus WriteBlock(const std::string& deviceName, DWORD address, const vector<DWORD>& values);
    DeviceManagerOperationStatus InterfaceReset(const std::string& deviceName);
    DeviceManagerOperationStatus SwReset(const std::string& deviceName);

    DeviceManagerOperationStatus AllocPmc(const std::string& deviceName, unsigned descSize, unsigned descNum, std::string& errorMsg);
    DeviceManagerOperationStatus DeallocPmc(const std::string& deviceName, std::string& outMessage);
    DeviceManagerOperationStatus CreatePmcFile(const std::string& deviceName, unsigned refNumber, std::string& outMessage);
    DeviceManagerOperationStatus FindPmcFile(const std::string& deviceName, unsigned refNumber, std::string& outMessage);

    DeviceManagerOperationStatus SendWmi(const std::string& deviceName, DWORD command, const std::vector<DWORD>& payload);
    DeviceManagerOperationStatus OpenInterface(const std::string& deviceName); // for backward compatibility
    DeviceManagerOperationStatus CloseInterface(const std::string& deviceName);
    DeviceManagerOperationStatus SetDriverMode(const std::string& deviceName, int newMode, int& oldMode);
    DeviceManagerOperationStatus DriverControl(const std::string& deviceName, uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize);
    DeviceManagerOperationStatus GetDeviceSilentMode(const std::string& deviceName, bool& silentMode);
    DeviceManagerOperationStatus SetDeviceSilentMode(const std::string& deviceName, bool silentMode);
    DeviceManagerOperationStatus GetDeviceCapabilities(const std::string& deviceName, DWORD& capabilities);

    // Host Status Update
    bool GetDeviceStatus(std::vector<DeviceData>& devicesData);

    bool AddRegister(const std::string& deviceName, const std::string& registerName, int address);
    bool RemoveRegister(const std::string& deviceName, const std::string& registerName);

    // Log collection
    bool GetLogCollectionMode() const;
    void SetLogCollectionMode(bool collectLogs);
    bool SetLogCollectionConfiguration(const vector<string>& deviceNames, const vector<string>& cpuTypeNames, const string& parameter, const string& value, string& errorMessage);
    string GetLogCollectionConfiguration(const vector<string>& deviceNames, const vector<string>& cpuTypeNames, string parameter);
    string DumpLogCollectionConfiguration(const vector<string>& deviceNames, const vector<string>& cpuTypeNames);

private:
    void PeriodicTasks(std::promise<void>& eventsTCPServerReadyPromise);
    void UpdateConnectedDevices();
    void CreateDevice(const std::string& deviceName);
    void DeleteDevice(const std::string& deviceName);
    bool IsDeviceSilent(const std::string& deviceName);
    std::shared_ptr<Device> GetDeviceByName(const std::string& deviceName);

    std::unordered_map<std::string, std::shared_ptr<Device>> m_devices; // map from unique string (unique inside a host) to a connected device
    unsigned const m_deviceManagerRestDurationMs;
    std::thread m_deviceManager;
    std::mutex m_connectedDevicesMutex;
    std::atomic<bool> m_terminate;

    // Collect logs flag
    bool m_collectLogs;

    const static std::unordered_map<int, std::string> m_rfTypeToString;
    const static std::unordered_map<int, std::string> m_basebandTypeToString;
    const static std::unordered_map<int, std::string> m_boardfileTypeToString;
};

#endif // !_DEVICEMANAGER_H_

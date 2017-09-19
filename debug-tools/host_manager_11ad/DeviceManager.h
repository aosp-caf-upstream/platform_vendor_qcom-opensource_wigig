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
#include <unordered_set>
#include <thread>
#include <future>
#include <atomic>

#include "HostDefinitions.h"
#include "Device.h"

using namespace std;

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

    DeviceManagerOperationStatus GetDevices(set<string>&);
    DeviceManagerOperationStatus Read(string deviceName, DWORD address, DWORD& value);
    DeviceManagerOperationStatus Write(string deviceName, DWORD address, DWORD value);
    DeviceManagerOperationStatus ReadBlock(string deviceName, DWORD address, DWORD blockSize, vector<DWORD>& values);
    DeviceManagerOperationStatus WriteBlock(string deviceName, DWORD address, const vector<DWORD>& values);
    DeviceManagerOperationStatus InterfaceReset(string deviceName);
    DeviceManagerOperationStatus SwReset(string deviceName);

    DeviceManagerOperationStatus AllocPmc(string deviceName, unsigned descSize, unsigned descNum, std::string& errorMsg);
    DeviceManagerOperationStatus DeallocPmc(string deviceName, std::string& outMessage);
    DeviceManagerOperationStatus CreatePmcFile(string deviceName, unsigned refNumber, std::string& outMessage);
    DeviceManagerOperationStatus FindPmcFile(string deviceName, unsigned refNumber, std::string& outMessage);

    DeviceManagerOperationStatus SendWmi(string deviceName, DWORD command, const vector<DWORD>& payload);
    DeviceManagerOperationStatus OpenInterface(string deviceName); // for backward compatibility
    DeviceManagerOperationStatus CloseInterface(string deviceName);
    DeviceManagerOperationStatus SetDriverMode(string deviceName, int newMode, int& oldMode);
    DeviceManagerOperationStatus DriverControl(string deviceName, uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize);
    DeviceManagerOperationStatus GetDeviceSilentMode(string deviceName, bool& silentMode);
    DeviceManagerOperationStatus SetDeviceSilentMode(string deviceName, bool silentMode);

private:
    void PeriodicTasks(std::promise<void>& eventsTCPServerReadyPromise);
    void UpdateConnectedDevices();
    void CreateDevice(string deviceName);
    void DeleteDevice(string deviceName);
    bool IsDeviceSilent(string deviceName);
    string GetStatusBarString(shared_ptr<Device> device);

    unordered_map<string, shared_ptr<Device>> m_devices; // map from unique string (unique inside a host) to a connected device
    unsigned const m_deviceManagerRestDurationMs;
    thread m_deviceManager;
    mutex m_connectedDevicesMutex;
    atomic<bool> m_terminate;
};

#endif // !_DEVICEMANAGER_H_

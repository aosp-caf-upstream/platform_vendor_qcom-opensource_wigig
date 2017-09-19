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

#include <chrono>
#include <thread>

#include "DeviceManager.h"
#include "Utils.h"
#include "AccessLayerAPI.h"
#include "DebugLogger.h"
#include "Utils.h"
#include "Host.h"
#include <map>
#include <sstream>
#include <string>
#include <exception>

DeviceManager::DeviceManager(std::promise<void>& eventsTCPServerReadyPromise) :
    m_deviceManagerRestDurationMs(500),
    m_terminate(false)
{
    m_deviceManager = thread(&DeviceManager::PeriodicTasks, this, std::ref(eventsTCPServerReadyPromise));
}

DeviceManager::~DeviceManager()
{
    m_terminate = true;
    m_deviceManager.join();
}

string DeviceManager::GetDeviceManagerOperationStatusString(DeviceManagerOperationStatus status)
{
    switch (status)
    {
    case dmosSuccess:
        return "Successful operation";
    case dmosNoSuchConnectedDevice:
        return "Unknown device";
    case dmosFailedToReadFromDevice:
        return "Read failure";
    case dmosFailedToWriteToDevice:
        return "Write failure";
    case dmosFailedToResetInterface:
        return "Reset interface failure";
    case dmosFailedToResetSw:
        return "SW reset failure";
    case dmosFailedToAllocatePmc:
        return "Allocate PMC failure";
    case dmosFailedToDeallocatePmc:
        return "Deallocate PMC failure";
    case dmosFailedToCreatePmcFile:
        return "Create PMC file failure";
    case dmosFailedToSendWmi:
        return "Send WMI failure";
    case dmosFail:
        return "Operation failure";
    case dmosSilentDevice:
        return "Device is in silent mode";
    default:
        return "DeviceManagerOperationStatus is unknown ";
    }
}

DeviceManagerOperationStatus DeviceManager::GetDevices(set<string>& devicesNames)
{
    devicesNames.clear();
    m_connectedDevicesMutex.lock();
    for (auto& device : m_devices)
    {
        devicesNames.insert(device.first);
    }
    m_connectedDevicesMutex.unlock();
    return dmosSuccess;
}

DeviceManagerOperationStatus DeviceManager::OpenInterface(string deviceName)
{
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_connectedDevicesMutex.unlock();
        return dmosSuccess;
    }
    m_connectedDevicesMutex.unlock();
    return dmosNoSuchConnectedDevice;
}

DeviceManagerOperationStatus DeviceManager::CloseInterface(string deviceName)
{
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_connectedDevicesMutex.unlock();
        return dmosSuccess;
    }
    m_connectedDevicesMutex.unlock();
    return dmosNoSuchConnectedDevice;
}

DeviceManagerOperationStatus DeviceManager::Read(string deviceName, DWORD address, DWORD& value)
{
    if (IsDeviceSilent(deviceName))
    {
        return dmosSilentDevice;
    }

    if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
    {
        return dmosInvalidAddress;
    }

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->Read(address, value);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            value = Utils::REGISTER_DEFAULT_VALUE;
            status = dmosFailedToReadFromDevice;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        value = Utils::REGISTER_DEFAULT_VALUE;
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::Write(string deviceName, DWORD address, DWORD value)
{
    if (IsDeviceSilent(deviceName))
    {
        return dmosSilentDevice;
    }

    if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
    {
        return dmosInvalidAddress;
    }

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->Write(address, value);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToWriteToDevice;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::ReadBlock(string deviceName, DWORD address, DWORD blockSize, vector<DWORD>& values)
{
    if (IsDeviceSilent(deviceName))
    {
        return dmosSilentDevice;
    }

    if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
    {
        return dmosInvalidAddress;
    }

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->ReadBlock(address, blockSize, values);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            vector<DWORD> defaultValues(blockSize, Utils::REGISTER_DEFAULT_VALUE);
            status = dmosFailedToReadFromDevice;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        vector<DWORD> defaultValues(blockSize, Utils::REGISTER_DEFAULT_VALUE);
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::WriteBlock(string deviceName, DWORD address, const vector<DWORD>& values)
{
    if (IsDeviceSilent(deviceName))
    {
        return dmosSilentDevice;
    }

    if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
    {
        return dmosInvalidAddress;
    }

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->WriteBlock(address, values);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToWriteToDevice;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::InterfaceReset(string deviceName)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        m_devices[deviceName]->GetDriver()->Reset(); // TODO - we need to separate between SW reset and interface reset
        status = dmosSuccess;
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::SwReset(string deviceName)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = false; //  m_devices[deviceName]->GetDriver()->Reset(); // TODO
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToResetSw;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::SetDriverMode(string deviceName, int newMode, int& oldMode)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->SetDriverMode(newMode, oldMode);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToResetSw;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::DriverControl(string deviceName, uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->DriverControl(Id, inBuf, inBufSize, outBuf, outBufSize);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToReadFromDevice;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::AllocPmc(string deviceName, unsigned descSize, unsigned descNum, string& errorMsg)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();

    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();

        bool success = m_devices[deviceName]->GetDriver()->AllocPmc(descSize, descNum, errorMsg);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            LOG_ERROR << "Failed to allocate PMC ring: " << errorMsg << std::endl;
            status = dmosFailedToAllocatePmc;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        errorMsg = "No device found";
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::DeallocPmc(string deviceName, std::string& outMessage)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();

        bool success = m_devices[deviceName]->GetDriver()->DeallocPmc(outMessage);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            LOG_ERROR << "Failed to de-allocate PMC ring: " << outMessage << std::endl;
            status = dmosFailedToDeallocatePmc;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::CreatePmcFile(string deviceName, unsigned refNumber, std::string& outMessage)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->CreatePmcFile(refNumber, outMessage);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToCreatePmcFile;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::FindPmcFile(string deviceName, unsigned refNumber, std::string& outMessage)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->GetDriver()->FindPmcFile(refNumber, outMessage);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToCreatePmcFile;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::SendWmi(string deviceName, DWORD command, const vector<DWORD>& payload)
{
    if (IsDeviceSilent(deviceName))
    {
        return dmosSilentDevice;
    }

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        m_devices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_devices[deviceName]->SendWmi(command, payload);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToSendWmi;
        }
        m_devices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

void DeviceManager::CreateDevice(string deviceName)
{
    m_connectedDevicesMutex.lock();
    shared_ptr<Device> device(new Device(deviceName));
    m_devices.insert(make_pair(deviceName, device));
    m_connectedDevicesMutex.unlock();

    if (device->GetIsAlive())
    {
        // Notify that new device discovered, also relevant for case of FW update
        Host::GetHost().PushEvent(NewDeviceDiscoveredEvent(deviceName, device->GetFwVersion(), device->GetFwTimestamp()));
    }
    else
    {
        // Delete unresponsive device, lock is acquired inside
        // Note: Can happen if device becomes unresponsive after enumeration
        DeleteDevice(deviceName);
        LOG_INFO << "Created unresponsive device '" << deviceName << "', removing..." << endl;
    }
}

void DeviceManager::DeleteDevice(string deviceName)
{
    m_connectedDevicesMutex.lock();
    // make sure that no client is using this object
    m_devices[deviceName]->m_mutex.lock();
    // no need that the mutex will be still locked since new clients have to get m_connectedDevicesMutex before they try to get m_mutex
    m_devices[deviceName]->m_mutex.unlock();
    m_devices.erase(deviceName);
    m_connectedDevicesMutex.unlock();
}

void DeviceManager::UpdateConnectedDevices()
{
    vector<string> devicesForRemove;
    // Delete unresponsive devices
    m_connectedDevicesMutex.lock();
    for (auto& connectedDevice : m_devices)
    {
        if (connectedDevice.second->GetSilenceMode()) //GetSilenceMode returns true if the device is silent the skip th update
        {
            continue;
        }
        DWORD value = Utils::REGISTER_DEFAULT_VALUE;
        connectedDevice.second->m_mutex.lock();
        if (!connectedDevice.second->GetDriver()->Read(BAUD_RATE_REGISTER, value))
        {
            devicesForRemove.push_back(connectedDevice.first);
        }
        connectedDevice.second->m_mutex.unlock();
    }
    m_connectedDevicesMutex.unlock();

    for (auto& device : devicesForRemove)
    {
        DeleteDevice(device);
    }

    devicesForRemove.clear();

    set<string> currentlyConnectedDevices = AccessLayer::GetDrivers();

    // delete devices that arn't connected anymore according to enumeration
    for (auto& connectedDevice : m_devices)
    {
        if (connectedDevice.second->GetSilenceMode()) //GetSilenceMode retunrs true if the device is silent the skip th update
        {
            continue;
        }

        if (0 == currentlyConnectedDevices.count(connectedDevice.first))
        {
            devicesForRemove.push_back(connectedDevice.first);
        }
    }
    for (auto& device : devicesForRemove)
    {
        DeleteDevice(device);
    }

    // add new connected devices
    vector<string> newDevices;
    m_connectedDevicesMutex.lock();
    for (auto& currentlyConnectedDevice : currentlyConnectedDevices)
    {
        if (0 == m_devices.count(currentlyConnectedDevice))
        {
            newDevices.push_back(currentlyConnectedDevice);
        }
    }
    m_connectedDevicesMutex.unlock();

    for (auto& device : newDevices)
    {
        CreateDevice(device);
    }
}

void DeviceManager::PeriodicTasks(std::promise<void>& eventsTCPServerReadyPromise)
{
    // wait for events TCP server readiness before running the main loop
    auto status = eventsTCPServerReadyPromise.get_future().wait_for(std::chrono::seconds(5));
    if (status == std::future_status::timeout)
    {
        LOG_ERROR << "DeviceManager: Events TCP Server did not become ready before timeout duration has passed";
    }

    while (!m_terminate)
    {
        UpdateConnectedDevices();

        // get local copy of m_devices not to hold the lock
        m_connectedDevicesMutex.lock();
        auto devices = m_devices;
        m_connectedDevicesMutex.unlock();

        for (auto& device : devices)
        {
            if (device.second->GetSilenceMode()) //GetSilenceMode retunrs true if the device is silent the skip the periodic task
            {
                continue;
            }

            std::vector<unique_ptr<HostManagerEventBase>> events = device.second->Poll();
            for (const auto& event : events)
            {
                Host::GetHost().PushEvent(*event.get());
            }

            try
            {
                if (LOG_STATUS)
                {
                    string statusString = GetStatusBarString(device.second);
                    system("clear");
                    cout << "\033[2J\033[1;1H";
                    cout << "\r" << statusString << flush;
                }
            }
            catch (exception e)
            {
                cout << e.what() << endl;
            }
        }
        this_thread::sleep_for(std::chrono::milliseconds(m_deviceManagerRestDurationMs));
    }
}

bool DeviceManager::IsDeviceSilent(string deviceName)
{
    bool isSilent = false;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) <= 0)
    {
        m_connectedDevicesMutex.unlock();
        return isSilent;
    }

    if ((NULL == m_devices[deviceName].get()) || (!m_devices[deviceName]->IsValid()))
    {
        LOG_ERROR << "Invalid device pointer in IsDeviceSilent (NULL)" << endl;
        m_connectedDevicesMutex.unlock();
        return isSilent;
    }

    m_devices[deviceName]->m_mutex.lock();
    m_connectedDevicesMutex.unlock();

    isSilent = m_devices[deviceName]->GetSilenceMode();

    m_devices[deviceName]->m_mutex.unlock();

    return isSilent;
}


string DeviceManager::GetStatusBarString(shared_ptr<Device> device)
{
    return "Not Supported";
    /*try
    {
        lock_guard<mutex> lock(device->m_mutex);

        DWORD FwMajhexVal;
        device->GetDriver()->Read(0x880a2c, FwMajhexVal);
        string FwMaj = to_string(FwMajhexVal);

        DWORD FwMinhexVal;
        device->GetDriver()->Read( 0x880a30, FwMinhexVal);
        string FwMin = to_string(FwMinhexVal);

        DWORD FwBuildhexVal;
        device->GetDriver()->Read( 0x880a38, FwBuildhexVal);
        string FwBuild = to_string(FwBuildhexVal);

        DWORD RfStathexVal;
        device->GetDriver()->Read( 0x880A68, RfStathexVal);
        string RfStat = (RfStathexVal == 0) ? "RF_OK" : "RF_NO_COMM";

        DWORD FwErrorhexVal;
        device->GetDriver()->Read( 0x91F020, FwErrorhexVal);
        string FwError = (FwErrorhexVal == 0) ? "OK" : to_string(FwErrorhexVal);

        DWORD UcErrorhexVal;
        device->GetDriver()->Read( 0x91F028, UcErrorhexVal);
        string UcError = (UcErrorhexVal == 0) ? "OK" : to_string(UcErrorhexVal);

        DWORD FwStatehexVal;
        device->GetDriver()->Read( 0x880A44, FwStatehexVal);
        string FwState;
        switch (FwStatehexVal)
        {
        case 1:
            FwState = "FW_LOADED";
            break;
        case 2:
            FwState = "FLASH_INIT";
            break;
        case 3:
            FwState = "CALIB";
            break;
        case 4:
            FwState = "IDLE";
            break;
        case 5:
            FwState = "CONNECTING...";
            break;
        case 6:
            FwState = "ASSOCIATED";
            break;
        case 7:
            FwState = "DISCONNECTING...";
            break;
        default:
            FwState = to_string(FwStatehexVal);
        }
        DWORD BF_MCShexVal;
        device->GetDriver()->Read( 0x880A60, BF_MCShexVal);
        string BF_MCS = to_string(BF_MCShexVal);

        DWORD UcRxonhexVal;
        device->GetDriver()->Read(0x9405BE, UcRxonhexVal);
        DWORD UcRxonhexVal16 = UcRxonhexVal << 16;
        string UcRxon;
        switch (UcRxonhexVal16)
        {
        case 0:
            UcRxon = "RX_OFF";
            break;
        case 1:
            UcRxon = "RX_ONLY";
            break;
        case 2:
            UcRxon = "RX_ON";
            break;
        default:
            UcRxon = to_string(UcRxonhexVal16);
        }

        DWORD TX_GPhexVal;
        device->GetDriver()->Read( 0x880A58, TX_GPhexVal);
        string TX_GP = (TX_GPhexVal == 0) ? "NO_LINK" : to_string(TX_GPhexVal);

        DWORD RX_GPhexVal;
        device->GetDriver()->Read( 0x880A5C, RX_GPhexVal);
        string RX_GP = (RX_GPhexVal == 0) ? "NO_LINK" : to_string(RX_GPhexVal);

        DWORD BD_SEQhexVal;
        device->GetDriver()->Read(0x941374, BD_SEQhexVal);
        string BD_SEQ = to_string(BD_SEQhexVal);

        DWORD BF_TRIGhexVal;
        device->GetDriver()->Read(0x941380, BF_TRIGhexVal);
        string BF_TRIG = "";
        switch (BF_TRIGhexVal)
        {
        case 1:
            BF_TRIG = "MCS1_TH_FAILURE";
            break;
        case 2:
            BF_TRIG = "MCS1_NO_BACK";
            break;
        case 4:
            BF_TRIG = "NO_CTS_IN_TXOP";
            break;
        case 8:
            BF_TRIG = "MAX_BCK_FAIL_TXOP";
            break;
        case 16:
            BF_TRIG = "FW_TRIGGER ";
            break;
        case 32:
            BF_TRIG = "MAX_BCK_FAIL_ION_KEEP_ALIVE";
            break;
        default:
            BF_TRIG = to_string(BF_TRIGhexVal);
        }

        DWORD CHANNELhexVal;
        device->GetDriver()->Read(0x883020, CHANNELhexVal);
        string CHANNEL = "CHANNEL";
        switch (CHANNELhexVal)
        {
        case 0x64FCACE:
            CHANNEL = "CHANNEL_1";
            break;
        case 0x68BA2E9:
            CHANNEL = "CHANNEL_2";
            break;
        case 0x6C77B03:
            CHANNEL = "CHANNEL_3";
            break;
        default:
            CHANNEL = to_string(CHANNELhexVal);
        }

        DWORD NAVhexVal;
        device->GetDriver()->Read( 0x940490, NAVhexVal);
        string NAV = to_string(NAVhexVal);

        stringstream s;

        s << "status bar is:" \
            << " || Fw_Version:" << FwMaj << "." << FwMin << "." << FwBuild \
            << " || RfStat:" << RfStat \
            << " || FwError:" << FwError \
            << " || UcError:" << UcError \
            << " || FwState:" << FwState \
            << " || BF_MCS:" << BF_MCS \
            << " || UC_RXON:" << UcRxon \
            << " || TX_GP:" << TX_GP \
            << " || RX_GP:" << RX_GP \
            << " || BD_SEQ:" << BD_SEQ \
            << " || BF_TRIG:" << BF_TRIG \
            << " || CHANNEL:" << CHANNEL \
            << " || NAV:" << NAV << flush;

        std::string formatedStatusBar = s.str();
        return formatedStatusBar;
    }
    catch (exception e) {
        cout << e.what() << endl;
        return "";
    }*/
}


/*

FwMaj =
FwMin =
FwBuild
RfStat =
FwError
UcError
FwState;
BF_MCS =
UcRxon;
TX_GP =
RX_GP =
BD_SEQ =
BF_TRIG
CHANNEL
NAV = to

*/

DeviceManagerOperationStatus DeviceManager::SetDeviceSilentMode(string deviceName, bool silentMode)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        lock_guard<mutex> lock(m_devices[deviceName]->m_mutex);
        m_connectedDevicesMutex.unlock();
        m_devices[deviceName]->SetSilenceMode(silentMode);
        status = dmosSuccess;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        status = dmosNoSuchConnectedDevice;
    }

    return status;
}


DeviceManagerOperationStatus DeviceManager::GetDeviceSilentMode(string deviceName, bool& silentMode)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        lock_guard<mutex> lock(m_devices[deviceName]->m_mutex);
        m_connectedDevicesMutex.unlock();
        silentMode = m_devices[deviceName]->GetSilenceMode();
        status = dmosSuccess;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        status = dmosNoSuchConnectedDevice;
    }

    return status;
}

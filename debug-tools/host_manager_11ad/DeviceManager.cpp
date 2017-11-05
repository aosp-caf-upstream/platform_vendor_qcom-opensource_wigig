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
#include <sstream>

#include "DeviceManager.h"
#include "Utils.h"
#include "AccessLayerAPI.h"
#include "DebugLogger.h"
#include "Utils.h"
#include "Host.h"

using namespace std;

// Initialize translation maps for the front-end data
const static std::string sc_noRfStr("NO_RF");
const std::unordered_map<int, std::string> DeviceManager::m_rfTypeToString = { {0, sc_noRfStr }, {1, "MARLON"}, {2, "SPR-R"}, {3, "TLN-A1"}, { 4, "TLN-A2" } };

const std::unordered_map<int, std::string> DeviceManager::m_basebandTypeToString =
{
    { 0, "UNKNOWN" }, { 1, "MAR-DB1" }, { 2, "MAR-DB2" },
    { 3, "SPR-A0"  }, { 4, "SPR-A1"  }, { 5, "SPR-B0"  },
    { 6, "SPR-C0"  }, { 7, "SPR-D0"  }, { 8, "TLN-M-A0"  },
    { 9, "TLN-M-B0" }
};

const std::unordered_map<int, std::string> DeviceManager::m_boardfileTypeToString =
{
    { 0, "UNDEFINED" },
    { 1, "generic_single_ant" },
    { 2, "generic_SIP" },
    { 3, "generic_reduced_size" },
    { 4, "generic_patches_only" },
    { 5, "generic_500mW" },
    { 6, "generic_500mW_removed_RF" },
    { 16, "ROGERS" },
    { 32, "GENERIC_LTCC" },
    { 48, "REGULATORY_LTCC" },
    { 64, "FeiDao_6430u" },
    { 80, "GENERIC_FALCON" },
    { 112, "DELL_E7440_non-touch" },
    { 113, "DELL_E7240_non" },
    { 114, "DELL_E7440_touch" },
    { 115, "DELL_E7240_touch" },
    { 128, "Corse" },
    { 144, "DELL_E5440" },
    { 160, "MURATA_DELL_D5000" },
    { 176, "DELL_XPS13" },
    { 192, "TOSHIBA_Z10" },
    { 208, "Semco_Sip" },
    { 209, "Semco_Sip_rev1_3" },
    { 224, "MTP-BStep" },
    { 225, "MTP-CStep" },
    { 240, "Liquid" },
    { 257, "Murata SI" }
};

DeviceManager::DeviceManager(std::promise<void>& eventsTCPServerReadyPromise) :
    m_deviceManagerRestDurationMs(500),
    m_terminate(false),
    m_collectLogs(false)
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

std::shared_ptr<Device> DeviceManager::GetDeviceByName(const std::string& deviceName)
{
    m_connectedDevicesMutex.lock();
    for (auto& device : m_devices)
    {
        if (deviceName == device.first)
        {
            m_connectedDevicesMutex.unlock();
            return device.second;
        }
    }
    m_connectedDevicesMutex.unlock();
    return nullptr;
}

DeviceManagerOperationStatus DeviceManager::OpenInterface(const string& deviceName)
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

DeviceManagerOperationStatus DeviceManager::CloseInterface(const string& deviceName)
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

DeviceManagerOperationStatus DeviceManager::Read(const string& deviceName, DWORD address, DWORD& value)
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

DeviceManagerOperationStatus DeviceManager::Write(const string& deviceName, DWORD address, DWORD value)
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

DeviceManagerOperationStatus DeviceManager::ReadBlock(const string& deviceName, DWORD address, DWORD blockSize, vector<DWORD>& values)
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

DeviceManagerOperationStatus DeviceManager::WriteBlock(const string& deviceName, DWORD address, const vector<DWORD>& values)
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

DeviceManagerOperationStatus DeviceManager::InterfaceReset(const string& deviceName)
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

DeviceManagerOperationStatus DeviceManager::SwReset(const string& deviceName)
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

DeviceManagerOperationStatus DeviceManager::SetDriverMode(const string& deviceName, int newMode, int& oldMode)
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

DeviceManagerOperationStatus DeviceManager::DriverControl(const string& deviceName, uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize)
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

DeviceManagerOperationStatus DeviceManager::AllocPmc(const string& deviceName, unsigned descSize, unsigned descNum, string& errorMsg)
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

DeviceManagerOperationStatus DeviceManager::DeallocPmc(const string& deviceName, std::string& outMessage)
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

DeviceManagerOperationStatus DeviceManager::CreatePmcFile(const string& deviceName, unsigned refNumber, std::string& outMessage)
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

DeviceManagerOperationStatus DeviceManager::FindPmcFile(const string& deviceName, unsigned refNumber, std::string& outMessage)
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

DeviceManagerOperationStatus DeviceManager::SendWmi(const string& deviceName, DWORD command, const vector<DWORD>& payload)
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

void DeviceManager::CreateDevice(const string& deviceName)
{
    m_connectedDevicesMutex.lock();
    shared_ptr<Device> device(new Device(deviceName));
    device->Init();
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

void DeviceManager::DeleteDevice(const string& deviceName)
{
    m_connectedDevicesMutex.lock();
    // make sure that no client is using this object
    m_devices[deviceName]->m_mutex.lock();
    m_devices[deviceName]->Fini();
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

        connectedDevice.second->m_mutex.lock();
        if (!connectedDevice.second->GetDriver()->IsValid())
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
        }
        this_thread::sleep_for(std::chrono::milliseconds(m_deviceManagerRestDurationMs));
    }
}

bool DeviceManager::IsDeviceSilent(const string& deviceName)
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

bool DeviceManager::GetDeviceStatus(vector<DeviceData>& devicesData)
{
    // Lock the devices
    lock_guard<mutex> lock(m_connectedDevicesMutex);

    auto devices = m_devices;

    for (auto& device : devices)
    {
        // Lock the specific device
        lock_guard<mutex> lock(device.second->m_mutex);

        // Create device data
        DeviceData deviceData;

        // Extract FW version
        deviceData.m_fwVersion = device.second->GetFwVersion();

        DWORD value = Utils::REGISTER_DEFAULT_VALUE;

        // Read FW assert code
        device.second->GetDriver()->Read(FW_ASSERT_REG, value);
        deviceData.m_fwAssert = value;

        // Read uCode assert code
        device.second->GetDriver()->Read(UCODE_ASSERT_REG, value);
        deviceData.m_uCodeAssert = value;

        // Read FW association state
        device.second->GetDriver()->Read(FW_ASSOCIATION_REG, value);
        deviceData.m_associated = (value == FW_ASSOCIATED_VALUE);

        // Read MCS value
        device.second->GetDriver()->Read(MCS_REG, value);
        deviceData.m_mcs = value;

        // Get FW compilation timestamp
        deviceData.m_compilationTime = device.second->GetFwTimestamp();

        // Get Device name
        deviceData.m_deviceName = device.second->GetDeviceName();

        // Get baseband name & RF type
        // BB type is stored in 2 lower bytes of device type register
        // RF type is stored in 2 upper bytes of device type register
        device.second->GetDriver()->Read(DEVICE_TYPE_REG, value);
        const auto basebandTypeIter = m_basebandTypeToString.find(value & 0xFFFF);
        deviceData.m_hwType = basebandTypeIter != m_basebandTypeToString.cend() ? basebandTypeIter->second : std::string("UNKNOWN");
        const auto rfTypeIter = m_rfTypeToString.find((value & 0xFFFF0000) >> 16);
        deviceData.m_rfType = rfTypeIter != m_rfTypeToString.cend() ? rfTypeIter->second : sc_noRfStr;

        // Get FW mode
        device.second->GetDriver()->Read(FW_MODE_REG, value);
        deviceData.m_mode = (value == 0) ? "Operational" : "WMI Only";

        // Get boot loader version
        device.second->GetDriver()->Read(BOOT_LOADER_VERSION_REG, value);
        std::ostringstream oss;
        oss << value;
        deviceData.m_bootloaderVersion = oss.str();

        // Get channel number
        device.second->GetDriver()->Read(CHANNEL_REG, value);
        int Channel = 0;
        switch (value)
        {
        case 0x64FCACE:
            Channel = 1;
            break;
        case 0x68BA2E9:
            Channel = 2;
            break;
        case 0x6C77B03:
            Channel = 3;
            break;
        default:
            Channel = 0;
        }
        deviceData.m_channel = Channel;

        // Get board file version
        device.second->GetDriver()->Read(BOARDFILE_REG, value);
        const auto boardfileTypeIter = m_boardfileTypeToString.find((value & 0xFFF000) >> 12);
        deviceData.m_boardFile = boardfileTypeIter != m_boardfileTypeToString.cend() ? boardfileTypeIter->second : std::string("UNDEFINED");

        DWORD rfConnected;
        DWORD rfEnabled;
        device.second->GetDriver()->Read(RF_CONNECTED_REG, rfConnected);
        device.second->GetDriver()->Read(RF_ENABLED_REG, rfEnabled);
        rfEnabled = rfEnabled >> 8;

        // Get RF state of each RF
        for (int rfIndex = 0; rfIndex < MAX_RF_NUMBER; rfIndex++)
        {
            int rfState = 0;

            if (rfConnected & (1 << rfIndex))
            {
                rfState = 1;
            }

            if (rfEnabled & (1 << rfIndex))
            {
                rfState = 2;
            }

            // TODO extract RF state for each RF
            deviceData.m_rf.insert(deviceData.m_rf.end(), rfState);
        }

        ////////// Get fixed registers values //////////////////////////
        RegisterData registerData;

        // uCode Rx on fixed reg
        device.second->GetDriver()->Read(UCODE_RX_ON_REG, value);
        DWORD UcRxonhexVal16 = value && 0xFFFF;
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
            UcRxon = "Unrecognized";
        }
        registerData.m_name = "uCodeRxOn";
        registerData.m_value = UcRxon;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // BF Sequence fixed reg
        device.second->GetDriver()->Read(BF_SEQ_REG, value);
        oss.str("");
        oss << value;
        registerData.m_name = "BF_Seq";
        registerData.m_value = oss.str();
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // BF Trigger fixed reg
        device.second->GetDriver()->Read(BF_TRIG_REG, value);
        string BF_TRIG = "";
        switch (value)
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
            BF_TRIG = "UNDEFINED";
        }
        registerData.m_name = "BF_Trig";
        registerData.m_value = BF_TRIG;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // Get NAV fixed reg
        device.second->GetDriver()->Read(NAV_REG, value);
        registerData.m_name = "NAV";
        oss.str("");
        oss << value;
        registerData.m_value = oss.str();
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // Get TX Goodput fixed reg
        device.second->GetDriver()->Read(TX_GP_REG, value);
        string TX_GP = "NO_LINK";
        if (value != 0)
        {
            oss.str("");
            oss << value;
            TX_GP = oss.str();
        }
        registerData.m_name = "TX_GP";
        registerData.m_value = TX_GP;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        // Get RX Goodput fixed reg
        device.second->GetDriver()->Read(RX_GP_REG, value);
        string RX_GP = "NO_LINK";
        if (value != 0)
        {
            oss.str("");
            oss << value;
            TX_GP = oss.str();
        }
        registerData.m_name = "RX_GP";
        registerData.m_value = RX_GP;
        deviceData.m_fixedRegisters.insert(deviceData.m_fixedRegisters.end(), registerData);

        ////////////// Fixed registers end /////////////////////////

        ////////////// Custom registers ////////////////////////////
        for (auto& reg : device.second->GetCustomRegisters())
        {
            registerData.m_name = reg.first;
            device.second->GetDriver()->Read(reg.second, value);
            oss.str("");
            oss << value;
            registerData.m_value = oss.str();
            deviceData.m_customRegisters.insert(deviceData.m_customRegisters.end(), registerData);
        }

        ////////////// Custom registers end ////////////////////////

        ////////////// Temperatures ////////////////////////////////
        // Baseband
        device.second->GetDriver()->Read(BASEBAND_TEMPERATURE_REG, value);
        float temperature = (float)value / 1000;
        oss.str("");
        oss.precision(2);
        oss << fixed << temperature;
        deviceData.m_hwTemp = oss.str();

        // RF
        if (deviceData.m_rfType != sc_noRfStr)
        {
            device.second->GetDriver()->Read(RF_TEMPERATURE_REG, value);
            temperature = (float)value / 1000;
            oss.str("");
            oss.precision(2);
            oss << fixed << temperature;
            deviceData.m_rfTemp = oss.str();
        }
        else // no RF, temperature value is not relevant
        {
            deviceData.m_rfTemp = "";
        }
        ////////////// Temperatures end ///////////////////////////

        // Add the device to the devices list
        devicesData.insert(devicesData.end(), deviceData);
    }

    return true;
}

bool DeviceManager::AddRegister(const string& deviceName, const string& registerName, int address)
{
    lock_guard<mutex> lock(m_connectedDevicesMutex);

    if (m_devices.count(deviceName) <= 0)
    {
        return false;
    }

    if ((NULL == m_devices[deviceName].get()) || (!m_devices[deviceName]->AddCustomRegister(registerName, address)))
    {
        LOG_ERROR << "Trying to add an already existing custom register name" << endl;
        return false;
    }

    return true;
}

bool DeviceManager::RemoveRegister(const string& deviceName, const string& registerName)
{
    lock_guard<mutex> lock(m_connectedDevicesMutex);

    if (m_devices.count(deviceName) <= 0)
    {
        return false;
    }

    if ((NULL == m_devices[deviceName].get()) || (!m_devices[deviceName]->RemoveCustomRegister(registerName)))
    {
        LOG_ERROR << "Trying to remove a non-existing custom register name" << endl;
        return false;
    }

    return true;
}

DeviceManagerOperationStatus DeviceManager::SetDeviceSilentMode(const string& deviceName, bool silentMode)
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


DeviceManagerOperationStatus DeviceManager::GetDeviceSilentMode(const string& deviceName, bool& silentMode)
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


// Log functions
bool DeviceManager::GetLogCollectionMode() const
{
    return m_collectLogs;
}

void DeviceManager::SetLogCollectionMode(bool collectLogs)
{
    m_collectLogs = collectLogs;

    // Start/Stop log collector for all devices
    m_connectedDevicesMutex.lock();
    for (auto& connectedDevice : m_devices)
    {
        connectedDevice.second->m_mutex.lock();

        if (collectLogs == false)
        {
            connectedDevice.second->StopLogCollector();
        }
        else
        {
            connectedDevice.second->StartLogCollector();
        }

        connectedDevice.second->m_mutex.unlock();
    }
    m_connectedDevicesMutex.unlock();
}

bool DeviceManager::SetLogCollectionConfiguration(const vector<string>& deviceNames, const vector<string>& cpuTypeNames, const string& parameter, const string& value, string& errorMessage)
{
    bool success;
    errorMessage = "";
    stringstream errorMessageSs;
    for (auto& deviceName : deviceNames)
    {
        shared_ptr<Device> d = GetDeviceByName(deviceName);
        if (nullptr == d)
        {
            success = false;
            errorMessageSs << "device name " << deviceName << " doesn't exist; ";
            continue;
        }

        for (auto& cpuTypeName : cpuTypeNames)
        {
            auto found = STRING_TO_CPU_TYPE.find(cpuTypeName);
            if (STRING_TO_CPU_TYPE.end() == found)
            {
                success = false;
                errorMessageSs << "no such cpu named " << cpuTypeName << ". Can be only FW/UCODE; ";
                continue;
            }
            log_collector::LogCollector* pLogCollector = d->GetLogCollector(found->second);
            if (nullptr == pLogCollector)
            {
                success = false;
                errorMessageSs << "device " << deviceName << " has no active tracer for " << cpuTypeName << "; ";
                continue;
            }
            pLogCollector->SetConfigurationParamerter(parameter, value);
        }
    }
    errorMessageSs << endl;
    errorMessage = errorMessageSs.str();
    return success;
}

string DeviceManager::GetLogCollectionConfiguration(const vector<string>& deviceNames, const vector<string>& cpuTypeNames, string parameter)
{
    stringstream res;
    bool success;
    for (auto& deviceName : deviceNames)
    {
        shared_ptr<Device> d = GetDeviceByName(deviceName);
        if (nullptr == d)
        {
            res << "device name " << deviceName << " doesn't exist; ";
            continue;
        }

        for (auto& cpuTypeName : cpuTypeNames)
        {
            auto found = STRING_TO_CPU_TYPE.find(cpuTypeName);
            if (STRING_TO_CPU_TYPE.end() == found)
            {
                res << "no such cpu named " << cpuTypeName << ". Can be only FW/UCODE; ";
                continue;
            }
            log_collector::LogCollector* pLogCollector = d->GetLogCollector(found->second);
            if (nullptr == pLogCollector)
            {
                res << "device " << deviceName << " has no active tracer for " << cpuTypeName << "; ";
                continue;
            }

            res << "device-" << deviceName << "-cpu-" << cpuTypeName << "-parameter-" << parameter << "=" << pLogCollector->GetConfigurationParameterValue(parameter, success) << ";"; // TODO - create constants for "=" and ";"
        }
    }
    res << endl;
    return res.str();
}

string DeviceManager::DumpLogCollectionConfiguration(const vector<string>& deviceNames, const vector<string>& cpuTypeNames)
{
    stringstream res;
    for (auto& deviceName : deviceNames)
    {
        shared_ptr<Device> d = GetDeviceByName(deviceName);
        if (nullptr == d)
        {
            res << "device name " << deviceName << " doesn't exist; ";
            continue;
        }

        for (auto& cpuTypeName : cpuTypeNames)
        {
            auto found = STRING_TO_CPU_TYPE.find(cpuTypeName);
            if (STRING_TO_CPU_TYPE.end() == found)
            {
                res << "no such cpu named " << cpuTypeName << ". Can be only FW/UCODE; ";
                continue;
            }
            log_collector::LogCollector* pLogCollector = d->GetLogCollector(found->second);
            if (nullptr == pLogCollector)
            {
                res << "device " << deviceName << " has no active tracer for " << cpuTypeName << "; ";
                continue;
            }

            res << "device-" << deviceName << "-cpu-" << cpuTypeName << ":" << pLogCollector->GetConfigurationDump() << ";";  // TODO - create constants for "=" and ";"
        }
    }
    res << endl;
    return res.str();
}

DeviceManagerOperationStatus DeviceManager::GetDeviceCapabilities(const string& deviceName, DWORD& capabilities)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_devices.count(deviceName) > 0)
    {
        lock_guard<mutex> lock(m_devices[deviceName]->m_mutex);
        m_connectedDevicesMutex.unlock();
        capabilities = m_devices[deviceName]->GetCapabilities();
        status = dmosSuccess;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        status = dmosNoSuchConnectedDevice;
    }

    return status;
}

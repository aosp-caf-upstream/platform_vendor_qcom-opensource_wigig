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
#include <future>

#include "DeviceManager.h"
#include "Utils.h"
#include "AccessLayerAPI.h"

DeviceManager::DeviceManager() :
    m_deviceManagerRestDurationMs(500),
    m_terminate(false)
{
    m_deviceManager = thread(&DeviceManager::PeriodicTasks, this);
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
    default:
        return "DeviceManagerOperationStatus is unknown ";
    }
}

DeviceManagerOperationStatus DeviceManager::GetDevices(set<string>& devicesNames)
{
    devicesNames.clear();
    for (auto& device : m_connectedDevices)
    {
        devicesNames.insert(device.first);
    }
    return dmosSuccess;
}

DeviceManagerOperationStatus DeviceManager::OpenInterface(string deviceName)
{
    if (m_connectedDevices.count(deviceName) > 0)
    {
        return dmosSuccess;
    }
    return dmosNoSuchConnectedDevice;
}

DeviceManagerOperationStatus DeviceManager::CloseInterface(string deviceName)
{
    if (m_connectedDevices.count(deviceName) > 0)
    {
        return dmosSuccess;
    }
    return dmosNoSuchConnectedDevice;
}

DeviceManagerOperationStatus DeviceManager::Read(string deviceName, DWORD address, DWORD& value)
{
	if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
	{
		return dmosInvalidAddress;
	}

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->Read(address, value);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            value = Device::GetRegisterDefaultValue();
            status = dmosFailedToReadFromDevice;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        value = Device::GetRegisterDefaultValue();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::Write(string deviceName, DWORD address, DWORD value)
{
	if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
	{
		return dmosInvalidAddress;
	}

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->Write(address, value);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToWriteToDevice;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
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
	if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
	{
		return dmosInvalidAddress;
	}

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->ReadBlock(address, blockSize, values);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            vector<DWORD> defaultValues(blockSize, Device::GetRegisterDefaultValue());
            status = dmosFailedToReadFromDevice;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        vector<DWORD> defaultValues(blockSize, Device::GetRegisterDefaultValue());
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::WriteBlock(string deviceName, DWORD address, const vector<DWORD>& values)
{
	if ((0 == address) || (0 != address % 4) || (0xFFFFFFFF == address))
	{
		return dmosInvalidAddress;
	}

    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->WriteBlock(address, values);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToWriteToDevice;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
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
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        m_connectedDevices[deviceName]->m_device->InterfaceReset();
        status = dmosSuccess;
        m_connectedDevices[deviceName]->m_mutex.unlock();
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
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->SwReset();
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToResetSw;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
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
	if (m_connectedDevices.count(deviceName) > 0)
	{
		m_connectedDevices[deviceName]->m_mutex.lock();
		m_connectedDevicesMutex.unlock();
		bool success = m_connectedDevices[deviceName]->m_device->SetDriverMode(newMode, oldMode);
		if (success)
		{
			status = dmosSuccess;
		}
		else
		{
			status = dmosFailedToResetSw;
		}
		m_connectedDevices[deviceName]->m_mutex.unlock();
		return status;
	}
	else
	{
		m_connectedDevicesMutex.unlock();
		return dmosNoSuchConnectedDevice;
	}
}

DeviceManagerOperationStatus DeviceManager::AllocPmc(string deviceName, unsigned descSize, unsigned descNum)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->AllocPmc(descSize, descNum);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToAllocatePmc;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::DeallocPmc(string deviceName)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->DeallocPmc();
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToDeallocatePmc;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
        return status;
    }
    else
    {
        m_connectedDevicesMutex.unlock();
        return dmosNoSuchConnectedDevice;
    }
}

DeviceManagerOperationStatus DeviceManager::CreatePmcFile(string deviceName, unsigned refNumber)
{
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->CreatePmcFile(refNumber);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToCreatePmcFile;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
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
    DeviceManagerOperationStatus status;
    m_connectedDevicesMutex.lock();
    if (m_connectedDevices.count(deviceName) > 0)
    {
        m_connectedDevices[deviceName]->m_mutex.lock();
        m_connectedDevicesMutex.unlock();
        bool success = m_connectedDevices[deviceName]->m_device->SendWmi(command, payload);
        if (success)
        {
            status = dmosSuccess;
        }
        else
        {
            status = dmosFailedToSendWmi;
        }
        m_connectedDevices[deviceName]->m_mutex.unlock();
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
    shared_ptr<ConnectedDevice> connectedDevice(new ConnectedDevice(AccessLayer::OpenDevice(deviceName)));
    m_connectedDevices.insert(make_pair(deviceName, connectedDevice));
    m_connectedDevicesMutex.unlock();
}

void DeviceManager::DeleteDevice(string deviceName)
{
    m_connectedDevicesMutex.lock();
    // make sure that no client is using this object
    m_connectedDevices[deviceName]->m_mutex.lock();
    // no need that the mutex will be still locked since new clients have to get m_connectedDevicesMutex before they try to get m_mutex
    m_connectedDevices[deviceName]->m_mutex.unlock();
    AccessLayer::CloseDevice(deviceName);
    m_connectedDevices.erase(deviceName);
    m_connectedDevicesMutex.unlock();
}

void DeviceManager::UpdateConnectedDevices()
{
    set<string> currentlyConnectedDevices = AccessLayer::GetDevices();

    // delete devices that arn't connected anymore
    vector<string> devicesForRemove;
    for (auto& connectedDevice : m_connectedDevices)
    {
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
    for (auto& currentlyConnectedDevice : currentlyConnectedDevices)
    {
        if (0 == m_connectedDevices.count(currentlyConnectedDevice))
        {
            newDevices.push_back(currentlyConnectedDevice);
        }
    }
    for (auto& device : newDevices)
    {
        CreateDevice(device);
    }
}

void DeviceManager::PeriodicTasks()
{
    while (!m_terminate)
    {
        UpdateConnectedDevices();
        for (auto& connectedDevice : m_connectedDevices)
        {
            connectedDevice.second->m_mutex.lock();
            connectedDevice.second->m_device->Poll();
            connectedDevice.second->m_mutex.unlock();
        }
        this_thread::sleep_for(std::chrono::milliseconds(m_deviceManagerRestDurationMs));
    }
}
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

#include "OperatingSystemConstants.h"
#include "../DebugLogger.h"
#ifdef _WINDOWS
#include "WindowsDriverAPI.h"
#else
#include "UnixDriverAPI.h"
#endif

#include "PciDevice.h"


//////////////////////////////////////////////////////////////////////////
// PCI interface

PciDevice::PciDevice(string deviceName, string interfaceName) : Device(deviceName, interfaceName)
{
    //LOG_MESSAGE_INFO(_T("Create PCI device access for: %s"), interfaceName.c_str());
    m_initialized = false;

#ifdef _WINDOWS
    m_pDriverApi.reset(new WindowsDriverAPI(interfaceName));
#else
    m_pDriverApi.reset(new UnixDriverAPI(interfaceName));
#endif

    if (m_pDriverApi->Open())
    {
        m_initialized = true;
    }
    else
    {
        m_initialized = false;
    }
}
PciDevice::~PciDevice()
{
    Close();
}

// Virtual access functions for device
bool PciDevice::Read(DWORD address, DWORD& value)
{
    return (m_pDriverApi->Read(address, value));
}
bool PciDevice::ReadBlock(DWORD address, DWORD blockSize, vector<DWORD>& values)
{
    bool success = false;

    DWORD* arrBlock = NULL;
    try
    {
        arrBlock = new DWORD[blockSize];

        // blockSize is in bytes, need to be multiplied by 4 for DWORDS
        success = (m_pDriverApi->ReadBlock(address, blockSize * 4, (char*)arrBlock));

        values = std::vector<DWORD>(arrBlock, arrBlock + (blockSize));

        LOG_DEBUG << "Read: " << values.size() << " Values \n";
    }
    catch (...)
    {
        LOG_ERROR << "Exception when trying to read block\n";
        delete[] arrBlock;
        return false;
    }

    delete[] arrBlock;
    return success;
}
bool PciDevice::Write(DWORD address, DWORD value)
{
    return (m_pDriverApi->Write(address, value));
}
bool PciDevice::WriteBlock(DWORD address, vector<DWORD> values)
{
    char* valuesToWrite = (char*)&values[0];

    return (m_pDriverApi->WriteBlock(address, values.size() * 4, valuesToWrite));
}

bool PciDevice::AllocPmc(unsigned descSize, unsigned descNum, std::string& outMessage)
{
    return m_pDriverApi->AllocPmc(descSize, descNum, outMessage);
}

bool PciDevice::DeallocPmc(std::string& outMessage)
{
    return m_pDriverApi->DeallocPmc(outMessage);
}

bool PciDevice::CreatePmcFile(unsigned refNumber, std::string& outMessage)
{
    return m_pDriverApi->CreatePmcFile(refNumber, outMessage);
}

bool PciDevice::FindPmcFile(unsigned refNumber, std::string& outMessage)
{
    return m_pDriverApi->FindPmcFile(refNumber, outMessage);
}

bool PciDevice::Open()
{
    return m_pDriverApi->Open();
}
void PciDevice::Close()
{
    return m_pDriverApi->Close();
}
bool PciDevice::ReOpen()
{
    return m_pDriverApi->ReOpen();
}

bool PciDevice::SetDriverMode(int newState, int &oldState)
{
    return m_pDriverApi->SetDriverMode(newState, oldState);
}
int PciDevice::GetDriverMode(int &currentState)
{
    return m_pDriverApi->GetDriverMode(currentState);
}

bool PciDevice::DriverControl(uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize)
{
	return (m_pDriverApi->DriverControl(Id, inBuf, inBufSize, outBuf, outBufSize));
}


static string NameDevice(string interfaceName)
{
    return std::string("PCI") + DEVICE_NAME_DELIMITER + interfaceName;
}

void PciDevice::InterfaceReset()
{
    m_pDriverApi->Reset();
    return;
}

set<string> PciDevice::Enumerate()
{
    // Hold all discovered hosts
    set<string> discoveredDevices;

#ifdef _WINDOWS
    set<string> enumeratedDevices = WindowsDriverAPI::Enumerate();
#else
    set<string> enumeratedDevices = UnixDriverAPI::Enumerate();
#endif

    set<string>::iterator it;
    for (it = enumeratedDevices.begin(); it != enumeratedDevices.end(); ++it)
    {
        string interfaceName = *it;

        PciDevice device("Enum", interfaceName);

        if (device.IsOpen())
        {
            discoveredDevices.insert(NameDevice(interfaceName));
            device.Close();
        }
    }

    return discoveredDevices;
}

bool PciDevice::try2open(int timeout)
{
    do
    {
        Open();
        if (m_pDriverApi->IsOpened())
            break;
        sleep_ms(100);
        timeout -= 100;
    } while (timeout > 0);

    if (!m_pDriverApi->IsOpened())
        return false;
    return true;
}
void PciDevice::rr32(DWORD addr, DWORD num_repeat, DWORD *arrBlock)
{
    //do something with params
    (void)addr;
    (void)num_repeat;
    (void)arrBlock;
    WLCT_ASSERT(0);
}

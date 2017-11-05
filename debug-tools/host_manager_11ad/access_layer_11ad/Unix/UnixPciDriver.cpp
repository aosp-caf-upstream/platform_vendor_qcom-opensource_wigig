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

#ifndef _WINDOWS

#include "DebugLogger.h"
#include "UnixPciDriver.h"
#include "PmcCfg.h"
#include "PmcData.h"

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include <net/if.h> // for struct ifreq
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h> // for struct ifreq

#include <sys/types.h> // for opendir
#include <dirent.h> // for opendir

static const string PCI_END_POINT_FOLDER = "/sys/module/wil6210/drivers/pci:wil6210/";
static const char* INVALID = "Invalid";

using namespace std;

UnixPciDriver::~UnixPciDriver()
{
    Close();
}

// Base access functions (to be implemented by specific device)
bool UnixPciDriver::Read(DWORD address, DWORD& value)
{
    IoctlIO io;

    io.addr = address;
    io.val = 0;
    io.op = EP_OPERATION_READ;
    if (!SendRWIoctl(io, m_fileDescriptor, m_interfaceName.c_str()))
    {
        return false;
    }

    value = io.val;

    return true;
}

bool UnixPciDriver::ReadBlock(DWORD addr, DWORD blockSize, char *arrBlock)
{
    IoctlIOBlock io;
    io.op = EP_OPERATION_READ;
    io.addr = addr;
    io.size = blockSize;
    io.buf = arrBlock;

    return SendRWBIoctl(io, m_fileDescriptor, m_interfaceName.c_str());
}

bool UnixPciDriver::ReadBlock(DWORD address, DWORD blockSize, vector<DWORD>& values)
{
    bool success = false;

    DWORD* arrBlock = NULL;
    try
    {
        arrBlock = new DWORD[blockSize];

        // blockSize is in bytes, need to be multiplied by 4 for DWORDS
        success = ReadBlock(address, blockSize * 4, (char*)arrBlock);

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

bool UnixPciDriver::Write(DWORD address, DWORD value)
{
    IoctlIO io;

    io.addr = address;
    io.val = value;
    io.op = EP_OPERATION_WRITE;

    if (!SendRWIoctl(io, m_fileDescriptor, m_interfaceName.c_str()))
    {
        return false;
    }

    return true;
}

bool UnixPciDriver::WriteBlock(DWORD address, DWORD blockSize, const char *valuesToWrite)
{
    IoctlIOBlock io;
    io.op = EP_OPERATION_WRITE;
    io.addr = address;
    io.size = blockSize;
    io.buf = (void*)valuesToWrite;

    return SendRWBIoctl(io, m_fileDescriptor, m_interfaceName.c_str());
}

bool UnixPciDriver::WriteBlock(DWORD addr, vector<DWORD> values)
{
    char* valuesToWrite = (char*)&values[0];
    DWORD sizeToWrite = values.size() * 4;

    return WriteBlock(addr, sizeToWrite, valuesToWrite);
}

string GetInterfaceNameFromEP(string pciEndPoint)
{
    // const definitions
    static const char* CURRENT_DIRECTORY = ".";
    static const char* PARENT_DIRECTORY = "..";

    stringstream interfaceNameContaingFolder; // path of a folder which contains the interface name of the specific EP
    interfaceNameContaingFolder << PCI_END_POINT_FOLDER << pciEndPoint << "/net";
    DIR* dp = opendir(interfaceNameContaingFolder.str().c_str());
    if (!dp)
    {
        return INVALID;
    }

    dirent* de; // read interface name. We assume there is only one folder, if not we take the first one
    do
    {
        de = readdir(dp);
    } while ((de != NULL) && ((strncmp(CURRENT_DIRECTORY, de->d_name, strlen(CURRENT_DIRECTORY)) == 0) || (strncmp(PARENT_DIRECTORY, de->d_name, strlen(PARENT_DIRECTORY)) == 0)));
    if (NULL == de)
    {
        closedir(dp);
        return INVALID;
    }

    closedir(dp);
    return de->d_name;
}

set<string> GetNetworkInterfaceNames()
{
    set<string> networkInterfaces;

    DIR* dp = opendir(PCI_END_POINT_FOLDER.c_str());
    if (!dp)
    {
        LOG_VERBOSE << "Failed to open PCI EP directories" << endl;
        return networkInterfaces;
    }
    dirent* de;
    while ((de = readdir(dp)) != NULL) // iterate through directory content and search for End Point folders
    {
        string potentialPciEndPoint(de->d_name);
        if (potentialPciEndPoint.find(":") != string::npos) // only PCI end point has ":" in its name
        {
            // Get interface name from End Point
            string networkInterfaceName = GetInterfaceNameFromEP(potentialPciEndPoint);
            if (networkInterfaceName != INVALID)
            {
                networkInterfaces.insert(networkInterfaceName);
            }
            else
            {
                LOG_VERBOSE << "Failed to find interface name for EP: " << potentialPciEndPoint << endl;
            }
        }
    }

    closedir(dp);
    return networkInterfaces;
}

set<string> UnixPciDriver::Enumerate()
{
    set<string> interfacesToCheck = GetNetworkInterfaceNames();
    set<string> connectedInterfaces;
    for (auto it = interfacesToCheck.begin(); it != interfacesToCheck.end(); ++it)
    {
        string interfaceName = *it;

        UnixPciDriver pciDriver(interfaceName);
        if (pciDriver.Open())
        {
            connectedInterfaces.insert(UnixPciDriver::NameDevice(interfaceName));
            pciDriver.Close();
        }
    }
    return connectedInterfaces;
}

bool UnixPciDriver::Open()
{
    if(m_initialized)
    {
        return true;
    }

    m_fileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (INVALID_FD == m_fileDescriptor || m_fileDescriptor < 0)
    {
        Close();
        return false;
    }

    // Validate interface is 11ad interface
    if(!ValidateInterface())
    {
        Close();
        return false;
    }

    if (!SetDebugFsPath())
    {
        LOG_VERBOSE << "Failed to find debug FS" << endl;
    }

    m_initialized = true;
    return true;
}

bool UnixPciDriver::SetDebugFsPath() // assuming m_interfaceName is set
{
    // const definitions
    static const char* PHY = "phy";

    // find phy number
    stringstream phyContaingFolder; // path of a folder which contains the phy of the specific interface
    phyContaingFolder << "/sys/class/net/" << m_interfaceName << "/device/ieee80211";
    DIR* dp = opendir(phyContaingFolder.str().c_str());
    if (!dp) // ieee80211 doesn't exist, meaning this isn't an 11ad interface
    {
        return false;
    }
    dirent* de;// read phy name (phy folder is named phyX where X is a digit). We assume there is only one folder, if not we take the first one
    do
    {
        de = readdir(dp);
    } while ((de != NULL) && (strncmp(PHY, de->d_name, strlen(PHY)) != 0));
    if (NULL == de)
    {
        closedir(dp);
        return false;
    }

    // find debug FS (using phy name)
    stringstream debugFsPath;
    debugFsPath << "/sys/kernel/debug/ieee80211/" << de->d_name << "/wil6210";
    if(-1 == access(debugFsPath.str().c_str(), F_OK)) // didn't find debug FS
    {
        closedir(dp);
        return false;
    }
    closedir(dp);

    // update debug FS path
    m_debugFsPath = debugFsPath.str();
    return true;
}

bool UnixPciDriver::ReOpen()
{
    return Open();
}

bool UnixPciDriver::AllocPmc(unsigned descSize, unsigned descNum, string& outMessage)
{
    PmcCfg pmcCfg(m_debugFsPath.c_str());
    OperationStatus st = pmcCfg.AllocateDmaDescriptors(descSize, descNum);

    outMessage = st.GetMessage();
    return st.IsSuccess();
}

bool UnixPciDriver::DeallocPmc(std::string& outMessage)
{
    PmcCfg pmcCfg(m_debugFsPath.c_str());
    OperationStatus st = pmcCfg.FreeDmaDescriptors();

    outMessage = st.GetMessage();
    return st.IsSuccess();
}

bool UnixPciDriver::CreatePmcFile(unsigned refNumber, std::string& outMessage)
{
    LOG_DEBUG << "Creating PMC data file #" << refNumber << std::endl;

    PmcDataFileWriter pmcFileWriter(refNumber, m_debugFsPath.c_str());

    OperationStatus st = pmcFileWriter.WriteFile();
    outMessage = st.GetMessage();

    if (!st.IsSuccess())
    {
        LOG_ERROR << "Error creating PMC data file for #" << refNumber << std::endl;
        return false;
    }

    LOG_DEBUG << "PMC data file created. Reported size: " << st.GetMessage() << std::endl;
    return true;
}

bool UnixPciDriver::FindPmcFile(unsigned refNumber, std::string& outMessage)
{
    LOG_DEBUG << "Looking for the PMC File #" << refNumber << endl;

    PmcDataFileLocator pmcDataFileLocator(refNumber);

    if (!pmcDataFileLocator.FileExists())
    {
        std::stringstream errorMsgBuilder;
        errorMsgBuilder << "Cannot find PMC file " << pmcDataFileLocator.GetFileName();
        outMessage = errorMsgBuilder.str();
        return false;
    }

    outMessage = pmcDataFileLocator.GetFileName();
    return true;
}

void UnixPciDriver::Close()
{
    if (m_fileDescriptor != INVALID_FD)
    {
        close(m_fileDescriptor);
        m_fileDescriptor = INVALID_FD;
    }
}

int UnixPciDriver::GetDriverMode(int &currentState)
{
    //do something with params
    (void)currentState;

    return 0;
}
bool UnixPciDriver::SetDriverMode(int newState, int &oldState)
{
    //do something with params
    (void)newState;
    (void)oldState;
    return false;
}

void UnixPciDriver::Reset()
{
    return;
}

bool UnixPciDriver::DriverControl(uint32_t Id,
                                  const void *inBuf, uint32_t inBufSize,
                                  void *outBuf, uint32_t outBufSize)
{
    //do something with params
    (void)Id;
    (void)inBuf;
    (void)inBufSize;
    (void)outBuf;
    (void)outBufSize;
    return false;
}

bool UnixPciDriver::SendRWIoctl(IoctlIO & io, int fd, const char* interfaceName)
{
    int ret;
    struct ifreq ifr;
    ifr.ifr_data = (char*)&io;

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", interfaceName);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    ret = ioctl(fd, WIL_IOCTL_MEMIO, &ifr); // read/write DWORD
    if (ret < 0)
    {
        return false;
    }

    return true;
}

bool UnixPciDriver::SendRWBIoctl(IoctlIOBlock & io, int fd, const char* interfaceName)
{
    int ret;
    struct ifreq ifr;
    ifr.ifr_data = (char*)&io;

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", interfaceName);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    ret = ioctl(fd, WIL_IOCTL_MEMIO_BLOCK, &ifr);  // read/write BYTES. number of bytes must be multiple of 4
    if (ret < 0)
    {
        return false;
    }

    return true;
}

// Receives interface name (wigig#, wlan#) and checks if it is responding
bool UnixPciDriver::ValidateInterface()
{
    IoctlIO io;
    io.addr = BAUD_RATE_REGISTER;
    io.op = EP_OPERATION_READ;

    if(SendRWIoctl(io, m_fileDescriptor, m_interfaceName.c_str()))
    {
        return true;
    }

    return false;
}

bool UnixPciDriver::IsValidInternal()
{
    if(-1 == access(m_debugFsPath.c_str(), F_OK)) // didn't find debug FS
    {
        return false;
    }

    return true;
}

string UnixPciDriver::NameDevice(string interfaceName)
{
    stringstream ss;
    ss << Utils::PCI << DEVICE_NAME_DELIMITER << interfaceName;
    return ss.str();
}
#endif // ifndef _WINDOWS

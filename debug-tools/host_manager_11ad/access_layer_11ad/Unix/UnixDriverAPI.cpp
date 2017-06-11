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
#include "UnixDriverAPI.h"
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

UnixDriverAPI::~UnixDriverAPI()
{
    Close();
}

// Base access functions (to be implemented by specific device)
bool UnixDriverAPI::Read(DWORD address, DWORD& value)
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

bool UnixDriverAPI::ReadBlock(DWORD addr, DWORD blockSize, char *arrBlock)
{
    IoctlIOBlock io;
    io.op = EP_OPERATION_READ;
    io.addr = addr;
    io.size = blockSize;
    io.buf = arrBlock;

    return SendRWBIoctl(io, m_fileDescriptor, m_interfaceName.c_str());
}

bool UnixDriverAPI::Write(DWORD address, DWORD value)
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
bool UnixDriverAPI::WriteBlock(DWORD addr, DWORD blockSize, const char *arrBlock)
{
    //blocks must be 32bit alligned!
    int sizeToWrite = (blockSize / 4) * 4;

    IoctlIOBlock io;
    io.op = EP_OPERATION_WRITE;
    io.addr = addr;
    io.size = sizeToWrite;
    io.buf = (void*)arrBlock;

    return SendRWBIoctl(io, m_fileDescriptor, m_interfaceName.c_str());
}

bool UnixDriverAPI::IsOpened(void)
{
    return m_initialized;
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

set<string> UnixDriverAPI::Enumerate()
{
    set<string> interfaces = GetNetworkInterfaceNames();

    return interfaces;
}

bool UnixDriverAPI::Open()
{
    if(IsOpened())
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

bool UnixDriverAPI::SetDebugFsPath() // assuming m_interfaceName is set
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

bool UnixDriverAPI::ReOpen()
{
    return Open();
}

bool UnixDriverAPI::InternalOpen()
{
    return true;
}

DWORD UnixDriverAPI::DebugFS(char *FileName, void *dataBuf, DWORD dataBufLen, DWORD DebugFSFlags)
{
    //do something with params
    (void)FileName;
    (void)dataBuf;
    (void)dataBufLen;
    (void)DebugFSFlags;
    return 0;
}

bool UnixDriverAPI::AllocPmc(unsigned descSize, unsigned descNum, string& outMessage)
{
    PmcCfg pmcCfg(m_debugFsPath.c_str());
    OperationStatus st = pmcCfg.AllocateDmaDescriptors(descSize, descNum);

    outMessage = st.GetMessage();
    return st.IsSuccess();
}

bool UnixDriverAPI::DeallocPmc(std::string& outMessage)
{
    PmcCfg pmcCfg(m_debugFsPath.c_str());
    OperationStatus st = pmcCfg.FreeDmaDescriptors();

    outMessage = st.GetMessage();
    return st.IsSuccess();
}

bool UnixDriverAPI::CreatePmcFile(unsigned refNumber, std::string& outMessage)
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

bool UnixDriverAPI::FindPmcFile(unsigned refNumber, std::string& outMessage)
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

void UnixDriverAPI::Close()
{
    if (m_fileDescriptor != INVALID_FD)
    {
        close(m_fileDescriptor);
        m_fileDescriptor = INVALID_FD;
    }
}

int UnixDriverAPI::GetDriverMode(int &currentState)
{
    //do something with params
    (void)currentState;

    return 0;
}
bool UnixDriverAPI::SetDriverMode(int newState, int &oldState)
{
    //do something with params
    (void)newState;
    (void)oldState;
    return false;
}

void UnixDriverAPI::Reset()
{
    return;
}

bool UnixDriverAPI::DriverControl(uint32_t Id,
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

bool UnixDriverAPI::SendRWIoctl(IoctlIO & io, int fd, const char* interfaceName)
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

bool UnixDriverAPI::SendRWBIoctl(IoctlIOBlock & io, int fd, const char* interfaceName)
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
bool UnixDriverAPI::ValidateInterface()
{
    IoctlIO io;
    io.addr = 0x880050; //baud rate
    io.op = EP_OPERATION_READ;

    if(SendRWIoctl(io, m_fileDescriptor, m_interfaceName.c_str()))
    {
        return true;
    }

    return false;
}

#endif // ifndef _WINDOWS
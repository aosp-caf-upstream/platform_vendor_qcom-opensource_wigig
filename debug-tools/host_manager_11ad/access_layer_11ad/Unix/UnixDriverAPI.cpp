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

#include <iostream>

#include "UnixDriverAPI.h"

#include <net/if.h> // for struct ifreq
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h> // for struct ifreq

#define END_POINT_RESULT_MAX_SIZE 256
#define NETWORK_INTERFACE_RESULT_MAX_SIZE 256

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
    IoctlIO io;

    // cout << "Read block addr = " << addr << ". block size = " << blockSize << "\n";

    //blocks must be 32bit alligned!
    int numReads = blockSize / 4;
    io.op = EP_OPERATION_READ;
    io.addr = addr;
    for (int i = 0; i < numReads; i++)
    {
        if (!SendRWIoctl(io, m_fileDescriptor, m_interfaceName.c_str()))
        {
            // cout << "Failed to Read in ReadBlock";
            return false;
        }

        // Copy the read 32 bits into the arrBlock
        memcpy(&arrBlock[(i * 4)], &io.val, sizeof(int32_t));
        // cout << "Read block " << i << " out of " << numReads << ". val = " << io.val << "\n";

        io.addr += sizeof(int32_t);
    }

    return true;
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
    // cout << "Write Block addr = " << addr << ". block size = " << blockSize << "\n";

    IoctlIO io;

    io.addr = addr;

    //blocks must be 32bit alligned!
    int sizeToWrite = blockSize / 4;
    io.op = EP_OPERATION_WRITE;
    for (int i = 0; i < sizeToWrite; i++)
    {
        io.val = *((int*)&(arrBlock[i * 4]));

        // cout << "Writing " << io.val << " to address " << io.addr << "\n";

        Write(io.addr, io.val);

        io.addr += sizeof(int32_t);
    }

    return true;
}

bool UnixDriverAPI::IsOpened(void)
{
    return m_initialized;
}

string GetInterfaceNameFromEP(string pciEndPoint)
{
    char szInterfaceCmdPattern[END_POINT_RESULT_MAX_SIZE];

    // This command translates the End Point to the interface name
    snprintf(szInterfaceCmdPattern, END_POINT_RESULT_MAX_SIZE, "ls /sys/module/wil6210/drivers/pci:wil6210/%s/net", pciEndPoint.c_str());

    FILE* pIoStream = popen(szInterfaceCmdPattern, "r");
    if (!pIoStream)
    {
        // cout << "Failed to run command to detect End Points" << "\n";
        //LOG_MESSAGE_ERROR("Failed to run command to detect End Points\n" );
        return "Invalid";
    }

    char foundInterfaceName[NETWORK_INTERFACE_RESULT_MAX_SIZE];

    while (fgets(foundInterfaceName, END_POINT_RESULT_MAX_SIZE, pIoStream) != NULL)
    {
        // The command output contains a newline character that should be removed
        foundInterfaceName[strcspn(foundInterfaceName, "\r\n")] = '\0';

        string interfaceName(foundInterfaceName);

        // cout << "PCI interface found: " << interfaceName << "\n";
        //LOG_MESSAGE_DEBUG("PCI interface found: %s", interfaceName);

        pclose(pIoStream);

        return interfaceName;
    }

    pclose(pIoStream);
    return "Invalid";
}

set<string> GetNetworkInterfaceNames()
{
    set<string> networkInterfaces;

    // Holds the console ouput containing the current End Point
    char pciEndPoints[END_POINT_RESULT_MAX_SIZE];

    // This command retrieves the PCI endpoints enumerated
    const char* szCmdPattern = "ls /sys/module/wil6210/drivers/pci\\:wil6210 | grep :";

    FILE* pIoStream = popen(szCmdPattern, "r");
    if (!pIoStream)
    {
        // cout << "Failed to run command to detect End Points" << "\n";
        //LOG_MESSAGE_ERROR("Failed to run command to detect End Points\n" );
        return networkInterfaces;
    }

    while (fgets(pciEndPoints, END_POINT_RESULT_MAX_SIZE, pIoStream) != NULL)
    {
        // The command output contains a newline character that should be removed
        pciEndPoints[strcspn(pciEndPoints, "\r\n")] = '\0';

        string pciEndPoint(pciEndPoints);
        // cout << "PCI End Point Found:" << pciEndPoint << "\n";
        //LOG_MESSAGE_DEBUG("PCI End Point Found: %s", pciEndPoint.c_str());

        // Get interface name from End Point
        string networkInterfaceName = GetInterfaceNameFromEP(pciEndPoint);

        if (networkInterfaceName != "Invalid")
        {
            networkInterfaces.insert(networkInterfaceName);
        }
    }

    pclose(pIoStream);
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

    // cout << "Trying to open device:" << m_interfaceName << "\n";
    //LOG_MESSAGE_DEBUG("Trying to open device:", m_interfaceName.c_str());

    m_fileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (INVALID_FD == m_fileDescriptor || m_fileDescriptor < 0)
    {
        // cout << "Failed to open socket to device:" << m_interfaceName << "\n";
        //LOG_MESSAGE_ERROR("Failed to open socket to device %s", m_interfaceName.c_str());
        Close();
        return false;
    }

    // Validate interface is 11ad interface
    if(!ValidateInterface())
    {
        // cout << "Failed to query interface:" << m_interfaceName << "\n";
        //LOG_MESSAGE_ERROR("Failed to query interface %s", m_interfaceName.c_str());
        Close();
        return false;
    }

    // Unix command for getting Debug FS path
    string debugFsFind = "echo \"/sys/kernel/debug/ieee80211/$(ls $(find /sys/devices -name " + m_interfaceName + ")/../../ieee80211 | grep -Eo \"phy[0-9]*\")/wil6210\"";

    // cout << debugFsFind << "\n";

    FILE* pIoStream = popen(debugFsFind.c_str(), "r");
    if (!pIoStream)
    {
        // cout << "Failed to run command to detect DebugFS" << "\n";
        //LOG_MESSAGE_ERROR("Failed to run command to detect DebugFS\n" );
        Close();
        return false;
    }

    char debugFSPath[DEBUG_FS_MAX_PATH_LENGTH];
    while (fgets(debugFSPath, DEBUG_FS_MAX_PATH_LENGTH, pIoStream) != NULL)
    {
        // The command output contains a newline character that should be removed
        debugFSPath[strcspn(debugFSPath, "\r\n")] = '\0';
        string str = (debugFSPath);
        m_debugFsPath = str;

        // cout << "Found DebugFS Path:" << m_debugFsPath << "\n";
        //LOG_MESSAGE_DEBUG("Found DebugFS Path: %s", debugFSPath);
    }

    pclose(pIoStream);
    m_initialized = true;
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

bool UnixDriverAPI::Ioctl(uint32_t Id,
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

    // cout << "Address is " << hex << io.addr << "\n";
    // cout << "fd is " << fd << "\n";
    // cout << "interfaceName is " << interfaceName << "\n";

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", interfaceName);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    ret = ioctl(fd, WIL_IOCTL_MEMIO, &ifr);
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

    // cout << "Checking interface name:" << m_interfaceName << "\n";
    //LOG_MESSAGE_DEBUG("Checking interface name: %s", m_interfaceName.c_str());

    if(SendRWIoctl(io, m_fileDescriptor, m_interfaceName.c_str()))
    {
        // cout << "Successfuly set interface name:" << m_interfaceName << "\n";
        //LOG_MESSAGE_DEBUG("Successfuly set interface name: %s", interfaceName);
        return true;
    }

    return false;
}

#endif // ifndef _WINDOWS

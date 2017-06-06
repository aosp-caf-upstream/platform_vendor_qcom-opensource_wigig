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

#ifndef _11AD_PCI_DEVICE_H_
#define _11AD_PCI_DEVICE_H_

#ifdef _WINDOWS
//#include "ioctl_if.h"
#else
// some definitions from ioctl_if.h
#define WILOCITY_IOCTL_INDIRECT_READ IOCTL_INDIRECT_READ_OLD
#define WILOCITY_IOCTL_INDIRECT_WRITE IOCTL_INDIRECT_WRITE_OLD
#define WILOCITY_IOCTL_INDIRECT_READ_BLOCK IOCTL_INDIRECT_READ_BLOCK
#define WILOCITY_IOCTL_INDIRECT_WRITE_BLOCK IOCTL_INDIRECT_WRITE_BLOCK
#endif

#include <memory>
#include "Device.h"
#include "DriverAPI.h"

using namespace std;

class PciDevice : public Device
{
public:
    PciDevice(string deviceName, string interfaceName);
    virtual ~PciDevice();

    // Device Management
    bool Open();
    void Close();

    // Virtual access functions for device
    bool Read(DWORD address, DWORD& value);
    bool ReadBlock(DWORD address, DWORD blockSize, vector<DWORD>& values);
    bool Write(DWORD address, DWORD value);
    bool WriteBlock(DWORD address, vector<DWORD> values);

    virtual bool AllocPmc(unsigned descSize, unsigned descNum, std::string& outMessage);
    virtual bool DeallocPmc(std::string& outMessage);
    virtual bool CreatePmcFile(unsigned refNumber, std::string& outMessage);
    virtual bool FindPmcFile(unsigned refNumber, std::string& outMessage);

    virtual void InterfaceReset();

    virtual bool SetDriverMode(int newState, int &oldState);
    virtual int GetDriverMode(int &currentState);
    virtual bool DriverControl(uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize);

    static set<string> Enumerate();

private:
    void rr32(DWORD addr, DWORD num_repeat, DWORD *arrBlock);
    bool ReOpen();
    bool try2open(int timeout);

    static void SamplingThreadProc(void *pDeviceAccss);
    static void PciPluginThreadProc(void *pDeviceAccss);

    // The driver API for the PCI access (OS specific)
    unique_ptr<DriverAPI> m_pDriverApi;
};

#endif //_11AD_PCI_DEVICE_H_

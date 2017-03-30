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

#ifndef _11AD_PCI_DRIVER_API_H_
#define _11AD_PCI_DRIVER_API_H_

#include <string>
#include <set>

#include "OperatingSystemConstants.h"

using namespace std;

class DriverAPI
{
public:
    DriverAPI(string interfaceName)
    {
        m_interfaceName = interfaceName;
    }
    virtual ~DriverAPI() {};

    // Base access functions (to be implemented by specific device)
    virtual bool Read(DWORD address, DWORD& value) = 0;
    virtual bool ReadBlock(DWORD addr, DWORD blockSize, char *arrBlock) = 0;
    virtual bool Write(DWORD address, DWORD value) = 0;
    virtual bool WriteBlock(DWORD addr, DWORD blockSize, const char *arrBlock) = 0;

    virtual bool IsOpened(void) = 0;

    virtual bool Open() = 0;
    virtual bool ReOpen() = 0;
    virtual bool Ioctl(uint32_t Id,
                       const void *inBuf, uint32_t inBufSize,
                       void *outBuf, uint32_t outBufSize) = 0;
    virtual DWORD DebugFS(char *FileName, void *dataBuf, DWORD dataBufLen, DWORD DebugFSFlags)
    {
        //do something with params
        (void)FileName;
        (void)dataBuf;
        (void)dataBufLen;
        (void)DebugFSFlags;
        return -1;
    }
    virtual void          Close() = 0;

    virtual int GetDriverMode(int &currentState) = 0;
    virtual bool SetDriverMode(int newState, int &oldState) = 0;

    virtual void Reset() = 0;

protected:
    string m_interfaceName;
    void* m_deviceHandle;

private:
};


#endif //_11AD_PCI_DRIVER_API_H_

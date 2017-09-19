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

#ifndef _11AD_DRIVER_API_H_
#define _11AD_DRIVER_API_H_

#include <string>
#include <set>
#include <vector>
#include <sstream>

#include "OperatingSystemConstants.h"
#include "Definitions.h"
#include "../DebugLogger.h"
#include "../Utils.h"

using namespace std;

class DriverAPI
{
public:
    DriverAPI(string interfaceName)
    {
        m_interfaceName = interfaceName;
        m_initialized = false;
    }
    virtual ~DriverAPI() {};

    // Base access functions (to be implemented by specific device)
    virtual bool Read(DWORD address, DWORD& value) { return false;  }
    virtual bool ReadBlock(DWORD addr, DWORD blockSize, vector<DWORD>& values) { return false; }
    virtual bool Write(DWORD address, DWORD value) { return false; }
    virtual bool WriteBlock(DWORD addr, vector<DWORD> values) { return false; };

    // PMC functions
    virtual bool AllocPmc(unsigned descSize, unsigned descNum, std::string& outMessage) { return false; }
    virtual bool DeallocPmc(std::string& outMessage) { return false; }
    virtual bool CreatePmcFile(unsigned refNumber, std::string& outMessage) { return false; }
    virtual bool FindPmcFile(unsigned refNumber, std::string& outMessage) { return false; }

    virtual bool IsOpen(void) { return m_initialized;  }
    virtual bool Open() { return false;  }
    virtual bool ReOpen() { return false; };
    virtual bool DriverControl(uint32_t Id,
        const void *inBuf, uint32_t inBufSize,
        void *outBuf, uint32_t outBufSize) { return false; }
    virtual DWORD DebugFS(char *FileName, void *dataBuf, DWORD dataBufLen, DWORD DebugFSFlags)
    {
        //do something with params
        (void)FileName;
        (void)dataBuf;
        (void)dataBufLen;
        (void)DebugFSFlags;
        return -1;
    }
    virtual void Close() {}

    virtual int GetDriverMode(int &currentState) { return false;  }
    virtual bool SetDriverMode(int newState, int &oldState) { return false;  }

    virtual void Reset() {}; // interface reset

    const string& GetInterfaceName() const
    {
        return m_interfaceName;
    }

protected:
    string m_interfaceName;
    bool m_initialized;
};


#endif //_11AD_DRIVER_API_H_
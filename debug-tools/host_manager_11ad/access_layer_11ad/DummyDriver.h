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

#ifndef _11AD_DUMMY_DRIVER_H_
#define _11AD_DUMMY_DRIVER_H_

#include <map>
#include "DriverAPI.h"

using namespace std;

class DummyDriver : public DriverAPI
{
public:
    DummyDriver(string interfaceName);
    ~DummyDriver();

    // Device Management
    bool Open();
    void Close();

    // Virtual access functions for device
    bool Read(DWORD address, DWORD& value);
    bool ReadBlock(DWORD address, DWORD blockSize, vector<DWORD>& values);
    bool Write(DWORD address, DWORD value);
    bool WriteBlock(DWORD address, vector<DWORD> values);

    virtual void InterfaceReset();
    bool SwReset();

    // PMC functions
    virtual bool AllocPmc(unsigned descSize, unsigned descNum, std::string& outMessage) { return false; };
    virtual bool DeallocPmc(std::string& outMessage) { return false; };
    virtual bool CreatePmcFile(unsigned refNumber, std::string& outMessage) { return false; };
    virtual bool FindPmcFile(unsigned refNumber, std::string& outMessage) { return false; };

    virtual bool ReOpen() { return false; };
    virtual bool DriverControl(uint32_t Id, const void *inBuf, uint32_t inBufSize, void *outBuf, uint32_t outBufSize, DWORD* pLastError = nullptr) { return false; };

    virtual int GetDriverMode(int &currentState) { return 0; };
    virtual bool SetDriverMode(int newState, int &oldState) { return false; };

    virtual void Reset() { return; };

    static set<string> Enumerate();

private:
    map<DWORD, DWORD> m_registersAddressToValue;
};
#endif //_11AD_DUMMY_DRIVER_H_

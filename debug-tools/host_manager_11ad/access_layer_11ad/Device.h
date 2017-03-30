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

#ifndef _11AD_DEVICE_H_
#define _11AD_DEVICE_H_

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include "OperatingSystemConstants.h"
#include "Definitions.h"
#include "../DebugLogger.h"

using namespace std;

class Device
{
public:
    Device(string deviceName, string interfaceName)
    {
        m_deviceName = deviceName;
        m_interfaceName = interfaceName;
        m_initialized = false;

        m_basebandType = BASEBAND_TYPE_NONE;
    }

    // ************************** Device API ****************************//
    // Device Management
    virtual bool Open() { return false; };
    virtual void Close() {};

    // Base access functions (to be implemented by specific device)
    virtual bool Read(DWORD address, DWORD& value)
    {
        //do something with params
        (void)address;
        (void)value;
        return false;
    };
    virtual bool ReadBlock(DWORD address, DWORD blockSize, vector<DWORD>& values)
    {
        //do something with params
        (void)address;
        (void)blockSize;
        (void)values;
        return false;
    };

    virtual bool Write(DWORD address, DWORD value)
    {
        //do something with params
        (void)address;
        (void)value;
        return false; };
    virtual bool WriteBlock(DWORD address, vector<DWORD> values)
    {
        //do something with params
        (void)address;
        (void)values;
        return false; };

    // Polling function
    //vector<DeviceEvent> DoPoll();

    // Functionality common to all devices
    bool SwReset() { return false; }

    virtual bool AllocPmc(unsigned descSize, unsigned descNum)
    {
        //do something with params
        (void)descSize;
        (void)descNum;
        return false;
    } // TODO: implement

    virtual bool DeallocPmc() { return false; } // TODO: implement

    virtual bool CreatePmcFile(unsigned refNumber)
    {
        //do something with params
        (void)refNumber;
        return false;
    } // TODO: implement

    virtual void InterfaceReset() {};

    virtual bool SetDriverMode(int newMode, int& oldMode)
    {
        //do something with params
        (void)newMode;
        (void)oldMode;
        return false;
    }

    bool SendWmi(DWORD command, vector<DWORD> payload)
    {
        //do something with params
        (void)command;
        (void)payload;
        return false;

    }
    vector<DWORD> GetWmiEvent()
    {

        vector<DWORD> ret;
        return ret;
    }

    bool IsOpen()
    {
        return m_initialized;
    }

    void Poll()
    {
        // TODO: implement polling: MB, logs, rgf
    }

    static unsigned int GetRegisterDefaultValue()
    {
        return 0xDEADDEAD;
    }
    // ************************** [END] Device API **********************//

    string GetDeviceName()
    {
        return m_deviceName;
    }

    string GetInterfaceName()
    {
        return m_interfaceName;
    }

    BasebandType GetBasebandType()
    {
        if (m_basebandType == BASEBAND_TYPE_NONE)
        {
            m_basebandType = ReadBasebandType();
        }

        return m_basebandType;
    }
    virtual ~Device(){};
protected:
    DWORD m_deviceHandle;
    bool m_initialized;
private:
    string m_deviceName;
    string m_interfaceName;
    BasebandType m_basebandType;

    BasebandType ReadBasebandType()
    {
        DWORD jtagVersion;
        const int rev_id_address = 0x880B34; //USER.JTAG.USER_USER_JTAG_1.dft_idcode_dev_id
        const int device_id_mask = 0x0fffffff; //take the first 28 bits from the Jtag Id
        BasebandType res = BASEBAND_TYPE_NONE;

        if (!Read(rev_id_address, jtagVersion))
        {
            LOG_ERROR << "Failed to read baseband type" << "\n";
        }

        LOG_INFO << "JTAG rev ID = " << hex << jtagVersion << "\n";

        switch (jtagVersion & device_id_mask)
        {
        case 0x612072F:
            res = BASEBAND_TYPE_MARLON;
            break;
        case 0x632072F:
            res = BASEBAND_TYPE_SPARROW;
            break;
        case 0x642072F:
        case 0x007E0E1:
            res = BASEBAND_TYPE_TALYN;
            break;
        default:
            ////LOG_MESSAGE_WARN("Invalid device type - assuming Sparrow");
            res = BASEBAND_TYPE_SPARROW;
            break;
        }

        return res;
    }
};

#endif // !_11AD_DEVICE_H_

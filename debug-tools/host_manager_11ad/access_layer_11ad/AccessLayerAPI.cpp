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

#include <sstream>

#include "AccessLayerAPI.h"

#ifdef _WINDOWS
#include "JTagDevice.h"
#include "SerialDevice.h"
#endif
#include "TestDevice.h"

#include "PciDevice.h"
#include "../Utils.h"

using namespace std;

set<string> AccessLayer::GetDevices()
{
    set<string> enumeratedDevices;

    // Enumerate
    set<string> pciDevices = PciDevice::Enumerate();

#ifdef _WINDOWS
    set<string> jtagDevices = JTagDevice::Enumerate();
    enumeratedDevices.insert(jtagDevices.begin(), jtagDevices.end());

    //set<string> serialDevices = SerialDevice::Enumerate();
    //enumeratedDevices.insert(serialDevices.begin(), serialDevices.end());
#endif

    enumeratedDevices.insert(pciDevices.begin(), pciDevices.end());
#ifdef _UseTestDevice
    set<string> testDevices = TestDevice::Enumerate();
    enumeratedDevices.insert(testDevices.begin(), testDevices.end());
#endif // _UseTestDevice

    return enumeratedDevices;
}

unique_ptr<Device> AccessLayer::OpenDevice(string deviceName)
{
    vector<string> tokens = Utils::Split(deviceName, DEVICE_NAME_DELIMITER);

    // Device name consists of exactly 3 elements:
    // 1. Baseband Type (SPARROW, TALYN...)
    // 2. Transport Type (PCI, JTAG, Serial...)
    // 3. Interface name (wMp, wPci, wlan0, wigig0...)
    if (tokens.size() != 2)
    {
        // LOG_MESSAGE_ERROR
        return NULL;
    }

    // Transport type
    unique_ptr<Device> pDevice;

    if ("PCI" == tokens[0])
    {
        pDevice.reset(new PciDevice(deviceName, tokens[1]));
    }

#ifdef _WINDOWS
    if ("JTAG" == tokens[0])
    {
        pDevice.reset(new JTagDevice(deviceName, tokens[1]));
    }

    if ("SERIAL" == tokens[0])
    {
        pDevice.reset(new SerialDevice(deviceName, tokens[1]));
    }
#endif

#ifdef _UseTestDevice
    if ("TEST" == tokens[0])
    {
        pDevice.reset(new TestDevice(deviceName, tokens[1]));
    }
#endif // _UseOnlyTestDevice


    return pDevice;
}

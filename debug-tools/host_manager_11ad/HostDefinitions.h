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

#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_

#include <string>
#include <cstdint>
#include "Utils.h"


// *************************************************************************************************
/*
    * The reply data may be generated in several ways, the data is expected to be obtained according to this type
    */
enum REPLY_TYPE
{
    REPLY_TYPE_NONE,
    REPLY_TYPE_BUFFER,
    REPLY_TYPE_FILE
};

// *************************************************************************************************
/*
    * A response to the client, through the servers
    */
typedef struct
{
    std::string message;
    REPLY_TYPE type;
    unsigned int length;
} ResponseMessage;

// *************************************************************************************************
/*
    * ConnectionStatus indicates whether to close a connection with a client or not.
    */
enum ConnectionStatus
{
    CLOSE_CONNECTION,
    KEEP_CONNECTION_ALIVE
};

// **************************** Events Structures and Enum Types **************************************
/*
    * Define an event struct which would be sent as bytes in packed mode.
    * The concept is to send the smallest message we can as it being sent many times and to multiple clients.
    * NOTE that the __exactly__ same struct is defined in the side that gets this struct.
    * The structures sent in packed mode (by using "#pragma pack(push, 1)")
    */

#pragma pack(push, 1) //following structures would be in packed mode

enum HOST_EVENT_TYPE : uint8_t
{
    VERSION_CHANGED,
    CONNECTED_USERS_CHANGED,
    DEVICES_LIST_CHANGED,
    DEVICE_EVENT
};

struct HostEvent
{
    HostEvent(HOST_EVENT_TYPE hostEventType) :
        m_marker("TheUtopicHostManager11ad"),
        m_currentTime(Utils::GetCurrentLocalTime()),
        m_eventType(hostEventType)
    { }

    const string m_marker; // for stream synchronization in DmTools
    string m_currentTime; // event's genaration time, in YYYY-MM-DD HH:mm:ss.<ms><ms><ms> format
    HOST_EVENT_TYPE m_eventType; //Value size is uint8_t
    uint32_t m_payloadSize;
    uint8_t *payLoad; //The size of the payload (which is the raw data from the device) is given in "payloadSize"
};

//The type of the event coming from the device - as the device aware of it
enum DEVICE_EVENT_TYPE : uint8_t
{
    WMI_EVENT,
    LOG_EVENT,
    POLLER_EVENT
};

typedef enum _DRIVER_MODE {
	IOCTL_WBE_MODE,
	IOCTL_WIFI_STA_MODE,
	IOCTL_WIFI_SOFTAP_MODE,
	IOCTL_CONCURRENT_MODE,    // This mode is for a full concurrent implementation  (not required mode switch between WBE/WIFI/SOFTAP)
	IOCTL_SAFE_MODE,          // A safe mode required for driver for protected flows like upgrade and crash dump...
} DRIVER_MODE;

struct DeviceEvent : public HostEvent
{
    DeviceEvent(DEVICE_EVENT_TYPE deviceEventType) :
        HostEvent(DEVICE_EVENT),
        m_deviceEventType(deviceEventType)
        {}

    DEVICE_EVENT_TYPE m_deviceEventType; //This value size is uint8_t
    uint16_t m_deviceNameSize; //This value would be sent with the package
    uint8_t* m_deviceName; //The size of the device name is in "deviceNameSize"
    uint32_t m_payloadSize;
    uint8_t* m_payLoad; //The size of the payload (which is the raw data from the device) is given in "payloadSize"
};


//Example for another host event
//typedef struct _DevicesChangedEvent : HostEvent
//{
//
//} DevicesChangedEvent;

#pragma pack(pop) //stop using packed mode

// *************************************************************************************************

#define MAX_REGS_LEN    (256* 1024)    // Max registers to read/write at once //TODO - is it needed? maybe read until line terminator?

// Max buffer size for a command and a reply. We should consider the longest command/data sequence to fit in a buffer.
// rb reading 1024 (MAX_REGS_LEN) registers: rb HHHHHHHH HHHH 1024*(0xHHHHHHHH) = 17 + 1024*10 = 10257 bytes
// wb writing 1024 (MAX_REGS_LEN) hex values: wb HHHHHHHH "1024*HH" = 14+1024*2 = 2062 bytes
#define MAX_INPUT_BUF (11*MAX_REGS_LEN) //TODO - is it needed? maybe read until line terminator?

// *************************************************************************************************

struct HostIps
{
    HostIps() :
        m_ip(""),
        m_broadcastIp("")
    {}

    std::string m_ip; // host's IP address
    std::string m_broadcastIp; // host's broadcast IP address (derivated by subnet mask)
};

// *************************************************************************************************

enum ServerType
{
    stTcp,
    stUdp
};

#endif // !_DEFINITIONS_H_

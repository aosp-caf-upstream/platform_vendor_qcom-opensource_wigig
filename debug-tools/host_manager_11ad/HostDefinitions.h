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

#include <map>
#include <string>
#include <cstdint>
#include <tuple>
#include "Utils.h"

// FW Defines
#define MAX_RF_NUMBER 8


#define FW_ASSERT_REG 0x91F020
#define UCODE_ASSERT_REG 0x91F028
#define FW_ASSOCIATION_REG 0x880A44
#define FW_ASSOCIATED_VALUE 0x6
#define MCS_REG 0x880A60
#define DEVICE_TYPE_REG 0x880A8C
#define FW_MODE_REG 0x880A34
#define BOOT_LOADER_VERSION_REG 0x880A0C
#define CHANNEL_REG 0x883020
#define BOARDFILE_REG 0x880014

#define UCODE_RX_ON_REG 0x9405BE
#define BF_SEQ_REG 0x941374
#define BF_TRIG_REG 0x941380
#define NAV_REG 0x940490
#define TX_GP_REG 0x880A58
#define RX_GP_REG 0x880A5C

#define RF_CONNECTED_REG 0x889488
#define RF_ENABLED_REG 0x889488

#define BASEBAND_TEMPERATURE_REG 0x91c808
#define RF_TEMPERATURE_REG 0x91c80c

// *************************************************************************************************
/*
 * The reply data may be generated in several ways, the data is expected to be obtained according to this type
 */
enum REPLY_TYPE
{
    REPLY_TYPE_NONE,
    REPLY_TYPE_BUFFER,
    REPLY_TYPE_FILE,
    REPLY_TYPE_WAIT_BINARY,
    REPLY_TYPE_BINARY
};

// *************************************************************************************************
/*
 * A response to the client, through the servers
 */
struct ResponseMessage
{
    ResponseMessage() :
        type(REPLY_TYPE_NONE),
        length(0U),
        binaryMessage(nullptr),
        inputBufSize(0U)
    {}

    std::string message;
    REPLY_TYPE type;
    size_t length;
    uint8_t* binaryMessage;
    vector<string> internalParsedMessage;
    unsigned inputBufSize;
};

// *************************************************************************************************
/* A response to the user in CLI mode
*/
struct Status
{
    Status() :
        m_success(true),
        m_message("") {}

    Status(bool success, std::string message) :
        m_success(success),
        m_message(message) {}

    bool m_success;
    std::string m_message;
};

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
 * Define an event struct which would be sent as Json string.
 * NOTE that the __exactly__ same struct is defined in the side that gets this struct.
 */

struct FwVersion
{
    DWORD m_major;
    DWORD m_minor;
    DWORD m_subMinor;
    DWORD m_build;

    FwVersion() :
        m_major(0), m_minor(0), m_subMinor(0), m_build(0)
    {}

    bool operator==(const FwVersion& rhs) const
    {
        return std::tie(m_major, m_minor, m_subMinor, m_build) == std::tie(rhs.m_major, rhs.m_minor, rhs.m_subMinor, rhs.m_build);
    }
};

struct FwTimestamp
{
    FwTimestamp() :
        m_hour(0), m_min(0), m_sec(0), m_day(0), m_month(0), m_year(0)
    {}

    DWORD m_hour;
    DWORD m_min;
    DWORD m_sec;
    DWORD m_day;
    DWORD m_month;
    DWORD m_year;

    bool operator==(const FwTimestamp& rhs) const
    {
        return std::tie(m_hour, m_min, m_sec, m_day, m_month, m_year) == std::tie(rhs.m_hour, rhs.m_min, rhs.m_sec, rhs.m_day, rhs.m_month, rhs.m_year);
    }
};

//enum DRIVER_MODE
//{
//    IOCTL_WBE_MODE,
//    IOCTL_WIFI_STA_MODE,
//    IOCTL_WIFI_SOFTAP_MODE,
//    IOCTL_CONCURRENT_MODE,    // This mode is for a full concurrent implementation  (not required mode switch between WBE/WIFI/SOFTAP)
//    IOCTL_SAFE_MODE,          // A safe mode required for driver for protected flows like upgrade and crash dump...
//};

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

const int SECOND_IN_MILLISECONDS = 1000;

#endif // !_DEFINITIONS_H_

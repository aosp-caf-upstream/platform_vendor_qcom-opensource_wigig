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
#ifndef _11AD_EVENTS_DEFINITIONS_H_
#define _11AD_EVENTS_DEFINITIONS_H_

#include <sstream>
#include <ostream>
#include <string>
#include <set>
#include "Utils.h"
#include "HostDefinitions.h"
#include "HostManagerDefinitions.h"
#include "LogCollectorDefinitions.h"
#include "JsonSerializeHelper.h"

//=== Base class for all events to be pushed to DMTools
class HostManagerEventBase
{
public:
    explicit HostManagerEventBase(const std::string& deviceName) :
        m_timestampLocal(std::move(Utils::GetCurrentLocalTimeString())),
        m_deviceName(deviceName)
    {}

    virtual ~HostManagerEventBase() {}

    // Serialization entry point
    void ToJson(std::ostream& os) const
    {
        JsonSerializeHelper jsh(os);
        jsh.Serialize("__type", GetTypeName());
        jsh.Serialize("TimestampLocal", m_timestampLocal);
        jsh.Serialize("DeviceName", m_deviceName);
        ToJsonInternal(jsh);
    }

private:
    std::string m_timestampLocal;    // creation time
    std::string m_deviceName;

    // enforce descendants to implement it and so make the class abstract
    virtual const char* GetTypeName(void) const = 0;
    virtual void ToJsonInternal(JsonSerializeHelper& jsh) const = 0;
};

// =================================================================================== //
// NewDeviceDiscoveredEvent

// FW version struct with serialization support
class SerializableFwVersion final : public FwVersion, public JsonSerializable
{
public:
    explicit SerializableFwVersion(const FwVersion& rhs) : FwVersion(rhs) {}

    void ToJson(ostream& os) const override
    {
        JsonSerializeHelper jsh(os);
        jsh.Serialize("Major", m_major);
        jsh.Serialize("Minor", m_minor);
        jsh.Serialize("SubMinor", m_subMinor);
        jsh.Serialize("Build", m_build);
    }
};

// FW compilation timestamp with serialization support
class SerializableFwTimestamp final : public FwTimestamp, public JsonSerializable
{
public:
    explicit SerializableFwTimestamp(const FwTimestamp& rhs) : FwTimestamp(rhs) {}

    void ToJson(ostream& os) const override
    {
        JsonSerializeHelper jsh(os);
        jsh.Serialize("Hour", m_hour);
        jsh.Serialize("Min", m_min);
        jsh.Serialize("Sec", m_sec);
        jsh.Serialize("Day", m_day);
        jsh.Serialize("Month", m_month);
        jsh.Serialize("Year", m_year);
    }
};

// Event to be sent upon new device discovery or device FW change
class NewDeviceDiscoveredEvent final : public HostManagerEventBase
{
public:
    NewDeviceDiscoveredEvent(const std::string& deviceName, const FwVersion& fwVersion, const FwTimestamp& fwTimestamp) :
        HostManagerEventBase(deviceName),
        m_fwVersion(fwVersion),
        m_fwTimestamp(fwTimestamp)
    {}

private:
    SerializableFwVersion m_fwVersion;
    SerializableFwTimestamp m_fwTimestamp;

    const char* GetTypeName() const override
    {
        return "NewDeviceDiscoveredEvent:#DmTools.Actors";
    }

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        jsh.SerializeComposite("FwVersion", m_fwVersion);
        jsh.SerializeComposite("FwTimestamp", m_fwTimestamp);
    }
};

// =================================================================================== //
// ClientConnectedEvent

class ClientConnectedEvent final : public HostManagerEventBase
{
public:
    /*
    newClientIPSet - a set of already connected users
    newClient - the client that was connected or diconnected
    connected - true if newClient was connected and false if was disconnected
    */
    ClientConnectedEvent(const std::set<std::string> & newClientIPSet, const std::string & newClient) :
        HostManagerEventBase(""),
        m_ExistingClientIPSet(newClientIPSet),
        m_newClient(newClient)
    {}

private:
    const std::set<std::string> m_ExistingClientIPSet;
    const std::string m_newClient;
    bool m_connected;

    const char* GetTypeName() const override
    {
        return "ClientConnectedEvent:#DmTools.Actors";
    }

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        jsh.Serialize("NewConnectedUser", m_newClient);
        jsh.SerializeStringArray("ExistingConnectedUsers", m_ExistingClientIPSet);
    }
};

class ClientDisconnectedEvent final : public HostManagerEventBase
{
public:
    /*
    newClientIPSet - a set of already connected users
    newClient - the client that was diconnected
    */
    ClientDisconnectedEvent(const std::set<std::string> & newClientIPSet, const std::string & newClient) :
        HostManagerEventBase(""),
        m_ExistingClientIPSet(newClientIPSet),
        m_newClient(newClient)
    {}

private:
    const std::set<std::string> m_ExistingClientIPSet;
    const std::string & m_newClient;
    bool m_connected;

    const char* GetTypeName() const override
    {
        return "ClientDisconnectedEvent:#DmTools.Actors";
    }

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        jsh.Serialize("DisconnectedUser", m_newClient);
        jsh.SerializeStringArray("ExistingConnectedUsers", m_ExistingClientIPSet);
    }
};

//********************************** New Log Lines event **********************************************//
class SerializableLogLineParams final : public JsonSerializable
{
public:
    explicit SerializableLogLineParams(const std::vector<unsigned> & logLineParams) : m_logLineParams(logLineParams) {}

private:
    std::vector<unsigned> m_logLineParams;

    void ToJson(ostream& os) const override
    {
        JsonSerializeHelper jsh(os);
        int i = 0;
        for (auto param : m_logLineParams)
        {
            i++;
            std::stringstream os;
            os << "Parameter" << i;
            jsh.Serialize(os.str().c_str(), param);
        }
    }
};

class SerializableLogLine final : public log_collector::RawLogLine, public JsonSerializable
{
public:
    explicit SerializableLogLine(RawLogLine logLine) : RawLogLine(logLine) {}

private:

    void ToJson(ostream& os) const override
    {
        JsonSerializeHelper jsh(os);
        jsh.Serialize("Module", m_module);
        jsh.Serialize("Level", m_level);
        jsh.Serialize("StringOffset", m_strOffset);
        jsh.Serialize("Signature", m_signature);
        jsh.Serialize("IsString", m_isString);
        jsh.Serialize("NumberOfParameters", m_params.size());
        jsh.SerializeStringArray("Parameters", m_params);
        jsh.Serialize("MLLR", m_missingLogsReason);
        jsh.Serialize("NOMLLS", m_numOfMissingLogDwords);
    }
};

class SerializableLogLinesList final : public JsonSerializable
{
public:
    explicit SerializableLogLinesList(const std::vector<log_collector::RawLogLine> & logLines)
    {
        m_logLines.reserve(logLines.size());
        for (const auto& logLine : logLines)
        {
            m_logLines.push_back(SerializableLogLine(logLine));
        }
    }

private:
    std::vector<SerializableLogLine> m_logLines;

    void ToJson(ostream& os) const override
    {
        JsonSerializeHelper jsh(os);
        int i = 0;
        for (const SerializableLogLine & logLine : m_logLines)
        {
            i++;
            std::stringstream os;
            os << "LogLine" << i;
            jsh.SerializeComposite(os.str().c_str(), logLine);
        }
    }
};

class NewLogsEvent final : public HostManagerEventBase
{
public:
    NewLogsEvent(const std::string& deviceName, const CpuType& cpuType, const std::vector<log_collector::RawLogLine>& logLines) :
        HostManagerEventBase(deviceName),
        m_cpuType(cpuType)
    {
        m_logLines.reserve(logLines.size());
        for (const auto& logLine : logLines)
        {
            m_logLines.emplace_back(new SerializableLogLine(logLine));
        }
    }

private:
    CpuType m_cpuType;
    std::vector<std::unique_ptr<JsonSerializable>> m_logLines;

    const char* GetTypeName() const override
    {
        return "NewLogsEvent:#DmTools.Actors";
    }

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        jsh.Serialize("TracerType", CPU_TYPE_TO_STRING[m_cpuType]);
        jsh.SerializeCompositeArray("LogLines", m_logLines);
    }
};
//********************************** END - New Log Lines event **********************************************//

// =================================================================================== //
// DriverEvent

// Event to be sent upon recieval of driver event
class DriverEvent final : public HostManagerEventBase
{
public:
    DriverEvent(const std::string& deviceName,
        int driverEventType,
        unsigned driverEventId,
        unsigned listenId,
        unsigned bufferLength,
        const unsigned char* binaryData) :
            HostManagerEventBase(deviceName),
            m_driverEventType(driverEventType),
            m_driverEventId(driverEventId),
            m_listenId(listenId),
            m_bufferLength(bufferLength),
            m_binaryData(binaryData)
    {}

private:
    const int m_driverEventType;
    const unsigned m_driverEventId;
    const unsigned m_listenId;
    const unsigned m_bufferLength;
    const unsigned char* m_binaryData;

    const char* GetTypeName() const override
    {
        return "DriverEvent:#DmTools.Actors";
    }

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        jsh.Serialize("DriverEventType", m_driverEventType);
        jsh.Serialize("DriverEventId", m_driverEventId);
        jsh.Serialize("ListenId", m_listenId);
        jsh.Serialize("BinaryDataBase64", JsonSerializeHelper::Base64Encode(m_binaryData, m_bufferLength)); // bufferLength = 0 if buffer is empty
    }
};

// =================================================================================== //

#endif // _11AD_EVENTS_DEFINITIONS_H_

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
#include "JsonSerializeHelper.h"

// Base class for all events to be pushed to DMTools
class HostManagerEventBase
{
public:
    HostManagerEventBase(const std::string& deviceName) :
        m_timestampLocal(std::move(Utils::GetCurrentLocalTime())),
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


// === NewDeviceDiscoveredEvent:

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

    const char* GetTypeName() const override
    {
        return "NewDeviceDiscoveredEvent:#DmTools.Actors";
    }

private:
    SerializableFwVersion m_fwVersion;
    SerializableFwTimestamp m_fwTimestamp;

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        jsh.SerializeComposite("FwVersion", m_fwVersion);
        jsh.SerializeComposite("FwTimestamp", m_fwTimestamp);
    }
};

//serialize the list of connected clients
class SerializableConnectedUserList final : public JsonSerializable
{
public:
    explicit SerializableConnectedUserList(const std::set<std::string> & newClientIPSet) : m_newClientIPSet(newClientIPSet)
    {}
private:
    const std::set<std::string> & m_newClientIPSet;
    void ToJson(ostream& os) const override
    {
        JsonSerializeHelper jsh(os);
        int i = 0;
        for (const std::string & client : m_newClientIPSet)
        {
            i++;
            std::stringstream os;
            os << "ConnectedClientIP" << i;
            jsh.Serialize(os.str().c_str(), client);
        }

    }

};


class ClientConnectionEvent final : public HostManagerEventBase
{
public:
    /*
    newClientIPSet - a set of already connected users
    newClient - the client that was connected or diconnected
    connected - true if newClient was connected and false if was disconnected
    */
    ClientConnectionEvent(const std::set<std::string> & newClientIPSet, const std::string & newClient, bool connected) :
        HostManagerEventBase(""),
        m_serializableNewClientIPSet(newClientIPSet),
        m_newClient(newClient),
        m_connected(connected)
    {}

    const char* GetTypeName() const override
    {
        return "ClientConnectionEvent:#DmTools.Actors";
    }

private:
    SerializableConnectedUserList m_serializableNewClientIPSet;
    //const std::set<std::string> & m_newClientIPSet;
    const std::string & m_newClient;
    bool m_connected;

    void ToJsonInternal(JsonSerializeHelper& jsh) const override
    {
        if (m_connected)
        {
            jsh.Serialize("NewConnectedUser", m_newClient);
        }
        else
        {
            jsh.Serialize("DisconnectedUser", m_newClient);
        }

        jsh.SerializeComposite("OtherConnectedUsers", m_serializableNewClientIPSet);

    }

};


#endif // _11AD_EVENTS_DEFINITIONS_H_

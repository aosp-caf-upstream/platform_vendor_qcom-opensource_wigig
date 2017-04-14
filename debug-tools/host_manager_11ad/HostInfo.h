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

#ifndef _HOSTINFO_H_
#define _HOSTINFO_H_

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <set>

#include "HostDefinitions.h"

using namespace std;

class HostInfo
{

public:

    /*
    HostInfo
    Initializes all hostInfo members (ips and alias)
    */
    HostInfo();

    /*
    GetIps
    Returns host's Ips (private and broadcast)
    @param: none
    @return: host's ips
    */
    const HostIps& GetIps();

    /*
    GetHostAlias
    Returns host's alias (if one was given by the user)
    @param: none
    @return: host's alias if exists, otherwise empty string
    */
    string GetAlias();

    /*
    SaveAliasToFile
    Gets a new alias for the host and saves it to a configuration file.
    This function is called from CommandsTcpServer only. It turns on a flag so that the UDP server (that is on a different thread) will check for the new alias
    @param: string - the new alias
    @return: bool - operation status - true for success, false otherwise
    */
    bool SaveAliasToFile(string newAlias);

    /*
    UpdateAliasFromFile
    Updates the m_alias member with the host's alias from 11ad's persistency
    @param: none
    @return: bool - operation status - true for success, false otherwise
    */
    bool UpdateAliasFromFile();

    /*
    GetConnectedUsers
    Returns a set of all the users that are connected to this host
    @param: none
    @return: set<string> of all connected users
    */
    const set<string>& GetConnectedUsers() { return m_connectedUsers;  }

    /*
    AddNewConnectedUser
    Adds a user to the connected user's list
    @param: string - a new user (currently the user's DmTools's IP. TODO: change to the user's personal host's name or user's DmTools username)
    @return: none
    */
    void AddNewConnectedUser(string user) { m_connectedUsers.insert(user); }

    /*
    RemoveConnectedUser
    Removes a user from the connected user's list
    @param: string - a user (currently the user's DmTools's IP. TODO: change to the user's personal host's name or user's DmTools username)
    @return: none
    */
    void RemoveNewConnectedUser(string user) { m_connectedUsers.erase(user); }

    string GetVersion() { return m_version;  }


private:
    HostIps m_ips; // host's network details // assumption: each host has only one IP address for ethernet interfaces
    string m_alias; // host's alias (given by user)
    const string m_persistencyPath; // host server's files location
    const string m_aliasFileName; // host's alias file
    const string m_oldHostAliasFile; // old location of the host alias
    set<string> m_connectedUsers; // list of users IPs that have a connection to the commandsTcpServer // TODO: change to the user's personal host's name or user's DmTools username
    string m_version; // host_manager_11ad version // TODO: update m_version
    atomic<bool> m_isAliasFileChanged; // when turned on indicates that m_alias contains stale information, so we need to update it from persistency
    mutex m_persistencyLock; // only one thread is allowed to change persistency at a time

    // load host's info from persistency and ioctls
    void LoadHostInfo();
};


#endif
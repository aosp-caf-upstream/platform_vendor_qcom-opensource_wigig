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

#include <iostream>
#include "HostInfo.h"
#include "DebugLogger.h"
#include "FileSystemOsAbstraction.h"
#include "VersionInfo.h"

// Version display is constructed using defines from VersionInfo.h which is updated automatically or manually during release
// Note: cannot use Windows specific GetFileVersionInfo to extract the version
const string HostInfo::s_version = TOOL_VERSION_STR;

HostInfo::HostInfo() :
    m_alias(""),
    m_persistencyPath(FileSystemOsAbstraction::GetConfigurationFilesLocation()),
    m_aliasFileName(FileSystemOsAbstraction::GetDirectoriesDilimeter() + "host_alias"),
    m_oldHostAliasFile(GetOldPersistencyLocation() + "host_manager_11ad_host_info"),
    m_isAliasFileChanged(false),
    m_capabilitiesMask(0)
{
    SetHostCapabilities();
    LoadHostInfo();
}

const string HostInfo::GetOldPersistencyLocation()
{
#ifdef __linux
    return "/etc/";
#elif __OS3__
    return "/tmp/";
#else
    return "C:\\Temp\\"; // TODO: change
#endif // __linux
}

const HostIps& HostInfo::GetIps()
{
    return m_ips;
}

string HostInfo::GetAlias()
{
    if (m_isAliasFileChanged)
    {
        UpdateAliasFromFile();
    }
    return m_alias;
}

bool HostInfo::SaveAliasToFile(const string& newAlias)
{
    lock_guard<mutex> lock(m_persistencyLock);
    if (!FileSystemOsAbstraction::WriteFile(m_persistencyPath + m_aliasFileName, newAlias))
    {
        LOG_WARNING << "Failed to write new alias to configuration file " << m_persistencyPath << m_aliasFileName << endl;
        return false;
    }
    m_isAliasFileChanged = true;
    return true;
}

bool HostInfo::UpdateAliasFromFile()
{
    lock_guard<mutex> lock(m_persistencyLock);
    if (!FileSystemOsAbstraction::ReadFile(m_persistencyPath + m_aliasFileName, m_alias))
    {
        LOG_WARNING << "Failed to write new alias to configuration file " << m_persistencyPath << m_aliasFileName << endl;
        return false;
    }
    m_isAliasFileChanged = false;
    return true;
}

set<string> HostInfo::GetConnectedUsers() const
{
    lock_guard<mutex> lock(m_connectedUsersLock);
    return m_connectedUsers;
}

void HostInfo::AddNewConnectedUser(const string& user)
{
    lock_guard<mutex> lock(m_connectedUsersLock);
    m_connectedUsers.insert(user);
}

void HostInfo::RemoveConnectedUser(const string& user)
{
    lock_guard<mutex> lock(m_connectedUsersLock);
    m_connectedUsers.erase(user);
}

void HostInfo::SetHostCapabilities()
{
    //SetCapability(COLLECTING_LOGS, true); // TODO - reenable
}

void HostInfo::SetCapability(CAPABILITIES capability, bool isTrue)
{
    const DWORD mask = (DWORD)1 << capability;
    m_capabilitiesMask = isTrue ? m_capabilitiesMask | mask : m_capabilitiesMask & ~mask;
}

bool HostInfo::IsCapabilitySet(CAPABILITIES capability) const
{
    return (m_capabilitiesMask & (DWORD)1 << capability) != (DWORD)0;
}

void HostInfo::LoadHostInfo()
{
    m_ips = FileSystemOsAbstraction::GetHostIps();

    // create host manager directory if doesn't exist
    bool res = FileSystemOsAbstraction::DoesFolderExist(m_persistencyPath);
    if (!res)
    {
        res = FileSystemOsAbstraction::CreateFolder(m_persistencyPath);
        if (!res)
        {
            LOG_WARNING << "Failed to create " << m_persistencyPath << " directory" << endl;
            return;
        }
        // backward compatibility - copy the alias that the user already given to its new place
        if (FileSystemOsAbstraction::IsFileExists(m_oldHostAliasFile)
            && !FileSystemOsAbstraction::MoveFileToNewLocation(m_oldHostAliasFile, m_persistencyPath + m_aliasFileName))
        {
            LOG_WARNING << "Failed to move " << m_oldHostAliasFile << " file to new location " << m_persistencyPath + m_aliasFileName << endl;
            return;
        }
    }

    res = FileSystemOsAbstraction::ReadFile(m_persistencyPath + m_aliasFileName, m_alias);
    if (!res) // file doesn't exist
    {
        res = FileSystemOsAbstraction::ReadHostOsAlias(m_alias);
        if (!res)
        {
            LOG_WARNING << "Failed to read OS host name" << endl;
            m_alias = "";
        }
        res = FileSystemOsAbstraction::WriteFile(m_persistencyPath + m_aliasFileName, m_alias);
        if (!res)
        {
            LOG_WARNING << "Failed to write host alias to persistency" << endl;
        }
    }
}

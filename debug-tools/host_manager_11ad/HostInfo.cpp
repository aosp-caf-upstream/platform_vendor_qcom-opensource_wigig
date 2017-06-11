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

#include "HostInfo.h"
#include "DebugLogger.h"
#include "UdpTempOsAbstruction.h"
#include <iostream>

const string HostInfo::s_version = "1.1.2";

HostInfo::HostInfo() :
    m_alias(""),
    m_persistencyPath(UdpTempOsAbstruction::GetPersistencyLocation() + "host_manager_11ad"),
    m_aliasFileName(UdpTempOsAbstruction::GetDirectoriesDilimeter() + "host_alias"),
    m_oldHostAliasFile(UdpTempOsAbstruction::GetPersistencyLocation() + "wigig_remoteserver_details"),
    m_isAliasFileChanged(false)
{
    LoadHostInfo();
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

bool HostInfo::SaveAliasToFile(string newAlias)
{
    m_persistencyLock.lock();
    if (!UdpTempOsAbstruction::WriteFile(m_persistencyPath + m_aliasFileName, newAlias))
    {
        LOG_WARNING << "Failed to write new alias to configuration file " << m_persistencyPath << m_aliasFileName << endl;
        return false;
    }
    m_isAliasFileChanged = true;
    m_persistencyLock.unlock();
    return true;
}

bool HostInfo::UpdateAliasFromFile()
{
    m_persistencyLock.lock();
    if (!UdpTempOsAbstruction::ReadFile(m_persistencyPath + m_aliasFileName, m_alias))
    {
        LOG_WARNING << "Failed to write new alias to configuration file " << m_persistencyPath << m_aliasFileName << endl;
        return false;
    }
    m_isAliasFileChanged = false;
    m_persistencyLock.unlock();
    return true;
}


//void HostInfo::LoadHostInfo()
//{
//    m_ips = UdpTempOsAbstruction::GetHostIps();
//
//    bool res = UdpTempOsAbstruction::ReadFile(m_persistencyPath, m_alias);
//    if (!res) // file doesn't exist
//    {
//        /*
//        res = UdpTempOsAbstruction::CreateFile(m_persistencyPath)
//        if (!res)
//        {
//            cout << "Failed to create persistency file" << endl;
//            m_alias = "";
//            return;
//        }
//        */
//        res = UdpTempOsAbstruction::ReadHostOsAlias(m_alias);
//        if (!res)
//        {
//            cout << "Failed to read OS host name" << endl;
//            m_alias = "";
//        }
//        res = UdpTempOsAbstruction::WriteFile(m_persistencyPath, m_alias);
//        if (!res)
//        {
//            cout << "Failed to write host alias to persistency" << endl;
//        }
//    }
//}

void HostInfo::LoadHostInfo()
{
    m_ips = UdpTempOsAbstruction::GetHostIps();

    // create host manager directory if doesn't exist
    bool res = UdpTempOsAbstruction::IsFolderExists(m_persistencyPath);
    if (!res)
    {
        res = UdpTempOsAbstruction::CreateFolder(m_persistencyPath);
        if (!res)
        {
            LOG_WARNING << "Failed to create " << m_persistencyPath << " directory" << endl;
            return;
        }
        // backward compatibility - copy the alias that the user already given to its new place
        if (UdpTempOsAbstruction::IsFileExists(m_oldHostAliasFile))
        {
            UdpTempOsAbstruction::MoveFileToNewLocation(m_oldHostAliasFile, m_persistencyPath + m_aliasFileName);
        }
    }

    res = UdpTempOsAbstruction::ReadFile(m_persistencyPath + m_aliasFileName, m_alias);
    if (!res) // file doesn't exist
    {
        res = UdpTempOsAbstruction::ReadHostOsAlias(m_alias);
        if (!res)
        {
            LOG_WARNING << "Failed to read OS host name" << endl;
            m_alias = "";
        }
        res = UdpTempOsAbstruction::WriteFile(m_persistencyPath + m_aliasFileName, m_alias);
        if (!res)
        {
            LOG_WARNING << "Failed to write host alias to persistency" << endl;
        }
    }
}

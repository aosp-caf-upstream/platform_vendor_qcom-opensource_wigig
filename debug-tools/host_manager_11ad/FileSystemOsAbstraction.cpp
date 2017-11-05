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

#ifdef __linux
#include <sys/ioctl.h> // for struct ifreq
#include <net/if.h> // for struct ifreq
#include <arpa/inet.h> // for the declaration of inet_ntoa
#include <netinet/in.h> // for struct sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <cerrno>
#include <sys/stat.h> //added for config- template path
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

#elif _WINDOWS
#include <windows.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <comutil.h> //for _bstr_t (used in the string conversion)
#pragma comment(lib, "comsuppw")

#else
#include <dirent.h>
#endif

#ifdef __ANDROID__
#include <sys/stat.h> //added for config- template path
#include <unistd.h> //added for config- template path
#include <sys/types.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include "DebugLogger.h"
#include "FileSystemOsAbstraction.h"

const std::string FileSystemOsAbstraction::LOCAL_HOST_IP = "127.0.0.1";

bool FileSystemOsAbstraction::FindEthernetInterface(struct ifreq& ifr, int& fd)
{
#ifdef __linux
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        LOG_WARNING << "Failed to get host's IP address and broadcast IP address" << std::endl;
        return false;
    }

    for (int i = 0; i < 100; i++)
    {
        snprintf(ifr.ifr_name, IFNAMSIZ - 1, "eth%d", i);

        if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0)
        {
            return true;
        }
    }
#endif
    return false;
}

HostIps FileSystemOsAbstraction::GetHostIps()
{
#ifdef __linux
    HostIps hostIps;
    int fd;
    struct ifreq ifr;

    ifr.ifr_addr.sa_family = AF_INET; // IP4V

                                      // Get IP address according to OS
    if (FindEthernetInterface(ifr, fd))
    {
        LOG_INFO << "Linux OS" << std::endl;
    }
    else
    {
        snprintf(ifr.ifr_name, IFNAMSIZ - 1, "br-lan");
        if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0)
        {
            LOG_INFO << "OpenWRT OS" << std::endl;
        }
        else
        {
            // Probably Android OS
            LOG_INFO << "Android OS (no external IP Adress)" << std::endl;
            hostIps.m_ip = FileSystemOsAbstraction::LOCAL_HOST_IP;
            hostIps.m_broadcastIp = FileSystemOsAbstraction::LOCAL_HOST_IP;
            return hostIps;
        }
    }

    hostIps.m_ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

    if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0)
    {
        LOG_WARNING << "Failed to get broadcast IP" << std::endl;
        return hostIps;
    }
    hostIps.m_broadcastIp = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    LOG_DEBUG << "Host's IP address is " << hostIps.m_ip << std::endl;
    LOG_DEBUG << "Broadcast IP address is " << hostIps.m_broadcastIp << std::endl;

    close(fd);
    return hostIps;
#else
    HostIps empty;
    empty.m_broadcastIp = "10.18.172.155";
    return empty;
#endif
}

bool FileSystemOsAbstraction::ReadFile(string fileName, string& data)
{
    ifstream fd(fileName.c_str());
    if (!fd.good()) // file doesn't exist
    {
        data = "";
        return false;
    }

    fd.open(fileName.c_str());
    stringstream content;
    content << fd.rdbuf();
    fd.close();
    data = content.str();
    return true;
}

bool FileSystemOsAbstraction::WriteFile(const string& fileName, const string& content)
{
    std::ofstream fd(fileName.c_str());
    if (!fd.is_open())
    {
        LOG_WARNING << "Failed to open file: " << fileName << std::endl;
        return false;
    }
    fd << content;
    if (fd.bad())
    {
        LOG_WARNING << "Failed to write to file: " << fileName << std::endl;
        fd.close();
        return false;
    }
    fd.close();
    return true;
}

bool FileExist(const std::string& Name)
{
#ifdef _WINDOWS
    struct _stat buf;
    int Result = _stat(Name.c_str(), &buf);
#else
    struct stat buf;
    int Result = stat(Name.c_str(), &buf);
#endif
    return Result == 0;
}



string FileSystemOsAbstraction::GetConfigurationFilesLocation()
{
    stringstream path;

    //should check __ANDROID__ first since __LINUX flag also set in Android
#ifdef __ANDROID__
    std::string t_path = "/data/vendor/wifi/host_manager_11ad/";
    if (!FileExist(t_path))
    {
        path << "/data/host_manager_11ad/";
    }
    else
    {
        path << t_path;
    }
#elif __linux
    return "/etc/host_manager_11ad/";
#elif _WINDOWS //windows
    LPWSTR lpwstrPath = NULL;
    // Get the ProgramData folder path of windows
    HRESULT result = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &lpwstrPath);
    if (SUCCEEDED(result))
    {
        // Convert the path to string
        std::wstring wpath(lpwstrPath);
        std::string strPath = std::string(wpath.cbegin(), wpath.cend());
        CoTaskMemFree(lpwstrPath);
        path << strPath << "\\Wilocity\\host_manager_11ad\\";
    }
#else //OS3
    return "/etc/host_manager_11ad/";
#endif // __linux
    return path.str();
}

std::string FileSystemOsAbstraction::GetTemplateFilesLocation()
{
    stringstream path;

    //should check __ANDROID__ first since __LINUX flag also set in Android


#ifdef  __ANDROID__
    std::string t_path = "/vendor/etc/wifi/host_manager_11ad/";
    if (!FileExist(t_path))
    {
        path << "/data/host_manager_11ad/";
    }
    else
    {
        path << t_path;
    }

#elif __linux
    return "/etc/host_manager_11ad/";
#elif _WINDOWS
    path << "..\\OnTargetUI\\";
#else //OS3
    return "/etc/host_manager_11ad/";
#endif // __linux
    return path.str();
}

string FileSystemOsAbstraction::GetDirectoriesDilimeter()
{
#ifndef _WINDOWS
    return "/";
#else
    return "\\";
#endif
}

bool FileSystemOsAbstraction::ReadHostOsAlias(string& alias)
{
#ifdef __linux
    if (!ReadFile("/etc/hostname", alias))
    {
        alias = "";
        return false;
    }
    return true;
#else
    alias = "";
    return false;
#endif // __linux
}

bool FileSystemOsAbstraction::DoesFolderExist(string path)
{
#ifndef _WINDOWS
    DIR* pDir = opendir(path.c_str());
    if (pDir != NULL)
    {
        (void)closedir(pDir);
        return true;
    }
    return false;
#else
    DWORD fileAttributes = GetFileAttributesA(path.c_str());
    if (INVALID_FILE_ATTRIBUTES == fileAttributes) // no such path
    {
        return false;
    }
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;   // given path is a directory
    }
    return false;    // given path isn't a directory
#endif
}

bool FileSystemOsAbstraction::IsFileExists(string path)
{
    ifstream f(path.c_str());
    return f.good();
}

bool FileSystemOsAbstraction::CreateFolder(string path)
{
#ifndef _WINDOWS
    system(("mkdir " + path).c_str());
    return true;
#else
    std::wstring wpath = std::wstring(path.cbegin(), path.cend());
    return CreateDirectory(wpath.c_str(), nullptr) == TRUE;
#endif
}

bool FileSystemOsAbstraction::MoveFileToNewLocation(string oldFileLocation, string newFileLocation)
{
#ifndef _WINDOWS
    system(("mv " + oldFileLocation + " " + newFileLocation).c_str());
    return true;
#else
    std::wstring wOldPath = std::wstring(oldFileLocation.cbegin(), oldFileLocation.cend());
    std::wstring wNewPath = std::wstring(newFileLocation.cbegin(), newFileLocation.cend());
    return MoveFile(wOldPath.c_str(), wNewPath.c_str()) == TRUE;
#endif
}
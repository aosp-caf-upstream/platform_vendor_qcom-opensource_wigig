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

#include <stdlib.h>
#include <string.h>
#include <cerrno>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

#else
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include "DebugLogger.h"
#include "UdpTempOsAbstruction.h"

const std::string UdpTempOsAbstruction::LOCAL_HOST_IP = "127.0.0.1";

bool UdpTempOsAbstruction::FindEthernetInterface(struct ifreq& ifr, int& fd)
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

HostIps UdpTempOsAbstruction::GetHostIps()
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
            hostIps.m_ip = UdpTempOsAbstruction::LOCAL_HOST_IP;
            hostIps.m_broadcastIp = UdpTempOsAbstruction::LOCAL_HOST_IP;
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

bool UdpTempOsAbstruction::ReadFile(string fileName, string& data)
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

bool UdpTempOsAbstruction::WriteFile(string fileName, string content)
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

string UdpTempOsAbstruction::GetPersistencyLocation()
{
#ifdef __linux
    return "/etc/";
#else
    return "C:\\Temp\\"; // TODO: change
#endif // __linux
}

string UdpTempOsAbstruction::GetDirectoriesDilimeter()
{
#ifdef __linux
    return "/";
#else
    return "\\";
#endif
}

bool UdpTempOsAbstruction::ReadHostOsAlias(string& alias)
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

bool UdpTempOsAbstruction::IsFolderExists(string path)
{
#ifdef __linux
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

bool UdpTempOsAbstruction::IsFileExists(string path)
{
    ifstream f(path.c_str());
    return f.good();
}

bool UdpTempOsAbstruction::CreateFolder(string path)
{
#ifdef __linux
    system(("mkdir " + path).c_str());
    return true;
#else
    // not Implemented yet
    return false;
#endif
}

void UdpTempOsAbstruction::MoveFileToNewLocation(string oldFileLocation, string newFileLocation)
{
#ifdef __linux
    system(("mv " + oldFileLocation + " " + newFileLocation).c_str());
#else
    // not Implemented yet
#endif
}

UdpSocket::UdpSocket(string broadcastIp, int portIn, int portOut)
{
    m_portIn = portIn;
    m_portOut = portOut;
    m_broadcastIp = broadcastIp;
#ifdef __linux

    // create UDP socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket < 0)
    {
        string error = "Can't open UDP socket";
        LOG_WARNING << error << endl;
        throw error;
    }

    // set broadcast flag on
    int enabled = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, (char*)&enabled, sizeof(enabled)) < 0)
    {
        string error = "Can't set broadcast option for udp socket, error: ";
        error += strerror(errno);

        LOG_WARNING << error << endl;
        shutdown(m_socket, SHUT_RDWR);
        throw error;
    }

    // bind socket to portIn
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_portIn);
    if (::bind(m_socket, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
    {
        string error = "Can't bind socket to port, error: ";
        error += strerror(errno);

        LOG_WARNING << error << std::endl;
        throw error;
    }
#else
    string error = "UDP socket is supported on linux OS only";
    throw error;
#endif
}

int UdpSocket::Send(string message)
{
#ifdef __linux
    struct sockaddr_in dstAddress;
    dstAddress.sin_family = AF_INET;
    inet_pton(AF_INET, m_broadcastIp.c_str(), &dstAddress.sin_addr.s_addr);
    dstAddress.sin_port = htons(m_portOut);
    int messageSize = message.length() * sizeof(char);
    int result = sendto(m_socket, message.c_str(), messageSize, 0, (sockaddr*)&dstAddress, sizeof(dstAddress));
    LOG_VERBOSE << "INFO : sendto with sock_out=" << m_socket << ", message=" << message << " messageSize=" << messageSize << " returned with " << result << std::endl;
    if (result < 0)
    {
        LOG_WARNING << "ERROR : Cannot send udp broadcast message, error " << ": " << strerror(errno) << std::endl;
        return 0;
    }
    return messageSize;
#else
    return 0;
#endif
}

const char* UdpSocket::Receive(int len)
{
#ifdef __linux
    char* buf = new char[len];
    if (recvfrom(m_socket, buf, len, 0, NULL, 0) < 0)
    {
        LOG_WARNING << "Can't receive from port " << m_portIn << std::endl;
        return "";
    }
    return buf;
#else
    return "";
#endif
}

void UdpSocket::Close()
{
#ifdef __linux
    shutdown(m_socket, SHUT_RDWR);
#else
#endif
}

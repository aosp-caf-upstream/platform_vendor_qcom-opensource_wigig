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

#include "NetworkInterface.h"
#include "DebugLogger.h"

#include <cerrno>

using namespace NetworkInterfaces;

NetworkInterface::NetworkInterface()
{
#ifdef _WINDOWS
    WSADATA wsa;

    if (-1 == WSAStartup(MAKEWORD(2, 0), &wsa))
    {
        LOG_ERROR << "Cannot initialize network library" << std::endl;
        exit(1);
    }
#endif

    m_fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fileDescriptor < 0)
    {
        LOG_ERROR << "Cannot create socket file descriptor: " << strerror(errno) << std::endl;
        exit(1);
    }

    char optval = 1;
    setsockopt(m_fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    m_localAddress.sin_family = AF_INET;
    m_bufferSize = 0;
    m_buffer = NULL;
}

NetworkInterface::NetworkInterface(int fileDescriptor)
    : m_fileDescriptor(fileDescriptor)
    , m_buffer(NULL)
    , m_bufferSize(0)
{
}

// Establish Connections
void NetworkInterface::Bind(int port)
{
    m_localAddress.sin_port = htons(port);
    m_localAddress.sin_addr.s_addr = INADDR_ANY;
    memset(m_localAddress.sin_zero, '\0', sizeof m_localAddress.sin_zero);

    if (-1 == bind(m_fileDescriptor, (struct sockaddr*)&m_localAddress, sizeof(m_localAddress)))
    {
        LOG_ERROR << "Cannot bind listener to port " << port << ": " << strerror(errno) << std::endl;
        LOG_ERROR << "Please verify if another application instance is running" << std::endl;
        exit(1);
    }
}

void NetworkInterface::Listen(int backlog)
{
    if (-1 == listen(m_fileDescriptor, backlog))
    {
        LOG_ERROR << "Cannot listen to the socket: " << strerror(errno) << std::endl;
        LOG_ERROR << "Please verify if another application instance is running" << std::endl;
        exit(1);
    }
}

NetworkInterface NetworkInterface::Accept()
{
    struct sockaddr_in remoteAddress;
    socklen_t size = sizeof(remoteAddress);

    int fileDescriptor = accept(m_fileDescriptor, (struct sockaddr*)&remoteAddress, &size);
    if (fileDescriptor <= 0)
    {
        LOG_ERROR << "Cannot accept incoming connection: " << strerror(errno) << std::endl;
        exit(1);
    }

    return NetworkInterface(fileDescriptor);
}

// Send and Receive
int NetworkInterface::Send(const std::string& data)
{
    return send(m_fileDescriptor, data.c_str(), data.size(), 0);
}

int NetworkInterface::Send(const char* data)
{
    return send(m_fileDescriptor, data, strlen(data), 0);
}

const char* NetworkInterface::Receive(int size, int flags)
{
    if (m_bufferSize <= size + 1)
    {
        m_buffer = (char*)(realloc(m_buffer, sizeof(char) * (size + 1)));
        if (!m_buffer) return NULL;
        m_bufferSize = size;
    }

    int bytesReceived = recv(m_fileDescriptor, m_buffer, size, flags);
    if (0 > bytesReceived) return NULL;
    m_buffer[bytesReceived] = '\0';

    return m_buffer;
}

// Socket Closing Functions
void NetworkInterface::Close()
{
#ifdef _WINDOWS
    WSACleanup();
    closesocket(m_fileDescriptor);
#else
    close(m_fileDescriptor);
#endif
}

void NetworkInterface::Shutdown(int type)
{
#ifdef _WINDOWS
    WSACleanup();
#endif

    shutdown(m_fileDescriptor, type);
}

const char* NetworkInterface::GetPeerName() const
{
    struct sockaddr_in remoteAddress;
    socklen_t size = sizeof(remoteAddress);

    if (-1 == getpeername(m_fileDescriptor, (struct sockaddr*)&remoteAddress, &size))
    {
        LOG_ERROR << "Failure in getpeername" << std::endl;
        exit(1);
    }

    return inet_ntoa(remoteAddress.sin_addr);
}

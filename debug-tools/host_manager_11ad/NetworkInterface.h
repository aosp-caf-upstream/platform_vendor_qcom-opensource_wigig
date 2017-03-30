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

#ifndef _SOCKET_H_
#define _SOCKET_H_

#ifdef _WINDOWS
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>

#ifdef _WINDOWS
#include <winsock.h>
#elif __linux
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#else

#endif

#ifdef _WINDOWS
typedef int socklen_t;
#endif


namespace NetworkInterfaces
{
    class NetworkInterface
    {
    private:

        int m_fileDescriptor;
        struct sockaddr_in m_localAddress;
        char* m_buffer;
        int m_bufferSize;

    public:

        NetworkInterface();
        explicit NetworkInterface(int sockfd);

        // Establish Connection
        void Bind(int portNumber);
        void Listen(int backlog = 5);
        NetworkInterface Accept();

        // Send and Receive
        int Send(const char* data);
        int Send(const std::string& data);
        const char* Receive(int size = 1024, int flags = 0);

        // Terminate Connection
        void Close();
        void Shutdown(int type);

        // Addresses
        const char* GetPeerName() const;
    };

}

#endif // !_NETWORK_INTERFACE_H_

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

#include "UdpServer.h"
#include "UdpTempOsAbstruction.h"
#include "CommandsHandler.h"
#include "Host.h"

const int UdpServer::m_maxMessageLength = 1024;

UdpServer::UdpServer(unsigned int udpPortIn, unsigned int udpPortOut, Host& host) :
    m_udpPortIn(udpPortIn),
    m_udpPortOut(udpPortOut),
    m_broadcastIp(host.GetHostInfo().GetIps().m_broadcastIp),
    m_CommandHandler(stUdp, host)
{
    try
    {
        m_pSocket.reset(new UdpSocket(m_broadcastIp, udpPortIn, udpPortOut));
    }
    catch (string error)
    {
        LOG_WARNING << "Failed to open UDP socket: " << error << endl;
        m_pSocket.reset();
    }
}

void UdpServer::StartServer()
{
    if (UdpTempOsAbstruction::LOCAL_HOST_IP == m_broadcastIp)
    {
        LOG_WARNING << "Can't start UDP server due to invalid host's IP/ broadcast IP";
        return;
    }

    if (m_pSocket)
    {
        LOG_DEBUG << "Start UDP server on local port " << m_udpPortIn << std::endl;
        LOG_DEBUG << "Broadcast messages are sent to port " << m_udpPortOut << std::endl;
        BlockingReceive();
    }
}

void UdpServer::Stop()
{
    m_pSocket.reset();
}

void UdpServer::BlockingReceive()
{
    do
    {
        const char* incomingMessage = m_pSocket->Receive(m_maxMessageLength);
        LOG_VERBOSE << "Got Udp message: " << incomingMessage << endl;
        ResponseMessage referencedResponse = { "", REPLY_TYPE_NONE, 0 };
        m_CommandHandler.ExecuteCommand(incomingMessage, referencedResponse);
        if (referencedResponse.length > 0)
        {
            LOG_VERBOSE << "Send broadcast message" << endl;
            SendBroadcastMessage(referencedResponse);
        }

    } while (true);
}

void UdpServer::SendBroadcastMessage(ResponseMessage responseMessage)
{
    m_pSocket->Send(responseMessage.message);
}
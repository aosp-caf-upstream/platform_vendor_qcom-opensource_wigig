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

#ifndef _HOST_H_
#define _HOST_H_

#include <memory>
#include <future>
#include "EventsDefinitions.h"
#include "CommandsTcpServer.h"
#include "EventsTcpServer.h"
#include "CommandsHandler.h"
#include "UdpServer.h"
#include "HostInfo.h"
#include "DeviceManager.h"

/*
 * Host is a class that holds the TCP and UDP server.
 * It also holds the Device Manager.
 */
class Host
{
public:

    /*
    * Host is s singletone due to HandleOsSignals - signal is a global function that can't get a pointer to the host as an argument
    */
    static Host& GetHost()
    {
        static Host host;
        return host;
    }

    /*
     * StratHost starts each one of the servers it holds
     */
    void StartHost(unsigned int commandsTcpPort, unsigned int eventsTcpPort, unsigned int udpPortIn, unsigned int udpPortOut, unsigned int httpPort);

    /*
     * StopHost stops each one of the servers it holds
     */
    void StopHost();

    HostInfo& GetHostInfo() { return m_hostInfo; }
    bool GetHostUpdate(HostData& data);

    DeviceManager& GetDeviceManager() { return m_deviceManager;  }

    // Push the given event through Events TCP Server
    void PushEvent(const HostManagerEventBase& event) const;

    // Decide whether to show host_manager menu
    void SetMenuDisplay(bool menuDisplayOn);

    // delete copy Cnstr and assignment operator
    // keep public for better error message
    // no move members will be declared implicitly
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

private:
    std::shared_ptr<CommandsTcpServer> m_pCommandsTcpServer;
    std::shared_ptr<EventsTcpServer> m_pEventsTcpServer;
    std::shared_ptr<UdpServer> m_pUdpServer;
    HostInfo m_hostInfo;
    std::promise<void> m_eventsTCPServerReadyPromise;    // sync. of device manager with events TCP server creation
    DeviceManager m_deviceManager;                        // note: it should be defined after the promise passsed to its Cnstr
    bool m_MenuDisplayOn;

    // define Cnstr to be private, part of Singleton pattern
    Host();

    // Menu display function
    void DisplayMenu();
};


#endif // ! _HOST_H_

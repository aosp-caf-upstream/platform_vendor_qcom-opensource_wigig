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

#include "Host.h"
#include "DebugLogger.h"
#include <iostream>
#include <thread>
#include <functional> //Included for the "ref" in events TCP server thread
#include "Utils.h"

using namespace std;

// The private singleton Cntr
// Note: the promise is defined before device manager field
Host::Host() : m_deviceManager(m_eventsTCPServerReadyPromise), m_MenuDisplayOn(false)
{}

void Host::StartHost(unsigned int commandsTcpPort, unsigned int eventsTcpPort, unsigned int udpPortIn, unsigned int udpPortOut, unsigned int httpPort)
{
    m_pCommandsTcpServer.reset(new CommandsTcpServer(commandsTcpPort, *this));

    if (!m_pCommandsTcpServer)
    { //m_pCommandsTcpServer is NULL, couldn't set Socket
        LOG_ERROR << "Couldn't set new socket for the commands TCP server" << endl;
        throw "Couldn't set new socket for the commands TCP server";
    }
    thread threadCommandsTcpServer;
    try
    {
        LOG_INFO << "Starting commands TCP server on port " << commandsTcpPort << endl;
        //threadCommandsTcpServer = thread(&CommandsTcpServer::Start, m_pCommandsTcpServer);
        threadCommandsTcpServer = thread(&CommandsTcpServer::Start, m_pCommandsTcpServer.get());
    }
    catch (exception e)
    {
        LOG_ERROR << "Couldn't start commands TCP server" << endl;
        throw "Couldn't start commands TCP server";
    }

    m_pEventsTcpServer.reset(new EventsTcpServer(eventsTcpPort));

    if (!m_pEventsTcpServer)
    { //m_pEventsTcpServer is NULL, couldn't set Socket
        LOG_ERROR << "Couldn't set new socket for the events TCP server" << endl;
        throw "Couldn't set new socket for the events TCP server";
    }
    m_eventsTCPServerReadyPromise.set_value(); // notify device manager - OK to push events

    thread threadEventsTcpServer;
    try
    {
        LOG_INFO << "Starting events TCP server on port " << eventsTcpPort << endl;
        threadEventsTcpServer = thread(&EventsTcpServer::Start, m_pEventsTcpServer.get());
    }
    catch (exception e)
    {
        LOG_ERROR << "Couldn't start events TCP server" << endl;
        throw "Couldn't start commands TCP server";
    }

    m_pUdpServer.reset(new UdpServer(udpPortIn, udpPortOut, *this));
    if (!m_pUdpServer)
    { //m_pUdpServer is NULL, couldn't set Socket
        LOG_ERROR << "Couldn't set new socket for the UDP server" << endl;
        throw "Couldn't set new socket for the UDP server";
    }

    thread threadUdpServer;
    try
    {
        LOG_INFO << "Starting UDP server at port in: " << udpPortIn << ", and port out: " << udpPortOut << endl;
        threadUdpServer = thread(&UdpServer::StartServer, m_pUdpServer);
    }
    catch (exception e)
    {
        LOG_ERROR << "Couldn't start UDP server thread" << e.what() << endl;
        throw "Couldn't start UDP server thread";
    }

    // If user requested to display menu & user hasn't exited the menu
    while (m_MenuDisplayOn)
    {
        DisplayMenu();
    }

    threadCommandsTcpServer.join();
    threadEventsTcpServer.join();
    threadUdpServer.join();
}

void Host::StopHost()
{
    m_pCommandsTcpServer->Stop();
    m_pEventsTcpServer->Stop();
    m_pUdpServer->Stop();
}

// Push the given event through Events TCP Server
void Host::PushEvent(const HostManagerEventBase& event) const
{
    if (!m_pEventsTcpServer)
    {    // shouldn't happen
        LOG_ERROR << "Host::PushEvent: Events TCP Server is not ready!" << endl;
        return;
    }

    std::stringstream os;
    event.ToJson(os);
    m_pEventsTcpServer->SendToAllConnectedClients(os.str());
}

// Push the given event through Events TCP Server to a specific client
void Host::SetMenuDisplay(bool menuDisplayOn)
{
    m_MenuDisplayOn = menuDisplayOn;
}

// Retrieve host data
bool Host::GetHostUpdate(HostData& data)
{
    // Extract host IP
    data.m_hostIP = m_hostInfo.GetIps().m_ip;

    // Extract host_manager version
    data.m_hostManagerVersion = m_hostInfo.GetVersion();

    // Extract host Alias
    data.m_hostAlias = m_hostInfo.GetAlias();

    // Update devices status
    if (!GetDeviceManager().GetDeviceStatus(data.m_devices))
    {
        return false;
    }

    return true;
}

// Display host_manager_11ad menu
void Host::DisplayMenu()
{
    // Clear the console screen
#ifdef _WINDOWS
    system("cls");
#else
    system("clear");
#endif

    int userInput;

    string logCollectionAction = "Start";
    if (GetDeviceManager().GetLogCollectionMode() == true)
    {
        logCollectionAction = "Stop";
    }

    // Display user menu options
    cout << "Please enter select the number of the requested operation:" << endl;
    cout << "    (1) " << logCollectionAction << " FW/uCode Log Collection" << endl;
    cout << "    (2) " << " Exit" << endl;
    cin >> userInput;

    switch (userInput)
    {
    case 1:
        // Toggle the log collection status
        GetDeviceManager().SetLogCollectionMode(!GetDeviceManager().GetLogCollectionMode());
        break;
    case 2:
        StopHost();
        m_MenuDisplayOn = false;
        break;
    default:
        break;
    }
}

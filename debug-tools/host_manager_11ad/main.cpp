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

#include "OsHandler.h"
#include "ArgumentsParser.h"
#include <memory>
//#include "Server.h"
#include "DebugLogger.h"
#include <iostream> //TODO - maybe remove in final
#include "Host.h"

using namespace std;

#define DEFAULT_COMMANDS_TCP_SERVER_PORT 12348 //TODO - change back to 12348
#define DEFAULT_EVENTS_TCP_SERVER_PORT 12339 //TODO - change back to 12347
#define DEFAULT_UDP_SERVER_PORT_IN_GET_MESSAGE 12349 // This is a UDP port to get messages only from the remote server
#define DEFAULT_REMOTE_UDP_SERVER_PORT_OUT_SEND_MESSAGE 12350 // This is the UDP port in the remote server to send messages only

// *************************************************************************************************

int main(int argc, char* argv[])
{
    try
    {
        LOG_INFO << "Host Manager 11ad version: " << HostInfo::GetVersion() << endl;

        unsigned int commandsTcpPort = DEFAULT_COMMANDS_TCP_SERVER_PORT;
        unsigned int eventsTcpPort = DEFAULT_EVENTS_TCP_SERVER_PORT;
        unsigned int udpPortIn = DEFAULT_UDP_SERVER_PORT_IN_GET_MESSAGE;
        unsigned int udpPortOut = DEFAULT_REMOTE_UDP_SERVER_PORT_OUT_SEND_MESSAGE;

        unique_ptr<ArgumentsParser> pArgumentsParser(new ArgumentsParser());
        //support to change by the user the commands TCP port only,events Tcp port UDP ports are not changeable by the user
        bool continueRunningHostManager = pArgumentsParser->ParseAndHandleArguments(argc, argv, commandsTcpPort);

        if (continueRunningHostManager)
        {
            //Handle OS specific configurations
            unique_ptr<OsHandler> pOsHandler(new OsHandler());
            pOsHandler->HandleOsSignals();

            //Start Host object
            LOG_INFO << "Starting Host Manager" << endl;
            Host::GetHost().StartHost(commandsTcpPort, eventsTcpPort, udpPortIn, udpPortOut);

            LOG_INFO << "Stopping host_manager_11ad" << endl;
        }

    }
    catch (exception& e)
    {
        LOG_ERROR << "Stopping host_manager_11ad due to failure: " << e.what() << endl;
    }

    return 0;


}
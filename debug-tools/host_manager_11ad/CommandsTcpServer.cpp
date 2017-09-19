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

#include <thread>
#include "CommandsTcpServer.h"
#include "NetworkInterface.h"
#include "FileReader.h"
#include "Host.h"

using namespace std;

// *************************************************************************************************

CommandsTcpServer::CommandsTcpServer(unsigned int commandsTcpPort, Host& host)
    :m_port(commandsTcpPort),m_pSocket(new NetworkInterfaces::NetworkInterface()), m_host(host)
{
}

// *************************************************************************************************

void CommandsTcpServer::Start()
{
    //LOG_INFO << "Starting commands TCP server on port " << m_port << endl;
    m_pSocket->Bind(m_port);
    m_pSocket->Listen();

    //Infinite loop that waits for clients to connect to the commands TCP server - there's no reason to stop this loop,
    //should run forever unless there is a problem
    while (true)
    {
        try
        {
            //thread serverThread = thread(&ServerThread, m_pServer->accept());
            thread serverThread(&CommandsTcpServer::ServerThread, this, m_pSocket->Accept()); //open a new thread for each client
            serverThread.detach();
        }
        catch (exception e)
        {
            LOG_ERROR << "Couldn't make a new connection or starting a new thread in Commands TCP server for a new client " << e.what() << endl;
        }

    }
}

void CommandsTcpServer::Stop()
{
    LOG_INFO << "Stopping the commands TCP server" << endl;
    m_pSocket->Shutdown(2); //type 2 -> Acts like the close(), shutting down both input and output
    m_pSocket.reset();
}



// *************************************************************************************************
//A thread function to handle each client that connects to the server
void CommandsTcpServer::ServerThread(NetworkInterfaces::NetworkInterface client)
{
    unique_ptr<CommandsHandler> pCommandsHandler(new CommandsHandler(stTcp, m_host));
    ConnectionStatus keepConnectionAliveFromCommand = KEEP_CONNECTION_ALIVE; //A flag for the content of the command - says if the client wants to close connection
    ConnectionStatus keepConnectionAliveFromReply = KEEP_CONNECTION_ALIVE; //A flag for the reply status, for problems in sending reply etc..
    //TODO: uncomment when DMTools side is ready
    // notify that new clinet is connected to the host (send list of connected users before the new one, and notification of the new one - true means connected, false diconnected)
    Host::GetHost().PushEvent(ClientConnectionEvent(Host::GetHost().GetHostInfo().GetConnectedUsers(), client.GetPeerName(), true));
    m_host.GetHostInfo().AddNewConnectedUser(client.GetPeerName()); // add the user's to the host's connected users


    do
    {
        string concatenatedMessages;

        try
        {
            const char* message = client.Receive();
            if (NULL == message)
            {
                keepConnectionAliveFromReply = CLOSE_CONNECTION;
                break;
            }

            concatenatedMessages = message;
            vector<string> splitMessages = Utils::Split(concatenatedMessages, '\r');

            for (auto& message : splitMessages)
            {
                ResponseMessage referencedResponse;
                if (message.empty())
                { //message back from the client is "", means the connection is closed
                    break;
                }
                //Try to execute the command from the client, get back from function if to keep the connection with the client alive or not
                keepConnectionAliveFromCommand = pCommandsHandler->ExecuteCommand(message, referencedResponse);

                if (referencedResponse.type == REPLY_TYPE_WAIT_BINARY) {
                    uint8_t* binaryInput = (uint8_t*)client.BinaryReceive(referencedResponse.inputBufSize);
                    keepConnectionAliveFromCommand = pCommandsHandler->ExecuteBinaryCommand(binaryInput, referencedResponse);
                }

                //Reply back to the client an answer for his command. If it wasn't successful - close the connection
                keepConnectionAliveFromReply = CommandsTcpServer::Reply(client, referencedResponse);
            }
            //LOG_INFO << "Message from Client to commands TCP server: " << message << endl;
        }
        catch (exception e)
        {
            LOG_ERROR << "Couldn't get the message from the client" << e.what() << endl;
            break;
        }

    } while (keepConnectionAliveFromCommand != CLOSE_CONNECTION && keepConnectionAliveFromReply != CLOSE_CONNECTION);

    //client.shutdown(0); //TODO - check how to do it correctly (without exception)
    //client.shutdown(1); //TODO - check how to do it correctly (without exception)
    //client.close(); //TODO - check how to do it correctly (without exception)
    LOG_INFO << "Closed connection with the client: " << client.GetPeerName() << endl;
    m_host.GetHostInfo().RemoveNewConnectedUser(client.GetPeerName());
    //TODO: uncomment when DMTools side is ready
    //notify that new clinet is disconnected from the host
    Host::GetHost().PushEvent(ClientConnectionEvent(Host::GetHost().GetHostInfo().GetConnectedUsers(), client.GetPeerName(), false));

}

// *************************************************************************************************

ConnectionStatus CommandsTcpServer::Reply(NetworkInterfaces::NetworkInterface &client, ResponseMessage &responseMessage)
{
    LOG_DEBUG << "Reply is: " << responseMessage.message << endl;

    switch (responseMessage.type)
    {
    case REPLY_TYPE_BUFFER:
        return ReplyBuffer(client, responseMessage);
    case REPLY_TYPE_FILE:
        return ReplyFile(client, responseMessage);
    case REPLY_TYPE_BINARY:
        return ReplyBinary(client, responseMessage);
    default:
        LOG_ERROR << "Unknown reply type" << endl;
        return CLOSE_CONNECTION;
    }
}


// *************************************************************************************************

ConnectionStatus CommandsTcpServer::ReplyBuffer(NetworkInterfaces::NetworkInterface &client, ResponseMessage &responseMessage)
{
    LOG_DEBUG << "Replying from a buffer (" << responseMessage.length << "B) Content: " << responseMessage.message << endl;

    if (0 == responseMessage.length)
    {
        LOG_ERROR << "No reply generated by a command handler - connection will be closed" << endl;
        return CLOSE_CONNECTION;
    }

    //TODO - maybe the sending format is ending with "\r\n"
    if (!client.SendString(responseMessage.message + "\r"))
    {
        LOG_ERROR << "Couldn't send the message to the client, closing connection" << endl;
        return CLOSE_CONNECTION;
    }

    return KEEP_CONNECTION_ALIVE;
}


//TODO - reply file had been copied from old "wilserver" almost without touching it.
//It has to be checked and also modified to fit the new "host_server_11ad"
//The same applies to "FileReader.h" and "FileReader.cpp"
ConnectionStatus CommandsTcpServer::ReplyFile(NetworkInterfaces::NetworkInterface& client, ResponseMessage& fileName)
{
    FileReader fileReader(fileName.message.c_str());
    size_t fileSize = fileReader.GetFileSize();
    LOG_DEBUG << "Replying from a file: " << fileName.message
              << " Size: " << fileSize << " B" << std::endl;

    if (0 == fileSize)
    {
        LOG_ERROR << "No file content is available for reply" << std::endl;
        return CLOSE_CONNECTION;
    }

    static const size_t SEND_BUFFER_LEN = 1024 * 1024;
    std::unique_ptr<char[]> spSendBuffer(new char[SEND_BUFFER_LEN]);
    if (!spSendBuffer)
    {
        LOG_ERROR << "Cannot allocate send buffer of " << SEND_BUFFER_LEN << " B";
        return CLOSE_CONNECTION;
    }

    size_t chunkSize = 0;
    do
    {
        LOG_VERBOSE << "Requesting for a file chunk of " << SEND_BUFFER_LEN << " B"  << std::endl;

        chunkSize = fileReader.ReadChunk(spSendBuffer.get(), SEND_BUFFER_LEN);
        if (chunkSize > 0)
        {
            LOG_ASSERT(chunkSize <= SEND_BUFFER_LEN);
            if (!client.SendBuffer(spSendBuffer.get(), chunkSize))
            {
                LOG_ERROR << "Error occurred while replying file content - transport error" << std::endl;
                return CLOSE_CONNECTION;
            }
        }

        // Error/Completion may occur with non-zero chunk as well
        if (fileReader.IsError())
        {
            LOG_ERROR << "Cannot send reply - file read error" << std::endl;
            return CLOSE_CONNECTION;
        }

        if (fileReader.IsCompleted())
        {
            LOG_DEBUG << "File Content successfully delivered" << std::endl;
            return KEEP_CONNECTION_ALIVE;
        }

        LOG_DEBUG << "File Chunk Delivered: " << chunkSize << "B" << std::endl;
    }
    while (chunkSize > 0);

    return KEEP_CONNECTION_ALIVE;
}

ConnectionStatus CommandsTcpServer::ReplyBinary(NetworkInterfaces::NetworkInterface &client, ResponseMessage &responseMessage)
{
    if (0 == responseMessage.length)
    {
        LOG_ERROR << "No reply generated by a command handler - connection will be closed" << endl;
        return CLOSE_CONNECTION;
    }

    if (!client.SendBuffer((const char*)responseMessage.binaryMessage, responseMessage.length))
    {
        LOG_ERROR << "Couldn't send the message to the client, closing connection" << endl;
        return CLOSE_CONNECTION;
    }

    return KEEP_CONNECTION_ALIVE;
}
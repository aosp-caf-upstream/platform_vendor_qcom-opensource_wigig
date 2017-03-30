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

#include "CommandsTcpServer.h"
#include <thread>
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
    LOG_INFO << "Starting commands TCP server on port " << m_port << endl;
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
}

// *************************************************************************************************
//A thread function to handle each client that connects to the server
void CommandsTcpServer::ServerThread(NetworkInterfaces::NetworkInterface client)
{
    unique_ptr<CommandsHandler> pCommandsHandler(new CommandsHandler(stTcp, m_host));
    ConnectionStatus keepConnectionAliveFromCommand = KEEP_CONNECTION_ALIVE; //A flag for the content of the command - says if the client wants to close connection
    ConnectionStatus keepConnectionAliveFromReply = KEEP_CONNECTION_ALIVE; //A flag for the reply status, for problems in sending reply etc..
    m_host.GetHostInfo().AddNewConnectedUser(client.GetPeerName()); // add the user's to the host's connected users

    do
    {
        string messages;
        messages.empty();

        try
        {
            messages = client.Receive();
            vector<string> splitMessages = Utils::Split(messages, '\r');

            for (auto& message : splitMessages)
            {
                ResponseMessage referencedResponse = { "", REPLY_TYPE_NONE, 0 };
                if (message.empty())
                { //message back from the client is "", means the connection is closed
                    break;
                }

                //Try to execute the command from the client, get back from function if to keep the connection with the client alive or not
                keepConnectionAliveFromCommand = pCommandsHandler->ExecuteCommand(message, referencedResponse);

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
    default:
        //LOG_ERROR << "Unknown reply type" << endl;
        break;
    }
    return CLOSE_CONNECTION;
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

    try
    {
        client.Send((responseMessage.message + "\r").c_str()); //TODO - maybe the sending format is ending with "\n\r"
    }
    catch (const exception& e)
    {
        LOG_ERROR << "couldn't send the message to the client, closing connection: " << e.what() << endl;
        return CLOSE_CONNECTION;
    }
    catch (int e) // TODO: check if we can remove the previous catch
    {
        LOG_ERROR << "couldn't send the message to the client, closing connection: " << e << endl;
        return CLOSE_CONNECTION;
    }


    return KEEP_CONNECTION_ALIVE;
}


//TODO - reply file had been copied from old "wilserver" almost without touching it.
//It has to be checked and also modified to fit the new "host_server_11ad"
//The same applies to "FileReader.h" and "FileReader.cpp"
ConnectionStatus CommandsTcpServer::ReplyFile(NetworkInterfaces::NetworkInterface & client, ResponseMessage & fileName)
{
    LOG_DEBUG << "Replying from a file: " << fileName.message << std::endl;
    FileReader fileReader(fileName.message.c_str());
    size_t fileSize = fileReader.GetFileSize();

    if (0 == fileSize)
    {
        LOG_ERROR << "No file content is available for reply" << std::endl;
        return CLOSE_CONNECTION;
    }

    static const size_t BUF_LEN = 64 * 1024;

    char* pBuf = new char[BUF_LEN];
    size_t chunkSize = 0;
    bool isError = false;

    do
    {
        LOG_VERBOSE << "Requesting for a file chunk" << std::endl;

        chunkSize = fileReader.ReadChunk(pBuf, BUF_LEN);
        if (chunkSize > 0)
        {
            //if (false == send_buffer(sock, pBuf, chunkSize)) //TODO - was in the old "wilserver" changed to the next line (with client.send(pBuf))
            if (0 == client.Send(pBuf))
            {
                LOG_ERROR << "Send error detected" << std::endl;
                isError = true;
                break;
            }
        }

        // Error/Completion may occur with non-zero chunk as well
        if (fileReader.IsError())
        {
            LOG_ERROR << "File read error detected" << std::endl;
            isError = true;
            break;
        }

        if (fileReader.IsCompleted())
        {
            LOG_DEBUG << "File completion detected" << std::endl;
            break;
        }

        LOG_DEBUG << "File Chunk Delivered: " << chunkSize << "B" << std::endl;
    } while (chunkSize > 0);

    delete[] pBuf;

    if (isError)
    {
        LOG_ERROR << "Error occurred while replying file content" << std::endl;
        return CLOSE_CONNECTION;
    }
    else
    {
        LOG_DEBUG << "File Content successfully delivered" << std::endl;
        return KEEP_CONNECTION_ALIVE;
    }
}

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

#include <stdio.h>
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
#include <fstream>
#include "server.h"
#include "cmdiface.h"
#include "debug.h"
#include "pmc_file.h"
#include "file_reader.h"

// Thread parameters structure
typedef struct {
    int sock;
    struct sockaddr address;
    int addr_len;
} connection_par_t;

volatile int Server::shutdown_flag = 0;
volatile int Server::running_threads = 0;
pthread_mutex_t Server::threads_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void *Server::server_process(void *ptr)
{
    char *buf;
    CmdIface *iface;
    int rw;

    if(ptr == NULL)
    {
        pthread_exit(0);
    }

    connection_par_t *par = (connection_par_t*)ptr;
    iface = new CmdIface();
    buf = new char[MAX_INPUT_BUF+1];
    if (!buf)
    {
        LOG_ERROR << "Cannot allocate server receive buffer" << std::endl;
        pthread_exit(0);
    }

    memset(buf, 0, MAX_INPUT_BUF+1);
    rw = read(par->sock, buf, MAX_INPUT_BUF);

    while(rw > 0)
    {
        // Got some data
        if(rw)
        {
            buf[rw] = '\0';
            LOG_INFO << "Client Command: " << PlainStr(buf) << std::endl;
            int cmdKeepalive = iface->do_command((const char*)buf, rw);
            LOG_DEBUG << "Command handling keepalive: " << cmdKeepalive << std::endl;

            // Check for keepalive return value
            int replyKeepalive = reply(par->sock, iface);

            if(cmdKeepalive == KEEPALIVE_CLOSE || replyKeepalive == KEEPALIVE_CLOSE)
            {
                // Handler asked to close the connection
                rw = 0;
                break;
            }
        }
        rw = read(par->sock, buf, MAX_INPUT_BUF);
    }

    // if 0 - connection is closed, <0 - error
    LOG_DEBUG << "Closing connection. Read result: " << rw << std::endl;
    close(par->sock);
    delete[] buf;
    delete iface;
    delete par;
    pthread_mutex_lock(&threads_counter_mutex);
    running_threads--;
    pthread_mutex_unlock(&threads_counter_mutex);
    pthread_exit(0);
}

int Server::reply(int sock, CmdIface* iface)
{
    LOG_INFO << "Reply: " << PlainStr(iface->get_reply()) << std::endl;

    switch (iface->get_reply_type())
    {
    case REPLY_TYPE_BUFFER:
        return reply_buffer(sock, iface->get_reply(), iface->get_reply_len());

    case REPLY_TYPE_FILE:
        return reply_file(sock, iface->get_reply());

    default:
        LOG_ERROR << "Unexpected reply type: " << iface->get_reply_type() << std::endl;
        return KEEPALIVE_CLOSE;
    }
}

int Server::reply_buffer(int sock, const char* pBuf, size_t len)
{
    LOG_DEBUG << "Replying from a buffer (" << len << "B) Content: " << PlainStr(pBuf) << std::endl;

    if (0 == len)
    {
        LOG_ERROR << "No reply generated by a command handler - connection will be closed" << std::endl;
        return KEEPALIVE_CLOSE;
    }

    if (false == send_buffer(sock, pBuf, len))
    {
        return KEEPALIVE_CLOSE;
    }

    return KEEPALIVE_OK;
}

int Server::reply_file(int sock, const char* pFileName)
{
    LOG_DEBUG << "Replying from a file: " << pFileName << std::endl;

    FileReader fileReader(pFileName);
    size_t fileSize = fileReader.GetFileSize();

    if (0 == fileSize)
    {
        LOG_ERROR << "No file content is available for reply" << std::endl;
        return KEEPALIVE_CLOSE;
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
            if (false == send_buffer(sock, pBuf, chunkSize))
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
    }
    while (chunkSize > 0);

    delete[] pBuf;

    if (isError)
    {
        LOG_ERROR << "Error occured while replying file content" << std::endl;
        return KEEPALIVE_CLOSE;
    }
    else
    {
        LOG_DEBUG << "File Content successfully delivered" << std::endl;
        return KEEPALIVE_OK;
    }
}

bool Server::send_buffer(int sock, const char* pBuf, size_t len)
{
    if (!pBuf)
    {
        LOG_ERROR << "Cannot reply to a command - No buffer is provided" << std::endl;
        return false;
    }

    size_t sentTillNow = 0;

    while(sentTillNow < len)
    {
        ssize_t sentBytes = write(sock, pBuf, len);
        if (sentBytes < 0)
        {
            int lastErrno = errno;
            LOG_ERROR << "Cannot send response buffer."
                      << " Error:" << lastErrno
                      << " Message: " << strerror(lastErrno)
                      << std::endl;
            return false;
        }

        sentTillNow += sentBytes;
        LOG_DEBUG << "Sent response chunk."
                  << " Chunk Size: "      << sentBytes
                  << " Sent till now: "   << sentTillNow
                  << " Response Length: " <<  len
                  << std::endl;
    }

    LOG_DEBUG << "Response buffer fully sent" << std::endl;
    return true;
}

/*
  Close the server to allow un-bind te socket - allowing future connections without delay
*/
int Server::stop()
{
    LOG_INFO << "Stopping the server" << std::endl;
    shutdown(sock, SHUT_RDWR);
    return 0;
}

/*
  Initialize server on the given port. The function returns in case of error,
  otherwise it doesn't return
*/
int Server::start(int port)
{
    LOG_INFO << "Starting the server on port " << port << std::endl;

    struct sockaddr_in address;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(sock < 0)
    {
        LOG_ERROR << "Cannot create a socket on port " << port << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Set the "Re-Use" socket option - allows reconnections after wilserver exit
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    if(bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
    {
        LOG_ERROR << "Cannot bind socket to port " << port << std::endl;
        return -2;
    }

    if (listen(sock, 5) < 0)
    {
        LOG_ERROR << "Cannot listen on port " << port << std::endl;
        return -3;
    }

    shutdown_flag = 0;
    while (shutdown_flag == 0)  {
        pthread_t thread;
        connection_par_t *par = new connection_par_t;
        par->sock = accept(sock, &par->address, (socklen_t*)&par->addr_len);
        if(par->sock < 0)
            delete par;
        else {
            pthread_mutex_lock(&threads_counter_mutex);
            running_threads++;
            pthread_mutex_unlock(&threads_counter_mutex);
            pthread_create(&thread, 0, &Server::server_process, (void *)par);
            pthread_detach(thread);
        }

    }
    // Wait till all the threads are done in case we ever exit the loop above
    while(running_threads > 0)
        sleep(1);

    LOG_INFO << "Server shutdown" << std::endl;

    return 0; // Wont get here, just to avoid the warning
}

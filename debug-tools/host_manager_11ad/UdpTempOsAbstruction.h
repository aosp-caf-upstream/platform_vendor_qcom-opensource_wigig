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

#ifndef _UDPTEMPOSABSTRUCTION_H_
#define _UDPTEMPOSABSTRUCTION_H_

#include <string>
#include <memory>
#include "HostDefinitions.h"

using namespace std;

class UdpTempOsAbstruction
{
public:

    /*
    GetHostIps
    Returns host's personal IP and broadcat IP
    @param: none
    @return: HostIps - host's personal IP and broadcat IP
    */
    static HostIps GetHostIps();

    /*
    ReadFile
    Gets a file name and reads its context.
    @param: fileName - full file name
    @return: file's content
    */
    static bool ReadFile(string fileName, string& data);

    /*
    WriteFile
    Gets a file name and the required content, and write the content to the file (overrides the old content if exists)
    @param: fileName - full file name
    @param: content - the new file's content
    @return: true for successful write operation, false otherwise
    */
    static bool WriteFile(string fileName, string content);

    /*
    GetPersistencyLocation
    Returns the host persistency location prefix accurding to the OS
    @param: none
    @return: host persistency location prefix accurding to the OS
    */
    static string GetPersistencyLocation();

    /*
    ReadHostOsAlias
    Returns the host's alias as defined in persistency
    @param: a reference to a string that will be updated with the host's alias
    @return: bool - status - true for successful operation, false otherwise
    */
    static bool ReadHostOsAlias(string& alias);

    static bool IsFolderExists(string path);

    static bool CreateFolder(string path);

    static void MoveFileToNewLocation(string oldFileLocation, string newFileLocation);

    static string GetDirectoriesDilimeter();

    static bool IsFileExists(string path);

    static const string LOCAL_HOST_IP; // default IP address

private:
    static bool FindEthernetInterface(struct ifreq& ifr, int& fd);

};

class UdpSocket
{
public:

    UdpSocket(string broadcastIp, int portIn, int portOut);

    ~UdpSocket() { Close(); }

    int Send(string message);

    const char* Receive(int len);

    void Close();

private:

    int m_portIn;
    int m_portOut;
    int m_socket;
    string m_broadcastIp;
};

#endif
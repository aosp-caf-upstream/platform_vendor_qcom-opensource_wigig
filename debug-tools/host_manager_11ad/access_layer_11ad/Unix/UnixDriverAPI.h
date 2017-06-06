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

#ifndef _11AD_PCI_UNIX_DRIVER_API_H_
#define _11AD_PCI_UNIX_DRIVER_API_H_


#ifndef _WINDOWS

#include "DriverAPI.h"

#define DEBUG_FS_MAX_PATH_LENGTH 1024

using namespace std;

class UnixDriverAPI : public DriverAPI
{
    typedef struct
    {
        DWORD deviceUID;
        DWORD commandID;
        DWORD dataSize; /* inBufSize + outBufSize */
        DWORD inBufOffset;
        DWORD inBufSize;
        DWORD outBufOffset;
        DWORD outBufSize;
    } IoctlHeader;

    #define IOCTL_FLAG_SET 0x1
    #define IOCTL_FLAG_GET 0x2

    #define EP_OPERATION_READ 0
    #define EP_OPERATION_WRITE 1
    #define WIL_IOCTL_MEMIO (SIOCDEVPRIVATE + 2)

    #define INVALID_FD -1

    static __INLINE BYTE *
        IoctlDataIn(IoctlHeader *h)
    {
        return ((BYTE*)&h[1]) + h->inBufOffset;
    }

    static __INLINE BYTE *
        IoctlDataOut(IoctlHeader *h)
    {
        return ((BYTE*)&h[1]) + h->outBufOffset;
    }

    typedef struct {
        uint32_t op;
        uint32_t addr; /* should be 32-bit aligned */
        uint32_t val;
    } IoctlIO;

public:
    UnixDriverAPI(string interfaceName) : DriverAPI(interfaceName)
    {
        m_initialized = false;
    }
    ~UnixDriverAPI();

    // Base access functions (to be implemented by specific device)
    bool Read(DWORD address, DWORD& value);
    bool ReadBlock(DWORD addr, DWORD blockSize, char *arrBlock);
    bool Write(DWORD address, DWORD value);
    bool WriteBlock(DWORD addr, DWORD blockSize, const char *arrBlock);

    bool IsOpened(void);

    bool Open();
    bool ReOpen();
    bool InternalOpen();
    bool DriverControl(uint32_t Id,
        const void *inBuf, uint32_t inBufSize,
        void *outBuf, uint32_t outBufSize);
    DWORD DebugFS(char *FileName, void *dataBuf, DWORD dataBufLen, DWORD DebugFSFlags);

    bool AllocPmc(unsigned descSize, unsigned descNum, std::string& outMessage);
    bool DeallocPmc(std::string& outMessage);
    bool CreatePmcFile(unsigned refNumber, std::string& outMessage);
    bool FindPmcFile(unsigned refNumber, std::string& outMessage);

    void Close();

    int GetDriverMode(int &currentState);
    bool SetDriverMode(int newState, int &oldState);

    void Reset();

    static set<string> Enumerate();

private:
    bool InternalIoctl(void *dataBuf, DWORD dataBufLen, DWORD ioctlFlags);
    bool SendRWIoctl(IoctlIO & io, int fd, const char* interfaceName);
    bool ValidateInterface();

    bool m_initialized;

    int m_fileDescriptor;

    string m_debugFsPath;
};

#endif // ifndef _WINDOWS

#endif //_11AD_PCI_UNIX_DRIVER_API_H_
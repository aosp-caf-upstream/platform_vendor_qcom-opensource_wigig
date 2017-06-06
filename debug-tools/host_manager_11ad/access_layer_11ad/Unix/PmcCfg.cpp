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

#include "PmcCfg.h"
#include "DebugLogger.h"

#include <string.h>
#include <fstream>
#include <errno.h>

// *************************************************************************************************

PmcCfg::PmcCfg(const char* szDebugFsPath)
{
    if (!szDebugFsPath)
    {
        LOG_ERROR << "No Debug FS path is provided" << std::endl;
        return;
    }

    std::stringstream pathBuilder;
    pathBuilder << szDebugFsPath << '/' << "pmccfg";

    m_PmcCfgFilePath = pathBuilder.str();

    LOG_DEBUG << "PMC Configurator Created.\n"
              << "    Debug FS Path: " << szDebugFsPath << '\n'
              << "    pmccfg: " << m_PmcCfgFilePath
              << std::endl;
}

// *************************************************************************************************

OperationStatus PmcCfg::AllocateDmaDescriptors(size_t descSize, size_t descNum)
{
    LOG_DEBUG << "Allocating PMC DMA Range. Descriptor Size: "
              << descSize << " Number of descriptors: " << descNum << std::endl;

    // TODO - Verify parameters for a valid range

    std::stringstream cmdBuilder;
    cmdBuilder << "alloc " << descNum << " " << descSize;

    OperationStatus st = WritePmcCommand(cmdBuilder.str().c_str());
    LOG_DEBUG << "Writing PMC command status: " << st << std::endl;

    if (!st.IsSuccess())
    {
        LOG_ERROR << "Failure writing PMC command: " << st << std::endl;
        return st;
    }

    return GetLastOperationStatus();
}

// *************************************************************************************************

OperationStatus PmcCfg::FreeDmaDescriptors()
{
    LOG_DEBUG << "De-Allocating PMC DMA Range" << std::endl;

    OperationStatus st = WritePmcCommand("free");
    LOG_DEBUG << "Querying driver for the last PMC command status: " << st << std::endl;

    if (!st.IsSuccess())
    {
        LOG_ERROR << "Failure querying for the last PMC command status: " << st << std::endl;
        return st;
    }

    return GetLastOperationStatus();
}

// *************************************************************************************************

OperationStatus PmcCfg::WritePmcCommand(const char* szPmcCmd)
{
    LOG_DEBUG << "Writing PMC command to pmccfg: " << szPmcCmd << std::endl;

    if(m_PmcCfgFilePath.empty())
    {
        return OperationStatus(false, "Cannot allocate PMC memory - No pmccfg file is detected");
    }

    std::ofstream debugFSFile;
    debugFSFile.open(m_PmcCfgFilePath);

    if (!debugFSFile.is_open())
    {
        std::stringstream msgBuilder;
        msgBuilder << "Cannot open " << m_PmcCfgFilePath << ": " << strerror(errno);
        return OperationStatus(false, msgBuilder.str().c_str());
    }

    debugFSFile << szPmcCmd;

    // Verify the stream state after writing
    if (!debugFSFile)
    {
        std::stringstream msgBuilder;
        msgBuilder << "Cannot write to " << m_PmcCfgFilePath << ": " << strerror(errno);
        return OperationStatus(false, msgBuilder.str().c_str());
    }

    // Unnecessary as closed on destruction, just for clarity.
    debugFSFile.close();

    return OperationStatus(true);
}

// *************************************************************************************************

OperationStatus PmcCfg::GetLastOperationStatus()
{
    LOG_DEBUG << "Querying pmccfg for the last operation status" << std::endl;

    if(m_PmcCfgFilePath.empty())
    {
        return OperationStatus(false, "Cannot query for last PMC operation status - No pmccfg file is detected");
    }

    std::ifstream debugFSFile;
    debugFSFile.open(m_PmcCfgFilePath);

    if(!debugFSFile.is_open())
    {
        std::stringstream msgBuilder;
        msgBuilder << "Cannot open " << m_PmcCfgFilePath << ": " << strerror(errno);
        return OperationStatus(false, msgBuilder.str().c_str());
    }

    // Query pmcconfig for the operation status
    // The file first line is expected to be formatted as follows:
    // Last command status: X
    std::string firstLineToken1;
    std::string firstLineToken2;
    std::string firstLineToken3;
    int lastOpStatus = -1;
    debugFSFile >> firstLineToken1 >> firstLineToken2 >> firstLineToken3 >> lastOpStatus;

    if (!debugFSFile)
    {
        // The failbit will also be set if the first line is not formatted as expected.
        // In this case errno is zero and error message is 'Success'.
        std::stringstream msgBuilder;
        msgBuilder << "Cannot read from " << m_PmcCfgFilePath << ": " << strerror(errno);
        return OperationStatus(false, msgBuilder.str().c_str());
    }

    LOG_DEBUG << "First line of pmccfg: "
              << firstLineToken1 << ' '
              << firstLineToken2 << ' '
              << firstLineToken3 << ' '
              << lastOpStatus
              << std::endl;

    if (0 != lastOpStatus)
    {
        // TODO: Parse erorr code to a meaningfull message
        std::stringstream msgBuilder;
        msgBuilder << "pmccfg operation failure reported by the driver: " << lastOpStatus;
        return OperationStatus(false, msgBuilder.str().c_str());
    }

    return OperationStatus(true);
}

// *************************************************************************************************

OperationStatus PmcCfg::FlashPmcData()
{
    // TBD
    OperationStatus st(false, "Not Implemented");
    return st;
}

// *************************************************************************************************

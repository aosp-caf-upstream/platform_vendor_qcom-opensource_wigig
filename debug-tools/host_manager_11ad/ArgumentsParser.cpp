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


#include "ArgumentsParser.h"
#include "HostInfo.h"

#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;
// *************************************************************************************************

int ArgumentsParser::ParseAndHandleArguments(int argc, char * argv[], unsigned int &commandsTcpPort)
{

    //do something with params
    (void)commandsTcpPort;

    for (int i = 0; i < argc; i++)
    {
        m_arguments.push_back(string(argv[i]));
    }
    if (DoesArgumentExist("-v"))
    { //Argument for the version of host_manager_11ad
        printf("Host Manager 11ad v%s\n", HostInfo::GetVersion().c_str());
    }
    if (DoesArgumentExist("-p"))
    { //Argument for setting the port of the commands TCP port

    }

    unsigned val;
    if (GetArgumentValue("-d", val))
    { //Argument for setting verbosity level
        g_LogConfig.SetMaxSeverity(val);
    }

    return 0;
}

bool ArgumentsParser::DoesArgumentExist(string option)
{
    bool doesArgumentExist = find(m_arguments.begin(), m_arguments.end(), option) != m_arguments.end();
    return doesArgumentExist;
}

bool ArgumentsParser::GetArgumentValue(string option, unsigned& val)
{
    auto argumentIter = find(m_arguments.begin(), m_arguments.end(), option);
    if (argumentIter != m_arguments.end())
    {
        auto valueIter = ++argumentIter;
        if (valueIter != m_arguments.end())
        {
            string valStr = *valueIter;
            try
            {
                val = strtoul(valStr.c_str(), NULL, 10);
                return true;
            }
            catch (...)
            {
                printf("Error in setting verbosity level\n");
            }
        }
    }
    return false;
}

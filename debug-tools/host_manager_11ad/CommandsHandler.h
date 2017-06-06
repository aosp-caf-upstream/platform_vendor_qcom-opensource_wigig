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

#ifndef _COMMANDSHANDLER_H_
#define _COMMANDSHANDLER_H_

#include <iostream>
#include <memory>
#include <map>
#include "MessageParser.h"
#include "HostDefinitions.h"
#include <sstream>

class Host;

using namespace std;


// *************************************************************************************************

class CommandsHandler
{
public:
    /*
     * pCommandFunction is used for the functions map - each function gets two arguments:
     * vector of strings which holds the arguments and the number of arguments in that vector
     */
    typedef ResponseMessage(CommandsHandler::*pCommandFunction)(vector<string>, unsigned int);

    /*
     * The constructor inserts each one of the available functions into the map - m_functionHandler - according to server type (TCP/UDP)
     */
    CommandsHandler(ServerType type, Host& host);

    ConnectionStatus ExecuteCommand(string message, ResponseMessage &referencedResponse);
    ConnectionStatus ExecuteBinaryCommand(uint8_t* binaryInput, ResponseMessage &referencedResponse);

private:
    shared_ptr<MessageParser> m_pMessageParser;
    //m_functionHandler is a map that maps a string = command name, to a function
    map<string, pCommandFunction> m_functionHandler;
    Host& m_host; // a refernce to the host (enables access to deviceManager and hostInfo)

    enum CommandsHandlerResponseStatus
    {
        chrsInvalidNumberOfArguments,
        chrsInvalidArgument,
        chrsOperationFailure,
        chrsLinuxSupportOnly,
        chrsDeviceIsSilent,
        chrsSuccess
    };

    string GetCommandsHandlerResponseStatusString(CommandsHandlerResponseStatus status)
    {
        switch (status)
        {
        case chrsInvalidNumberOfArguments:
            return "Invalid arguments number";
        case chrsInvalidArgument:
            return "Invalid argument type";
        case chrsOperationFailure:
            return "Operation failure";
        case chrsLinuxSupportOnly:
            return "Linux support only";
        case chrsSuccess:
            return "Success";
        case chrsDeviceIsSilent:
            return "SilentDevice";

        default:
            return "CommandsHandlerResponseStatus is unknown";
        }
    }

    /*
      FormatResponseMessage
      Decorate the response message with time stamp and a success status
      @param: successStatus - true for a successful operation, false otherwise
      @param: message - the content of the response
      @return: the decorated response
    */
    string DecorateResponseMessage(bool successStatus, string message = "");

    // **********************************Commands Functions:****************************************
    ResponseMessage GetInterfaces(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage OpenInterface(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage CloseInterface(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage Read(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage Write(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage ReadBlock(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage WriteBlock(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage InterfaceReset(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage SwReset(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage AllocPmc(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage DeallocPmc(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage CreatePmcFile(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage FindPmcFile(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage SendWmi(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage GetTime(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage SetDriverMode(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage GetHostManagerVersion(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage DriverControl(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage GenericDriverIO(vector<string> arguments, void* inputBuf, unsigned int inputBufSize);

    ResponseMessage GetDeviceSilenceMode(vector<string> arguments, unsigned int numberOfArguments);

    ResponseMessage SetDeviceSilenceMode(vector<string> arguments, unsigned int numberOfArguments);

    bool ValidArgumentsNumber(string functionName, size_t numberOfArguments, size_t expectedNumOfArguments, string& responseMessage)
    {
        if (expectedNumOfArguments != numberOfArguments)
        {
            stringstream error;
            LOG_WARNING << "Mismatching number of arguments in " << functionName << ": expected " << expectedNumOfArguments << " but got "<< numberOfArguments << endl;
            responseMessage = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidNumberOfArguments));
            return false;
        }
        return true;
    }

    /*
      GetHostNetworkInfo
      Return host's IP and host's alias as the Response
      @param: an empty vector
      @return: a response with a string that contains both  host's IP and host's alias
    */
    ResponseMessage GetHostNetworkInfo(vector<string> arguments, unsigned int numberOfArguments);

    /*
      SetHostAlias
      Get a new alias and define it as the new host's alias
      @param: a vector with one string representing the new alias
      @return: a response with feedback about the operation status (success/failure)
    */
    ResponseMessage SetHostAlias(vector<string> arguments, unsigned int numberOfArguments);

    const char m_device_delimiter = ' ';

    const char m_array_delimiter = ' ';

    const char m_reply_feilds_delimiter = '|';
};


#endif // !_COMMANDSHANDLER_H_

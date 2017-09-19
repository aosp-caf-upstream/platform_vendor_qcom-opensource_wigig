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

#include "CommandsHandler.h"
#include "Host.h"
#include "HostDefinitions.h"
#include "FileReader.h"

#include <sstream>
#include <string>

string CommandsHandler::DecorateResponseMessage(bool successStatus, string message)
{
    string status = successStatus ? "Success" : "Fail";
    string decoratedResponse = Utils::GetCurrentLocalTime() + m_reply_feilds_delimiter + status;
    if (message != "")
    {
        decoratedResponse += m_reply_feilds_delimiter + message;
    }
    return decoratedResponse;
}

// **************************************TCP commands handlers*********************************************************** //
ResponseMessage CommandsHandler::GetInterfaces(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)arguments;

    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        set<string> devices;
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().GetDevices(devices);

        if (dmosSuccess != status)
        {
            LOG_ERROR << "Error while trying to get interfaces. Error: " << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status)? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            // create one string that contains all connected devices
            stringstream devicesSs;
            bool firstTime = true;

            for (std::set<string>::const_iterator it = devices.begin(); it != devices.end(); ++it)
            {
                if (firstTime)
                {
                    devicesSs << *it;
                    firstTime = false;
                    continue;
                }
                devicesSs << m_device_delimiter << *it;
            }
            response.message = DecorateResponseMessage(true, devicesSs.str());
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::OpenInterface(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().OpenInterface(arguments[0]);

        if (dmosSuccess != status)
        {
            LOG_ERROR << "Error while trying to open interface " << arguments[0] + ". Error: " << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            response.message = DecorateResponseMessage(true, arguments[0]); // backward compatibility
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::CloseInterface(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().CloseInterface(arguments[0]);

        if (dmosSuccess != status)
        {
            LOG_ERROR << "Error while trying to close interface " << arguments[0] << ". Error: " + m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            response.message = DecorateResponseMessage(true); // backward compatibility
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::Read(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
        DWORD address;
        if (!Utils::ConvertHexStringToDword(arguments[1], address))
        {
            LOG_WARNING << "Error in Read arguments: given address isn't starting with 0x" << endl;
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            DWORD value;
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().Read(arguments[0], address, value);
            if (dmosSuccess != status)
            {
                if (dmosSilentDevice == status)
                {
                    response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsDeviceIsSilent));
                }
                else
                {
                    LOG_ERROR << "Error while trying to read address " << arguments[1] << " from " + arguments[0] << ". Error: " + m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
                    response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                        DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));

                }
            }
            else
            {
                stringstream message;
                message << "0x" << hex << value;
                response.message = DecorateResponseMessage(true, message.str());
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::Write(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 3, response.message))
    {
        DWORD address, value;
        if (!Utils::ConvertHexStringToDword(arguments[1], address) || !Utils::ConvertHexStringToDword(arguments[2], value))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().Write(arguments[0], address, value);
            if (dmosSuccess != status)
            {
                if (dmosSilentDevice == status)
                {
                    response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsDeviceIsSilent));
                }
                else
                {
                    LOG_ERROR << "Error while trying to write value " << arguments[2] << " to " << arguments[1] + " on "
                              << arguments[0] + ". Error: "
                              << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
                    response.message = (dmosNoSuchConnectedDevice == status) ?
                        DecorateResponseMessage(false,m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                        DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
                }
            }
            else
            {
                response.message = DecorateResponseMessage(true);
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::ReadBlock(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 3, response.message))
    {
        DWORD address, blockSize;
        if (!Utils::ConvertHexStringToDword(arguments[1], address) || !Utils::ConvertHexStringToDword(arguments[2], blockSize))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            vector<DWORD> values;
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().ReadBlock(arguments[0], address, blockSize, values);
            if (dmosSuccess != status)
            {
                if (dmosSilentDevice == status)
                {
                    response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsDeviceIsSilent));
                }
                else
                {
                    LOG_ERROR << "Error while trying to read " << arguments[2] << " addresses starting at address "
                              << arguments[1] << " from " << arguments[0] << ". Error: "
                              << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
                    response.message = (dmosNoSuchConnectedDevice == status) ?
                        DecorateResponseMessage(false,m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                        DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
                }
            }
            else
            {
                stringstream responseSs;
                auto it = values.begin();
                if (it != values.end())
                {
                    responseSs << "0x" << hex << *it;
                    ++it;
                }
                for (; it != values.end(); ++it)
                {
                    responseSs << m_array_delimiter << "0x" << hex << *it;
                }
                response.message = DecorateResponseMessage(true, responseSs.str());
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::WriteBlock(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 3, response.message))
    {
        DWORD address;
        vector<DWORD> values;
        if (!Utils::ConvertHexStringToDword(arguments[1], address) || !Utils::ConvertHexStringToDwordVector(arguments[2], m_array_delimiter, values))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            // perform write block
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().WriteBlock(arguments[0], address, values);
            if (dmosSuccess != status)
            {
                if (dmosSilentDevice == status)
                {
                    response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsDeviceIsSilent));
                }
                else
                {
                    LOG_ERROR << "Error in write blocks. arguments are:\nDevice name - " << arguments[0]
                              << "\nStart address - " << arguments[1] <<
                        "\nValues - " << arguments[2] << endl;
                    response.message = (dmosNoSuchConnectedDevice == status) ?
                        DecorateResponseMessage(false,m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)):
                        DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
                }
            }
            else
            {
                response.message = DecorateResponseMessage(true);
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::InterfaceReset(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().InterfaceReset(arguments[0]);
        if (dmosSuccess != status)
        {
            LOG_ERROR << "Failed to perform interface reset on " << arguments[0] << ". Error: " << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            response.message = DecorateResponseMessage(true);
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::SwReset(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().SwReset(arguments[0]);
        if (dmosSuccess != status)
        {
            LOG_ERROR << "Failed to perform sw reset on " << arguments[0] << ". Error: " << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            response.message = DecorateResponseMessage(true);
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************

ResponseMessage CommandsHandler::AllocPmc(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)numberOfArguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    stringstream ss;
    for (auto& s : arguments)
    {
        ss << "," << s;
    }
    LOG_VERBOSE << ss.str() << endl;

    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 3, response.message))
    {
        unsigned descSize;
        unsigned descNum;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], descSize) ||
            !Utils::ConvertDecimalStringToUnsignedInt(arguments[2], descNum))
        {
            stringstream error;
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            std::string errorMsg;
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().AllocPmc(arguments[0], descSize, descNum, errorMsg);
            if (dmosSuccess != status)
            {
                stringstream error;
                LOG_ERROR << "PMC allocation Failed: " << errorMsg << std::endl;
                response.message = DecorateResponseMessage(false, errorMsg);
            }
            else
            {
                response.message = DecorateResponseMessage(true);
            }
        }
    }

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************

ResponseMessage CommandsHandler::DeallocPmc(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)numberOfArguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    stringstream ss;
    for (auto& s : arguments)
    {
        ss << "," << s;
    }
    LOG_VERBOSE << ss.str() << endl;

    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 1, response.message))
    {
        std::string errorMsg;
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().DeallocPmc(arguments[0], errorMsg);
        if (dmosSuccess != status)
        {
            stringstream error;
            LOG_ERROR << "PMC de-allocation Failed: " << errorMsg << std::endl;
            response.message = DecorateResponseMessage(false, errorMsg);
        }
        else
        {
            response.message = DecorateResponseMessage(true);
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

// *************************************************************************************************

ResponseMessage CommandsHandler::CreatePmcFile(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)numberOfArguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    stringstream ss;
    for (auto& s : arguments)
    {
        ss << "," << s;
    }
    LOG_VERBOSE << ss.str() << endl;

    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 2, response.message))
    {
        unsigned refNumber;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], refNumber))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            std::string outMsg;
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().CreatePmcFile(arguments[0], refNumber, outMsg);
            if (dmosSuccess != status)
            {
                stringstream error;
                LOG_ERROR << "PMC data file creation failed: " << outMsg << std::endl;
                response.message = DecorateResponseMessage(false, outMsg);
            }
            else
            {
                response.message = DecorateResponseMessage(true, outMsg);
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::FindPmcFile(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)numberOfArguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    stringstream ss;
    for (auto& s : arguments)
    {
        ss << "," << s;
    }
    LOG_VERBOSE << ss.str() << endl;

    ResponseMessage response;
    response.type = REPLY_TYPE_BUFFER;

#ifdef _WINDOWS
    response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsLinuxSupportOnly));
#else
    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 2, response.message))
    {
        unsigned refNumber;
        if (!Utils::ConvertDecimalStringToUnsignedInt(arguments[1], refNumber))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        else
        {
            std::string outMessage;
            DeviceManagerOperationStatus status = m_host.GetDeviceManager().FindPmcFile(arguments[0], refNumber, outMessage);
            if (dmosSuccess != status)
            {
                stringstream error;
                LOG_ERROR << "PMC data file lookup failed: " << outMessage << std::endl;
                response.message = DecorateResponseMessage(false, outMessage);
            }
            else
            {
                response.message = outMessage;
                response.type = REPLY_TYPE_FILE;
            }
        }
    }
#endif // _WINDOWS
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::SendWmi(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)numberOfArguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    stringstream ss;
    for (auto& s : arguments)
    {
        ss << "," << s;
    }
    LOG_VERBOSE << ss.str() << endl;

    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, arguments.size(), 3, response.message))
    {
        DWORD command;
        vector<DWORD> payload;
        if (!Utils::ConvertHexStringToDword(arguments[1], command) || !Utils::ConvertHexStringToDwordVector(arguments[2], m_array_delimiter, payload))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().SendWmi(arguments[0], command, payload);
        if (dmosSuccess != status)
        {
            if (dmosSilentDevice == status)
            {
                response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsDeviceIsSilent));
            }
            else
            {
                LOG_ERROR << __FUNCTION__ << ":" << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
                response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                    DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
            }
        }
        else
        {
            response.message = DecorateResponseMessage(true);
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::SetHostAlias(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        if (m_host.GetHostInfo().SaveAliasToFile(arguments[0]))
        {
            response.message = DecorateResponseMessage(true);
        }
        else
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetHostAlias(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true, m_host.GetHostInfo().GetAlias());
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetTime(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)arguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        response.message = DecorateResponseMessage(true);
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

// *************************************************************************************************
ResponseMessage CommandsHandler::SetDriverMode(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
        int newMode = IOCTL_WBE_MODE;
        int oldMode = IOCTL_WBE_MODE;

        if ("WBE_MODE" == arguments[1])
        {
            newMode = IOCTL_WBE_MODE;
        }
        else if ("WIFI_STA_MODE" == arguments[1])
        {
            newMode = IOCTL_WIFI_STA_MODE;
        }
        else if ("WIFI_SOFTAP_MODE" == arguments[1])
        {
            newMode = IOCTL_WIFI_SOFTAP_MODE;
        }
        else if ("CONCURRENT_MODE" == arguments[1])
        {
            newMode = IOCTL_CONCURRENT_MODE;
        }
        else if ("SAFE_MODE" == arguments[1])
        {
            newMode = IOCTL_SAFE_MODE;
        }
        else
        {
            // TODO
            response.message = dmosFail;
            response.type = REPLY_TYPE_BUFFER;
            response.length = response.message.size();
            return response;
        }

        DeviceManagerOperationStatus status = m_host.GetDeviceManager().SetDriverMode(arguments[0], newMode, oldMode);
        if (dmosSuccess != status)
        {
            LOG_ERROR << "Failed to set driver mode on " << arguments[0] << ". Error: " << m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            string message;

            switch (oldMode)
            {
            case IOCTL_WBE_MODE:
                message = "WBE_MODE";
                break;
            case IOCTL_WIFI_STA_MODE:
                message = "WIFI_STA_MODE";
                break;
            case IOCTL_WIFI_SOFTAP_MODE:
                message = "WIFI_SOFTAP_MODE";
                break;
            case IOCTL_CONCURRENT_MODE:
                message = "CONCURRENT_MODE";
                break;
            case IOCTL_SAFE_MODE:
                message = "SAFE_MODE";
                break;
            default:
                break;
            }

            response.message = DecorateResponseMessage(true, message);
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

ResponseMessage CommandsHandler::GetHostManagerVersion(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)arguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 0, response.message))
    {
        string res = m_host.GetHostInfo().GetVersion();
        response.message = DecorateResponseMessage(true, res);
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

// *************************************************************************************************
ResponseMessage CommandsHandler::DriverControl(vector<string> arguments, unsigned int numberOfArguments)
{
    //cout << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 4, response.message))
    {
        DWORD inBufSize;
        //vector<DWORD> inputValues;
        if (!Utils::ConvertHexStringToDword(arguments[2], inBufSize))
        {
            response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
        }
        response.inputBufSize = inBufSize;
    }
    response.internalParsedMessage = arguments;
    response.type = REPLY_TYPE_WAIT_BINARY;
    return response;
}
// *************************************************************************************************

// *************************************************************************************************
ResponseMessage CommandsHandler::GenericDriverIO(vector<string> arguments, void* inputBuf, unsigned int inputBufSize)
{
    //cout << __FUNCTION__ << endl;
    ResponseMessage response;
    DWORD id, inBufSize, outBufSize;
    //vector<DWORD> inputValues;
    if (!Utils::ConvertHexStringToDword(arguments[1], id) || !Utils::ConvertHexStringToDword(arguments[2], inBufSize) || !Utils::ConvertHexStringToDword(arguments[3], outBufSize))
    {
        response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
    }

    else {
        DeviceManagerOperationStatus status;

        uint8_t* outputBuf = new uint8_t[outBufSize];
        memset(outputBuf, 0, outBufSize);

        status = m_host.GetDeviceManager().DriverControl(arguments[0], id, inputBuf, inBufSize, outputBuf, outBufSize);
        response.length = outBufSize;

        if (dmosSuccess != status)
        {
            LOG_DEBUG << "Driver IO command handler: Failed to execute driver IOCTL operation" << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
            response.binaryMessage = (uint8_t*)"Failed to read from driver";
        }
        else
        {
            LOG_DEBUG << "Driver IO command handler: Success" << endl;
            response.binaryMessage = (uint8_t*)outputBuf;
        }
    }

    response.type = REPLY_TYPE_BINARY;
    return response;
}


ResponseMessage CommandsHandler::GetDeviceSilenceMode(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 1, response.message))
    {
        bool silentMode;
        DeviceManagerOperationStatus status = m_host.GetDeviceManager().GetDeviceSilentMode(arguments[0], silentMode);
        if (dmosSuccess != status)
        {
            LOG_ERROR << "Error while trying to GetDeviceSilenceMode at " << arguments[0] << ". Error: " + m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
            response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
        }
        else
        {
            stringstream message;
            message << (silentMode ? 1 : 0);
            response.message = DecorateResponseMessage(true, message.str());
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}

ResponseMessage CommandsHandler::SetDeviceSilenceMode(vector<string> arguments, unsigned int numberOfArguments)
{
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;
    if (ValidArgumentsNumber(__FUNCTION__, numberOfArguments, 2, response.message))
    {
        {
            bool silentMode = false;

            if (!Utils::ConvertStringToBool(arguments[1], silentMode))
            {
                response.message = DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsInvalidArgument));
            }

            DeviceManagerOperationStatus status = m_host.GetDeviceManager().SetDeviceSilentMode(arguments[0], silentMode);
            if (dmosSuccess != status)
            {
                LOG_ERROR << "Error while trying to SetDeviceSilenceMode at: " << arguments[0] << " to: " + arguments[1] << ". Error: " + m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status) << endl;
                response.message = (dmosNoSuchConnectedDevice == status) ? DecorateResponseMessage(false, m_host.GetDeviceManager().GetDeviceManagerOperationStatusString(status)) :
                    DecorateResponseMessage(false, GetCommandsHandlerResponseStatusString(chrsOperationFailure));
            }
            else
            {
                string mode = silentMode ? "Silenced" : "UnSilenced";
                LOG_INFO << "Device:"<< arguments[0] <<"is now " << mode << endl;
                stringstream message;
                message << "Silent mode set to:" << silentMode;
                response.message = DecorateResponseMessage(true, message.str());
            }
        }
    }
    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

// **************************************UDP commands handlers*********************************************************** //
ResponseMessage CommandsHandler::GetHostNetworkInfo(vector<string> arguments, unsigned int numberOfArguments)
{
    //do something with params
    (void)numberOfArguments;
    LOG_VERBOSE << __FUNCTION__ << endl;
    ResponseMessage response;

    if (arguments.size() != 0)
    {
        response.message = DecorateResponseMessage(false, "Failed to get host's info: expected zero argument");
    }
    else
    {
        response.message = "GetHostIdentity;" + m_host.GetHostInfo().GetIps().m_ip + ";" + m_host.GetHostInfo().GetAlias();
    }

    response.type = REPLY_TYPE_BUFFER;
    response.length = response.message.size();
    return response;
}
// *************************************************************************************************

CommandsHandler::CommandsHandler(ServerType type, Host& host) :
    m_host(host)
{
    if (stTcp == type) // TCP server
    {
        m_functionHandler.insert(make_pair("get_interfaces", &CommandsHandler::GetInterfaces));
        m_functionHandler.insert(make_pair("open_interface", &CommandsHandler::OpenInterface));
        m_functionHandler.insert(make_pair("close_interface", &CommandsHandler::CloseInterface));
        m_functionHandler.insert(make_pair("r", &CommandsHandler::Read));
        m_functionHandler.insert(make_pair("rb", &CommandsHandler::ReadBlock));
        m_functionHandler.insert(make_pair("w", &CommandsHandler::Write));
        m_functionHandler.insert(make_pair("wb", &CommandsHandler::WriteBlock));
        m_functionHandler.insert(make_pair("interface_reset", &CommandsHandler::InterfaceReset));
        m_functionHandler.insert(make_pair("sw_reset", &CommandsHandler::SwReset));
        m_functionHandler.insert(make_pair("alloc_pmc", &CommandsHandler::AllocPmc));
        m_functionHandler.insert(make_pair("dealloc_pmc", &CommandsHandler::DeallocPmc));
        m_functionHandler.insert(make_pair("create_pmc_file", &CommandsHandler::CreatePmcFile));
        m_functionHandler.insert(make_pair("read_pmc_file", &CommandsHandler::FindPmcFile));
        m_functionHandler.insert(make_pair("send_wmi", &CommandsHandler::SendWmi));
        m_functionHandler.insert(make_pair("set_host_alias", &CommandsHandler::SetHostAlias));
        m_functionHandler.insert(make_pair("get_host_alias", &CommandsHandler::GetHostAlias));
        m_functionHandler.insert(make_pair("get_time", &CommandsHandler::GetTime));
        m_functionHandler.insert(make_pair("set_local_driver_mode", &CommandsHandler::SetDriverMode));
        m_functionHandler.insert(make_pair("get_host_manager_version", &CommandsHandler::GetHostManagerVersion));
        m_functionHandler.insert(make_pair("driver_control", &CommandsHandler::DriverControl));
        m_functionHandler.insert(make_pair("set_silence_mode", &CommandsHandler::SetDeviceSilenceMode));
        m_functionHandler.insert(make_pair("get_silence_mode", &CommandsHandler::GetDeviceSilenceMode));
    }
    else // UDP server
    {
        m_functionHandler.insert(make_pair(/*"get_host_network_info"*/"GetHostIdentity", &CommandsHandler::GetHostNetworkInfo));
    }
}

// *************************************************************************************************

ConnectionStatus CommandsHandler::ExecuteCommand(string message, ResponseMessage &referencedResponse)
{
    m_pMessageParser.reset(new MessageParser(message));

    string commandName = m_pMessageParser->GetCommandFromMessage();

    if (m_functionHandler.find(commandName) == m_functionHandler.end())
    { //There's no such a command, the return value from the map would be null
        LOG_WARNING << "Unknown command from client: " << commandName << endl;
        referencedResponse.message = "Unknown command: " + commandName;
        referencedResponse.length = referencedResponse.message.size();
        referencedResponse.type = REPLY_TYPE_BUFFER;
        return KEEP_CONNECTION_ALIVE;
    }
    referencedResponse = (this->*m_functionHandler[commandName])(m_pMessageParser->GetArgsFromMessage(), m_pMessageParser->GetNumberOfArgs()); //call the function that fits commandName

    return KEEP_CONNECTION_ALIVE;
}

// *************************************************************************************************

ConnectionStatus CommandsHandler::ExecuteBinaryCommand(uint8_t* binaryInput, ResponseMessage &referencedResponse)
{
    referencedResponse = GenericDriverIO(referencedResponse.internalParsedMessage, binaryInput, referencedResponse.inputBufSize);

    return KEEP_CONNECTION_ALIVE;
}


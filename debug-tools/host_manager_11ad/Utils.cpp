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

#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>


#ifdef __linux
#else
#define localtime_r(_Time, _Tm) localtime_s(_Tm, _Time)
#endif

using namespace std;

// *************************************************************************************************
vector<string> Utils::Split(string str, char delimiter)
{
    vector<string> splitStr;
    size_t nextSpacePosition = str.find_first_of(delimiter);
    while (string::npos != nextSpacePosition)
    {
        splitStr.push_back(str.substr(0, nextSpacePosition));
        str = str.substr(nextSpacePosition + 1);
        nextSpacePosition = str.find_first_of(delimiter);
    }

    if ("" != str)
    {
        splitStr.push_back(str);
    }
    return splitStr;
    /*
      vector<string> splitMessage;
      stringstream sstream(message);
      string word;
      while (getline(sstream, word, delim))
      {
      if (word.empty())
      { //don't push whitespace
      continue;
      }
      splitMessage.push_back(word);
      }
      return splitMessage;
    */
}

// *************************************************************************************************
string Utils::GetCurrentLocalTime()
{
    chrono::system_clock::time_point nowTimePoint = chrono::system_clock::now(); // get current time

    // convert epoch time to struct with year, month, day, hour, minute, second fields
    time_t now = chrono::system_clock::to_time_t(nowTimePoint);
    tm localTime;
    localtime_r(&now, &localTime);

    // get milliseconds field
    const chrono::duration<double> tse = nowTimePoint.time_since_epoch();
    chrono::seconds::rep milliseconds = chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;


    ostringstream currentTime;
    currentTime << (1900 + localTime.tm_year) << '-'
                << std::setfill('0') << std::setw(2) << (localTime.tm_mon + 1) << '-'
                << std::setfill('0') << std::setw(2) << localTime.tm_mday << ' '
                << std::setfill('0') << std::setw(2) << localTime.tm_hour << ':'
                << std::setfill('0') << std::setw(2) << localTime.tm_min << ':'
                << std::setfill('0') << std::setw(2) << localTime.tm_sec << '.'
                << std::setfill('0') << std::setw(3) << milliseconds;

    string currentTimeStr(currentTime.str());
    return currentTimeStr;
}

// *************************************************************************************************
bool Utils::ConvertHexStringToDword(string str, DWORD& word)
{
    if (str.find_first_of("0x") != 0) //The parameter is a hex string (assuming that a string starting with 0x must be hex)
    {
        return false;
    }
    istringstream s(str);
    s >> hex >> word;
    return true;
}

// *************************************************************************************************
bool Utils::ConvertHexStringToDwordVector(string str, char delimiter, vector<DWORD>& values)
{
    vector<string> strValues = Utils::Split(str, delimiter);
    values.reserve(strValues.size());
    for (auto& strValue : strValues)
    {
        DWORD word;
        if (Utils::ConvertHexStringToDword(strValue, word))
        {
            values.push_back(word);
        }
        else
        {
            return false;
        }
    }

    return true;
}

// *************************************************************************************************
bool Utils::ConvertDecimalStringToUnsignedInt(string str, unsigned int& ui)
{
    unsigned long l;
    try
    {
        l = strtoul(str.c_str(), nullptr, 10); // 10 for decimal base
    }
    catch (...)
    {
        return false;
    }

    ui = l;
    return true;
}


bool Utils::ConvertStringToBool(string str, bool& boolVal)
{
    string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

    if (lowerStr.compare("false") == 0)
    {
        boolVal = false;
        return true;
    }
    else if (lowerStr.compare("true") == 0)
    {
        boolVal = true;
        return true;
    }
    else
    {
        return false;
    }
}




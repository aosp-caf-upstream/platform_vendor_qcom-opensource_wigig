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

#include <array>
#include "JsonSerializeHelper.h"

JsonSerializeHelper::JsonSerializeHelper(std::ostream& os) : m_os(os)
{
    StartSerialization();
}

JsonSerializeHelper::~JsonSerializeHelper()
{
    FinishSerialization();
}

void JsonSerializeHelper::Serialize(const char* key, const std::string& value)
{
    Serialize(key, value.c_str());
}

void JsonSerializeHelper::Serialize(const char* key, const char* value)
{
    // last comma is overriden during Finish
    m_os << R"_(")_" << key << R"_(":")_" << value << R"_(",)_";
}

void JsonSerializeHelper::Serialize(const char* key, const unsigned long long value)
{
    // last comma is overriden during Finish
    m_os << R"_(")_" << key << R"_(":)_" << value << ",";
}

// Serialization service for composite member that implements the JsonSerializable interface
void JsonSerializeHelper::SerializeComposite(const char* key, const JsonSerializable& value)
{
    // last comma is overriden during Finish
    m_os << R"_(")_" << key << R"_(":)_";
    value.ToJson(m_os);
    m_os << ",";
}

void JsonSerializeHelper::SerializeCompositeArray(const char* key, const std::vector<std::unique_ptr<JsonSerializable>>& values)
{
    m_os << R"_(")_" << key << R"_(":[)_";

    bool first = true;
    for (auto & val : values)
    {
        if (!first) {
            m_os << ",";
        }
        first = false;
        if (val)
        {
            val->ToJson(m_os);
        }
        else
        {
            LOG_ERROR << "Got uninitialized vector. key is " << key << std::endl;
        }
    }
    m_os << "],";
}

// Encode given binary array as Base64 string
std::string JsonSerializeHelper::Base64Encode(const std::vector<unsigned char>& binaryData)
{
    return Base64Encode(binaryData.data(), binaryData.size());
}

std::string JsonSerializeHelper::Base64Encode(const unsigned char binaryData[], unsigned length)
{
    static const std::string base64Vocabulary = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    if (length == 0U)
    {
        return result;
    }

    // reserve room for the result (next higher multiple of 3, div by 3, mult by 4)
    unsigned mod3 = length % 3;
    result.reserve((length + (mod3 ? 3 - mod3 : 0)) * 4 / 3);

    int val1 = 0, val2 = -6;
    for (unsigned i = 0U; i < length; ++i)
    {
        val1 = (val1 << 8) + static_cast<int>(binaryData[i]);
        val2 += 8;
        while (val2 >= 0)
        {
            result.push_back(base64Vocabulary[(val1 >> val2) & 0x3F]);
            val2 -= 6;
        }
    }

    if (val2 > -6)
    {
        result.push_back(base64Vocabulary[((val1 << 8) >> (val2 + 8)) & 0x3F]);
    }

    while (result.size() % 4)
    {
        result.push_back('=');
    }

    return result;
}

 bool JsonSerializeHelper::Base64Decode(const std::string& base64Data, std::vector<unsigned char>& binaryData)
{
    static const unsigned char lookup[] =
    {
        62,  255, 62,  255, 63,  52,  53, 54, 55, 56, 57, 58, 59, 60, 61, 255,
        255, 0,   255, 255, 255, 255, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10,  11,  12,  13,  14,  15,  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
        255, 255, 255, 255, 63,  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36,  37,  38,  39,  40,  41,  42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };

    binaryData.clear();
    if (base64Data.empty())
    {
        return false;
    }
    binaryData.reserve(base64Data.size() * 3 / 4);

    int val1 = 0, val2 = -8;
    for (auto c : base64Data)
    {
        if (c < '+' || c > 'z')
        {
            break;
        }

        int mappedVal = static_cast<int>(lookup[c - '+']);
        if (mappedVal >= 64)
        {
            break;
        }

        val1 = (val1 << 6) + mappedVal;
        val2 += 6;

        if (val2 >= 0)
        {
            binaryData.push_back(char((val1 >> val2) & 0xFF));
            val2 -= 8;
        }
    }

    return true;
}

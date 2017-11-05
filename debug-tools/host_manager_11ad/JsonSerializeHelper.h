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
#ifndef _11AD_JSON_SERIALIZE_HELPER_H_
#define _11AD_JSON_SERIALIZE_HELPER_H_

#include <ostream>
#include <string>
#include <vector>
#include <memory>
#include "DebugLogger.h"

// Interface for composite types to be used as part of event payload
class JsonSerializable
{
public:
    virtual void ToJson(std::ostream& os) const = 0;
    virtual ~JsonSerializable() {}
};


// Helper class implementing the serialization services in Json format
class JsonSerializeHelper final
{
public:
    explicit JsonSerializeHelper(std::ostream& os);

    ~JsonSerializeHelper();

    void Serialize(const char* key, const std::string& value);

    void Serialize(const char* key, const char* value);

    void Serialize(const char* key, const unsigned long long value);

    // Serialization service for composite member that implements the JsonSerializable interface
    void SerializeComposite(const char* key, const JsonSerializable& value);

    void SerializeCompositeArray(const char* key, const std::vector<std::unique_ptr<JsonSerializable>>& values);

    // Type of template can be any iterable container
    template<class T>
    void SerializeStringArray(const char* key, const T& values)
    {
        m_os << R"_(")_" << key << R"_(":[)_";

        bool first = true;
        for (auto & val : values)
        {
            if (!first) {
                m_os << ",";
            }
            first = false;
            m_os << R"_(")_" << val << R"_(")_";
        }
        m_os << "],";
    }

    // Encode given binary array as Base64 string
    static std::string Base64Encode(const std::vector<unsigned char>& binaryData);
    static std::string Base64Encode(const unsigned char binaryData[], unsigned length);

    // Decode the string given in Base64 format
    static bool Base64Decode(const std::string& base64Data, std::vector<unsigned char>& binaryData);

private:
    std::ostream& m_os;

    void StartSerialization()
    {
        m_os << "{";
    }

    void  FinishSerialization()
    {
        m_os.seekp(-1, m_os.cur); // override the last comma
        m_os << "}";
    }
};

#endif // _11AD_JSON_SERIALIZE_HELPER_H_

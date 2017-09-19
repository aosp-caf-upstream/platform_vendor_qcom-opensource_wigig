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
#include "Device.h"

Device::Device(const string& deviceName) :
    m_basebandType(BASEBAND_TYPE_NONE),
    m_driver(AccessLayer::OpenDriver(deviceName)),
    m_isSilent(false),
    m_deviceName(deviceName)
{
    m_isAlive = ReadDeviceFwInfoInternal(m_fwVersion, m_fwTimestamp);
}

BasebandType Device::GetBasebandType()
{
    if (m_basebandType == BASEBAND_TYPE_NONE)
    {
        m_basebandType = ReadBasebandType();
    }

    return m_basebandType;
}

BasebandType Device::ReadBasebandType() const
{
    DWORD jtagVersion = Utils::REGISTER_DEFAULT_VALUE;
    const int rev_id_address = 0x880B34; //USER.JTAG.USER_USER_JTAG_1.dft_idcode_dev_id
    const int device_id_mask = 0x0fffffff; //take the first 28 bits from the Jtag Id
    BasebandType res = BASEBAND_TYPE_NONE;

    if (!m_driver->Read(rev_id_address, jtagVersion))
    {
        LOG_ERROR << "Failed to read baseband type" << "\n";
    }

    LOG_INFO << "JTAG rev ID = " << hex << jtagVersion << "\n";

    switch (jtagVersion & device_id_mask)
    {
    case 0x612072F:
        res = BASEBAND_TYPE_MARLON;
        break;
    case 0x632072F:
        res = BASEBAND_TYPE_SPARROW;
        break;
    case 0x642072F:
    case 0x007E0E1:
        res = BASEBAND_TYPE_TALYN;
        break;
    default:
        ////LOG_MESSAGE_WARN("Invalid device type - assuming Sparrow");
        res = BASEBAND_TYPE_SPARROW;
        break;
    }

    return res;
}

vector<unique_ptr<HostManagerEventBase>> Device::Poll(void)
{
    // TODO: implement polling: MB, logs, rgf

    vector<unique_ptr<HostManagerEventBase>> events;
    events.reserve(1U);
    lock_guard<mutex> lock(m_mutex);

    // Poll FW change
    FwVersion fwVersion;
    FwTimestamp fwTimestamp;

    m_isAlive = ReadDeviceFwInfoInternal(fwVersion, fwTimestamp);
    if (m_isAlive && !(m_fwVersion == fwVersion && m_fwTimestamp == fwTimestamp))
    {
        m_fwVersion = fwVersion;
        m_fwTimestamp = fwTimestamp;

        events.emplace_back(new NewDeviceDiscoveredEvent(GetDeviceName(), m_fwVersion, m_fwTimestamp));
    }

    return events;
}

// Internal service for fetching the FW version and compile time
// Note: The device lock should be acquired by the caller
bool Device::ReadDeviceFwInfoInternal(FwVersion& fwVersion, FwTimestamp& fwTimestamp) const
{
    // FW version
    bool readOk = m_driver->Read(FW_VERSION_MAJOR_REGISTER, fwVersion.m_major);
    readOk &= m_driver->Read(FW_VERSION_MINOR_REGISTER,        fwVersion.m_minor);
    readOk &= m_driver->Read(FW_VERSION_SUB_MINOR_REGISTER, fwVersion.m_subMinor);
    readOk &= m_driver->Read(FW_VERSION_BUILD_REGISTER,        fwVersion.m_build);

    // FW compile time
    readOk &= m_driver->Read(FW_TIMESTAMP_HOUR_REGISTER,    fwTimestamp.m_hour);
    readOk &= m_driver->Read(FW_TIMESTAMP_MINUTE_REGISTER,    fwTimestamp.m_min);
    readOk &= m_driver->Read(FW_TIMESTAMP_SECOND_REGISTER,    fwTimestamp.m_sec);
    readOk &= m_driver->Read(FW_TIMESTAMP_DAY_REGISTER,        fwTimestamp.m_day);
    readOk &= m_driver->Read(FW_TIMESTAMP_MONTH_REGISTER,    fwTimestamp.m_month);
    readOk &= m_driver->Read(FW_TIMESTAMP_YEAR_REGISTER,    fwTimestamp.m_year);

    if (!readOk)
    {
        LOG_ERROR << "Failed to read FW info for device " << GetDeviceName() << endl;
    }

    return readOk;
}

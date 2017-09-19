#ifndef _DRIVER_FACTORY_
#define _DRIVER_FACTORY_

#pragma once

#include <memory>

#include "Definitions.h"
#include "DriverAPI.h"
#include "DummyDriver.h"
#ifdef _WINDOWS
#include "WindowsPciDriver.h"
#include "JTagDriver.h"
#include "SerialDriver.h"
#elif __OS3__
#include "OS3DriverAPI.h"
#else
#include "UnixPciDriver.h"
#endif // _WINDOWS

class DriverFactory
{
public:

    static unique_ptr<DriverAPI> GetDriver(DeviceType deviceType, string interfaceName)
    {
        switch (deviceType)
        {
#ifdef _WINDOWS
        case JTAG:
            return unique_ptr<JTagDriver>(new JTagDriver(interfaceName));
        case SERIAL:
            return unique_ptr<SerialDriver>(new SerialDriver(interfaceName));
#endif // WINDOWS
        case DUMMY:
            return unique_ptr<DummyDriver>(new DummyDriver(interfaceName));
        case PCI:
#ifdef _WINDOWS
            return unique_ptr<WindowsDriverAPI>(new WindowsDriverAPI(interfaceName));
#elif __OS3__
            return unique_ptr<OS3DriverAPI>(new OS3DriverAPI(interfaceName));
#else
            return unique_ptr<UnixPciDriver>(new UnixPciDriver(interfaceName));
#endif // WINDOWS
        default:
            LOG_ERROR << "Got invalid device type. Return an empty driver" << endl;
            return unique_ptr<DriverAPI>(new DriverAPI(interfaceName));
        }
    }
};

#endif // !_DRIVER_FACTORY_

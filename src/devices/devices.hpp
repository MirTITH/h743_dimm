#pragma once

#include <stpp/device/io_device.hpp>
#include <memory>

namespace devices
{
    void InitDevices();
    extern std::unique_ptr<stpp::device::IoDevice> Uart1;
}

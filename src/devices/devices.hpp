#pragma once

#include <stpp/device/byte_device.hpp>
#include <memory>

namespace devices
{
    void InitDevices();
    extern std::unique_ptr<stpp::device::ByteDevice> Uart1;
}

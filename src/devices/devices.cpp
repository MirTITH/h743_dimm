#include "devices.hpp"
#include <stpp/device_framework/drivers/uart_driver.hpp>
#include <usart.h>

namespace devices
{
    // Device defines begin
    std::unique_ptr<stpp::device::ByteDevice> Uart1;

    // Device defines end

    void InitDevices()
    {
        using namespace stpp::driver;
        using namespace stpp::device;
        Uart1 = std::make_unique<ByteDevice>(std::make_unique<UartDriver>(&huart1), 1024);
        Uart1->Open();
    }
}
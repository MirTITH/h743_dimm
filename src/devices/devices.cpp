#include "devices.hpp"
#include <stpp/driver/uart_driver.hpp>
#include <usart.h>

namespace devices
{
    std::unique_ptr<stpp::device::ByteDevice> Uart1;

    template <typename dev_type, typename driver_ptr>
    static void MakeDevice(dev_type &device, driver_ptr &&driver)
    {
        device = std::make_unique<typename dev_type::element_type>(std::forward<driver_ptr>(driver));
    }

    void InitDevices()
    {
        using namespace stpp::driver;
        
        MakeDevice(Uart1, std::make_unique<UartDriver>(&huart1));
    }
}
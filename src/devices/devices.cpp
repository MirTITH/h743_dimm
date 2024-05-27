#include "devices.hpp"
#include <stpp/driver/driver_uart.hpp>
#include <usart.h>

namespace devices
{
    std::unique_ptr<stpp::device::IoDevice> Uart1;

    template <typename dev_type, typename driver_ptr>
    static void MakeDevice(dev_type &device, driver_ptr &&driver)
    {
        device = std::make_unique<typename dev_type::element_type>(std::forward<driver_ptr>(driver));
    }

    void InitDevices()
    {
        using namespace stpp::driver;

        MakeDevice(Uart1, std::make_unique<DriverUart>(huart1));
    }
}
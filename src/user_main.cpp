#include "user_main.h"
#include <cstring>
#include <main.h>
#include <FreeRTOS.h>
#include <task.h>
#include <usart.h>
#include <devices/devices.hpp>

void StartDefaultTask(void const *argument)
{
    (void)argument;

    devices::InitDevices();
    devices::Uart1->Open();

    const char str[] = "Hello, World!\r\n";

    while (true) {
        HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
        devices::Uart1->Write(str, 0, std::strlen(str), HAL_MAX_DELAY);
        // HAL_UART_Transmit(&huart1, (const uint8_t *)str, std::strlen(str), HAL_MAX_DELAY);
        // uart->Write(str, 0, std::strlen(str), HAL_MAX_DELAY);
        vTaskDelay(200);
    }

    vTaskDelete(nullptr); // 删除当前线程
}

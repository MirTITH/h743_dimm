#include "user_main.h"
#include <cstdio>
#include <main.h>
#include <FreeRTOS.h>
#include <task.h>
#include <usart.h>
#include <devices/devices.hpp>
#include <stpp/thread_priority_def.h>
#include <tim.h>
#include <HighPrecisionTime/high_precision_time.h>
#include <vector>
// #include <stpp/freertos_allocator.hpp>

// void ByteDeviceDaemon(void *argument)
// {
//     auto uart = (stpp::device::ByteDevice *)argument;
//     uart->Spin();
// }

// void WriteTask(void *argument)
// {
//     auto prefix_str = (const char *)argument;
//     int count       = 0;
//     while (true) {
//         char str[32];
//         auto length = std::snprintf(str, sizeof(str), "%s: %d\n", prefix_str, count++);
//         devices::Uart1->Write(str, length);
//         vTaskDelay(10);
//     }
// }

void StartDefaultTask(void const *argument)
{
    (void)argument;

    HPT_Init();
    devices::InitDevices();

    // extern void test_main();
    // test_main();

    // stpp::LimitedAllocator<int> alloc(1024);

    // for (int i = 0; i < 10; i++) {
    //     vec.push_back(i);
    // }

    // devices::InitDevices();
    // xTaskCreate(ByteDeviceDaemon, "Uart1Daemon", 256, devices::Uart1.get(), PriorityNormal, nullptr);
    // vTaskDelay(1000);

    HAL_TIM_Base_Start_IT(&htim6);

    char str[64];

    while (true) {
        HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
        auto len = std::snprintf(str, sizeof(str), "\t\t\t\t\t\tAllocated size: %u\n", devices::Uart1->mem_.GetAllocatedSize());
        devices::Uart1->Write(str, len);
        // HAL_UART_Transmit(&huart1, (const uint8_t *)str, std::strlen(str), HAL_MAX_DELAY);
        // uart->Write(str, 0, std::strlen(str), HAL_MAX_DELAY);
        // HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)str, std::strlen(str));
        vTaskDelay(100);
    }

    vTaskDelete(nullptr); // 删除当前线程
}

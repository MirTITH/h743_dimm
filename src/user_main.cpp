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

void ByteDeviceDaemon(void *argument)
{
    auto uart = (stpp::device::ByteDevice *)argument;
    uart->Spin();
}

void WriteTask(void *argument)
{
    auto prefix_str = (const char *)argument;
    int count       = 0;
    while (true) {
        char str[32];
        auto length = std::snprintf(str, sizeof(str), "%s: %d\n", prefix_str, count++);
        devices::Uart1->Write(str, length);
        vTaskDelay(10);
    }
}

void StartDefaultTask(void const *argument)
{
    (void)argument;

    extern void test_main();
    test_main();

    HPT_Init();

    devices::InitDevices();
    xTaskCreate(ByteDeviceDaemon, "Uart1Daemon", 256, devices::Uart1.get(), PriorityNormal, nullptr);
    vTaskDelay(1000);

    HAL_TIM_Base_Start_IT(&htim6);

    xTaskCreate(WriteTask, "WriteTask1", 256, (void *)"WriteTask1", PriorityNormal, nullptr);
    xTaskCreate(WriteTask, "WriteTask2", 256, (void *)"Hello WriteTask2", PriorityNormal, nullptr);
    xTaskCreate(WriteTask, "WriteTask3", 256, (void *)"你好！😀WriteTask3", PriorityNormal, nullptr);

    while (true) {
        HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
        // HAL_UART_Transmit(&huart1, (const uint8_t *)str, std::strlen(str), HAL_MAX_DELAY);
        // uart->Write(str, 0, std::strlen(str), HAL_MAX_DELAY);
        // HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)str, std::strlen(str));
        vTaskDelay(200);
    }

    vTaskDelete(nullptr); // 删除当前线程
}

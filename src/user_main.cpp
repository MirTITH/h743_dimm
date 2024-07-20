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

// void CallbackFunc(stpp::ErrorCode e = stpp::ErrorCode::OK)
// {
//     (void)e;
//     devices::Uart1->SyncWrite("Called CallbackFunc\n");
//     devices::Uart1->AsyncRead(str, sizeof(str), CallbackFunc);
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

    // devices::Uart1->AsyncRead(str, 64, [str](stpp::ErrorCode) {

    // });

    // CallbackFunc();

    char read_buf[4] = {};
    devices::Uart1->AsyncRead(read_buf, sizeof(read_buf) - 1, [&read_buf](stpp::ErrorCode) {
        char str[20];
        auto len = std::snprintf(str, sizeof(str), "AsyncRead: %s\n", read_buf);
        devices::Uart1->AsyncWrite(str, len);
    });

    while (true) {
        HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);

        constexpr char str[] = "Hello World!\n";
        devices::Uart1->SyncWrite(str, std::strlen(str));

        devices::Uart1->SyncWrite("You can put any string here\n");

        auto callback_func = [](stpp::ErrorCode e) {
            char str[40];
            auto len = std::snprintf(str, sizeof(str), "AllocatedSize: %u\n", devices::Uart1->GetAllocatedSize());
            devices::Uart1->AsyncWrite(str, len);
            switch (e) {
                case stpp::ErrorCode::OK:
                    devices::Uart1->AsyncWrite("AsyncWrite complete!\n");
                    break;
                case stpp::ErrorCode::ERROR:
                    devices::Uart1->AsyncWrite("AsyncWrite timeout!\n");
                    break;
                default:
                    devices::Uart1->AsyncWrite("AsyncWrite unknown error!\n");
                    break;
            }
        };

        devices::Uart1->AsyncWrite("AsyncWriting\n", callback_func);
        devices::Uart1->AsyncWriteNoCopy("AsyncWritingNoCopy\n", callback_func);

        char read_buf2[4] = {};
        devices::Uart1->SyncRead(read_buf2, sizeof(read_buf2) - 1); // sizeof(read_buf) - 1 保证最后一个字节一定是 \0
        devices::Uart1->SyncWrite("SyncRead: ");
        devices::Uart1->SyncWrite(read_buf2);
        devices::Uart1->SyncWrite("\n");

        // // HAL_UART_Transmit(&huart1, (const uint8_t *)str, std::strlen(str), HAL_MAX_DELAY);
        // uart->Write(str, 0, std::strlen(str), HAL_MAX_DELAY);
        // HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)str, std::strlen(str));
        // devices::Uart1->SyncWrite(str, sizeof(str));
        // devices::Uart1->SyncWrite("\n");
        vTaskDelay(1000);
    }

    vTaskDelete(nullptr); // 删除当前线程
}

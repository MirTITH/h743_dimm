# STPP Library

## 什么是 STPP

STPP (STm32 c Plus Plus library) 是为 STM32 平台编写的 C++ 库。这个库旨在提供许多便利的算法和方法。另外，此库的目的并不是代替 STM32 官方的 HAL 库，而是作为日常开发的补充。

## 如何添加 STPP 到工程中

1. 准备好 stpp 的代码，将 stpp 文件夹复制到工程中
2. 将 stpp 文件夹所在的父路径添加包含路径中
3. 将 stpp 文件夹中的所有 .c 和 .cpp 文件添加到编译列表

## 使用指南

### Device Framework

这是一个设备框架，可以实现外设的异步或同步读写

当前只支持串口设备

#### 配置

1. 配置 CubeMX

   1. 使能串口，打开发送和接收的 DMA，Mode: Normal
   2. 打开该串口的所有 DMA 中断和 global interrupt
   3. 生成代码

2. 定义设备

   ```cpp
   // devices.cpp
   
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
   ```

   ```cpp
   // devices.hpp
   
   #pragma once
   
   #include <stpp/device_framework/byte_device.hpp>
   #include <memory>
   
   namespace devices
   {
       void InitDevices();
       extern std::unique_ptr<stpp::device::ByteDevice> Uart1;
   }
   
   ```

2. 设置回调函数

   ```cpp
   // user_irq.cpp
   
   #include <main.h>
   #include <devices/devices.hpp>
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
   void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
   #ifdef __cplusplus
   }
   #endif
   
   void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
   {
       if (huart->Instance == USART1) {
           devices::Uart1->GetDriver()->HardwareTxCpltCallback();
       }
   }
   
   void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
   {
       if (huart->Instance == USART1) {
           devices::Uart1->GetDriver()->HardwareRxCpltCallback();
       }
   }
   ```

3. 初始化设备

   ```cpp
   // user_main.cpp
   
   #include <devices/devices.hpp>
   
   void user_main(){
       // ...
       // 在HAL库初始化完毕设备，使用设备之前调用
       devices::InitDevices();
       // ...
   }
   ```

#### 用法示例

```cpp
#include <cstdio>
#include <devices/devices.hpp>

// 同步写入，函数会在写入完成后返回（不能在中断中调用）
constexpr char str[] = "Hello World!\n";
devices::Uart1->SyncWrite(str, std::strlen(str));

devices::Uart1->SyncWrite("You can put any string here\n");


// 异步写入，函数会立刻返回，由守护线程进行写入
devices::Uart1->AsyncWrite("AsyncWriting\n");

// 异步写入也可以传入一个回调函数
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

// 如果可以保证传入的数据在写入完成前一直有效，可以使用 NoCopy 函数提高效率：
devices::Uart1->AsyncWriteNoCopy("AsyncWritingNoCopy\n");

// 同步读取
char sync_read_buf[4] = {};
devices::Uart1->SyncRead(sync_read_buf, sizeof(sync_read_buf) - 1); // sizeof(sync_read_buf) - 1 保证最后一个字节一定是 \0

devices::Uart1->SyncWrite("SyncRead: ");
devices::Uart1->SyncWrite(sync_read_buf);
devices::Uart1->SyncWrite("\n");

// 异步读取
char read_buf[4] = {};
devices::Uart1->AsyncRead(read_buf, sizeof(read_buf) - 1, [&read_buf](stpp::ErrorCode) {
    char str[20];
    auto len = std::snprintf(str, sizeof(str), "AsyncRead: %s\n", read_buf);
    devices::Uart1->AsyncWrite(str, len);
});
```


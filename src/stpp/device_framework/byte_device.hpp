#pragma once

#include <cstring>
#include <FreeRTOS.h>
#include <stdexcept>
#include <task.h>
#include <list>
#include <mutex>
#include "drivers/byte_driver.hpp"
#include "../freertos_lock.hpp"
#include <memory>
#include <functional>
#include "../error_code.hpp"
#include "../freertos_memory.hpp"
#include <queue>
#include "private_include/callback_func.hpp"
#include "private_include/data_to_process.hpp"
#include "../freertos_delay_ms.h"
#include "private_include/byte_device_daemon_task.hpp"

namespace stpp
{
    namespace device
    {
        class ByteDevice
        {
            using Mallocator_t       = stpp::MallocLimited<std::malloc, std::free>;
            using TxDataWithCallback = device_framework_internal::TxDataWithCallback<Mallocator_t>;
            using RxDataWithCallback = device_framework_internal::RxDataWithCallback;
            using CallbackFunc_t     = device_framework_internal::CallbackFunc_t;

        public:
            /**
             * @brief 构造一个字节设备
             *
             * @param driver 字节驱动
             * @param mem_limit 内部缓冲区大小，单位字节
             */
            ByteDevice(std::unique_ptr<driver::ByteDriver> driver, std::size_t mem_limit = 1024)
                : driver_(std::move(driver)), mem_(mem_limit),
                  tx_queue_(AllocatorLimited<TxDataWithCallback, Mallocator_t>(mem_)), rx_queue_(AllocatorLimited<RxDataWithCallback, Mallocator_t>(mem_)) {};

            ByteDevice(ByteDevice &&)                 = delete;
            ByteDevice(const ByteDevice &)            = delete;
            ByteDevice &operator=(ByteDevice &&)      = delete;
            ByteDevice &operator=(const ByteDevice &) = delete;
            ~ByteDevice()                             = default;

            /**
             * @brief 打开设备，启动读写守护线程
             *
             * @param daemon_thread_name 线程名称
             */
            void Open(const char *const daemon_thread_name = "ByteDevice")
            {
                auto result = xTaskCreate(device_framework_internal::ByteDeviceDaemon, daemon_thread_name, 512, this, 3, &spin_task_handle_);

                if (result != pdPASS) {
                    throw std::runtime_error("Failed to create ByteDevice daemon task");
                }
            }

            /**
             * @brief 同步读取，线程会阻塞直到数据读取完成。不能在中断上下文中调用。
             *
             * @param data 读取到的数据会保存在这里
             * @param length 读取数据的长度，单位字节
             * @param timeout 超时时间，单位 ms。注意：超时返回后，数据可能还在接收中（data 可能还在被写入）
             * @return true 读取成功
             * @return false 读取失败
             */
            bool SyncRead(void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                if (InHandlerMode()) {
                    throw std::runtime_error("SyncRead() can't be called in interrupt context. Use AsyncRead() instead.");
                }

                auto current_task_handle = xTaskGetCurrentTaskHandle();

                auto is_success = AsyncRead(data, length, [current_task_handle](stpp::ErrorCode) {
                    if (InHandlerMode()) {
                        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                        vTaskNotifyGiveFromISR(current_task_handle, &xHigherPriorityTaskWoken);
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                    } else {
                        xTaskNotifyGive(current_task_handle);
                    }
                });

                if (is_success) {
                    ulTaskNotifyTake(pdTRUE, FreeRtosMsToTick(timeout)); // 等待数据发送完成
                }

                return is_success;
            }

            /**
             * @brief 同步写入，线程会阻塞直到数据发送完成。不能在中断上下文中调用。
             *
             * @param data 要发送的数据
             * @param length 数据长度，单位字节
             * @param timeout 超时时间，单位 ms。注意：超时返回后，数据可能还在发送中
             * @return true 写入成功
             * @return false 写入失败
             */
            bool SyncWrite(const void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                if (InHandlerMode()) {
                    throw std::runtime_error("SyncWrite() can't be called in interrupt context. Use AsyncWrite() instead.");
                }

                auto current_task_handle = xTaskGetCurrentTaskHandle();

                auto is_success = AsyncWriteNoCopy(data, length, [current_task_handle](stpp::ErrorCode) {
                    if (InHandlerMode()) {
                        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                        vTaskNotifyGiveFromISR(current_task_handle, &xHigherPriorityTaskWoken);
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                    } else {
                        xTaskNotifyGive(current_task_handle);
                    }
                });

                if (is_success) {
                    ulTaskNotifyTake(pdTRUE, FreeRtosMsToTick(timeout)); // 等待数据发送完成
                }

                return is_success;
            }

            /**
             * @brief 同步写入，线程会阻塞直到数据发送完成。不能在中断上下文中调用。
             *
             * @param str 要发送的数据
             * @param timeout 超时时间，单位 ms。注意：超时返回后，数据可能还在发送中
             * @return true 写入成功
             * @return false 写入失败
             */
            bool SyncWrite(const std::string_view str, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                if (InHandlerMode()) {
                    throw std::runtime_error("SyncWrite() can't be called in interrupt context. Use AsyncWrite() instead.");
                }

                auto current_task_handle = xTaskGetCurrentTaskHandle();

                auto is_success = AsyncWriteNoCopy(str.data(), str.length(), [current_task_handle](stpp::ErrorCode) {
                    if (InHandlerMode()) {
                        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                        vTaskNotifyGiveFromISR(current_task_handle, &xHigherPriorityTaskWoken);
                        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                    } else {
                        xTaskNotifyGive(current_task_handle);
                    }
                });

                if (is_success) {
                    ulTaskNotifyTake(pdTRUE, FreeRtosMsToTick(timeout)); // 等待数据发送完成
                }

                return true;
            }

            /**
             * @brief 异步读取，不会阻塞。可以在中断上下文中调用。
             *
             * @param data 读取到的数据会保存在这里
             * @param length 数据长度，单位字节
             * @return true 成功
             * @return false 失败
             */
            bool AsyncRead(void *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
            {
                try {
                    RxDataWithCallback data_with_cb(static_cast<uint8_t *>(data), length, std::move(callback));
                    {
                        std::lock_guard lock(rx_queue_.lock);
                        rx_queue_.queue.push(std::move(data_with_cb));
                    }
                    NotifySpinTask();
                    return true;
                } catch (const std::exception &e) {
                    return false;
                }
            }

            /**
             * @brief 异步写入，不会阻塞。可以在中断上下文中调用。
             *
             * @param data 要发送的数据
             * @param length 数据长度，单位字节
             * @return true 写入成功
             * @return false 写入失败
             */
            bool AsyncWrite(const void *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
            {
                try {
                    TxDataWithCallback data_with_cb(mem_, static_cast<const uint8_t *>(data), length, std::move(callback));
                    {
                        std::lock_guard lock(tx_queue_.lock);
                        tx_queue_.queue.push(std::move(data_with_cb));
                    }
                    NotifySpinTask();
                    return true;
                } catch (const std::exception &e) {
                    return false;
                }
            }

            /**
             * @brief 异步写入，不会阻塞。可以在中断上下文中调用。
             *
             * @param str 要发送的数据
             * @return true 写入成功
             * @return false 写入失败
             */
            bool AsyncWrite(const std::string_view str, CallbackFunc_t callback = CallbackFunc_t())
            {
                try {
                    TxDataWithCallback data_with_cb(mem_, reinterpret_cast<const uint8_t *>(str.data()), str.length(), std::move(callback));
                    {
                        std::lock_guard lock(tx_queue_.lock);
                        tx_queue_.queue.push(std::move(data_with_cb));
                    }
                    NotifySpinTask();
                    return true;
                } catch (const std::exception &e) {
                    return false;
                }
            }

            /**
             * @brief 异步写入，不会阻塞。可以在中断上下文中调用。这个函数不会将 data 复制到内部缓冲区，而是直接使用 data。data 必须保证在回调函数被调用之前一直有效。
             *
             * @param data 要发送的数据
             * @param length 数据长度，单位字节
             * @return true 写入成功
             * @return false 写入失败
             */
            bool AsyncWriteNoCopy(const void *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
            {
                try {
                    TxDataWithCallback data_with_cb(static_cast<const uint8_t *>(data), length, std::move(callback));
                    {
                        std::lock_guard lock(tx_queue_.lock);
                        tx_queue_.queue.push(std::move(data_with_cb));
                    }
                    NotifySpinTask();
                    return true;
                } catch (const std::exception &e) {
                    return false;
                }
            }

            /**
             * @brief 异步写入，不会阻塞。可以在中断上下文中调用。这个函数不会将 data 复制到内部缓冲区，而是直接使用 data。data 必须保证在回调函数被调用之前一直有效。
             *
             * @param str 要发送的数据
             * @return true 写入成功
             * @return false 写入失败
             */
            bool AsyncWriteNoCopy(const std::string_view str, CallbackFunc_t callback = CallbackFunc_t())
            {
                try {
                    TxDataWithCallback data_with_cb(reinterpret_cast<const uint8_t *>(str.data()), str.length(), std::move(callback));
                    {
                        std::lock_guard lock(tx_queue_.lock);
                        tx_queue_.queue.push(std::move(data_with_cb));
                    }
                    NotifySpinTask();
                    return true;
                } catch (const std::exception &e) {
                    return false;
                }
            }

            driver::ByteDriver *GetDriver() const
            {
                return driver_.get();
            }

            /**
             * @brief 获取已分配的内存大小
             *
             * @return std::size_t
             */
            std::size_t GetAllocatedSize() const
            {
                return mem_.GetAllocatedSize();
            }

        protected:
            std::unique_ptr<driver::ByteDriver> driver_;

            Mallocator_t mem_;

            template <typename Data_t>
            class QueueWithLock
            {
                using DataList_t = std::list<Data_t, AllocatorLimited<Data_t, Mallocator_t>>;

            public:
                stpp::CriticalSection lock;
                std::queue<Data_t, DataList_t> queue;

                QueueWithLock(AllocatorLimited<Data_t, Mallocator_t> allocator)
                    : queue(std::move(allocator)) {};
            };

            using TxQueueWithLock = QueueWithLock<TxDataWithCallback>;
            using RxQueueWithLock = QueueWithLock<RxDataWithCallback>;

            TxQueueWithLock tx_queue_;
            stpp::BinarySemphr tx_sem_{true}; // 当 driver_ 正在发送数据时，上锁

            RxQueueWithLock rx_queue_;
            stpp::BinarySemphr rx_sem_{true}; // 当 driver_ 正在接收数据时，上锁

            TaskHandle_t spin_task_handle_ = nullptr;

            void NotifySpinTaskFromThread()
            {
                assert(spin_task_handle_ != nullptr); // 你可能忘记了调用 Open()
                xTaskNotifyGive(spin_task_handle_);
            }

            void NotifySpinTaskFromISR()
            {
                assert(spin_task_handle_ != nullptr); // 你可能忘记了调用 Open()

                // 通知守护线程传输
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                vTaskNotifyGiveFromISR(spin_task_handle_, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }

            void NotifySpinTask()
            {
                if (InHandlerMode()) {
                    NotifySpinTaskFromISR();
                } else {
                    NotifySpinTaskFromThread();
                }
            }

            friend void stpp::device_framework_internal::ByteDeviceDaemon(void *argument);

            void Spin()
            {
                TxDataWithCallback tx_data;
                RxDataWithCallback rx_data;

                NotifySpinTaskFromThread(); // 防止在 Spin 之前有数据

                while (true) {
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                    if (tx_sem_.lock_from_thread(0)) { // driver_ 能够发送数据
                        std::unique_lock lock{tx_queue_.lock};
                        auto queue_size = tx_queue_.queue.size();
                        if (queue_size > 0) { // 有数据需要发送
                            tx_data = std::move(tx_queue_.queue.front());
                            tx_queue_.queue.pop();
                            lock.unlock();

                            this->driver_->SetWriteCpltCb([this, &tx_data](stpp::ErrorCode ec) {
                                if (tx_data.callback_) {
                                    tx_data.callback_(ec);
                                }
                                this->tx_sem_.unlock();
                            });

                            this->driver_->AsyncWrite(tx_data.data_.get(), tx_data.length_);

                            if (queue_size > 1) {
                                NotifySpinTaskFromThread(); // 还有数据需要发送，下一轮继续检查
                            }
                        } else {
                            tx_sem_.unlock();
                        }
                    } else {
                        NotifySpinTaskFromThread(); // driver_ 正忙，下一轮继续检查
                        vTaskDelay(1);
                    }

                    if (rx_sem_.lock_from_thread(0)) { // driver_ 能够接收数据
                        std::unique_lock lock{rx_queue_.lock};
                        auto queue_size = rx_queue_.queue.size();
                        if (queue_size > 0) { // 有数据需要接收
                            rx_data = std::move(rx_queue_.queue.front());
                            rx_queue_.queue.pop();
                            lock.unlock();

                            this->driver_->SetReadCpltCb([this, &rx_data](stpp::ErrorCode ec) {
                                if (rx_data.callback_) {
                                    rx_data.callback_(ec);
                                }
                                this->rx_sem_.unlock();
                            });

                            this->driver_->AsyncRead(rx_data.data_.get(), rx_data.length_);

                            if (queue_size > 1) {
                                NotifySpinTaskFromThread(); // 还有数据需要发送，下一轮继续检查
                            }
                        } else {
                            rx_sem_.unlock();
                        }
                    } else {
                        NotifySpinTaskFromThread(); // driver_ 正忙，下一轮继续检查
                        vTaskDelay(1);
                    }
                }
            }
        };

    }
}
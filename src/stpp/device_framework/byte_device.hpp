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
            ByteDevice(std::unique_ptr<driver::ByteDriver> driver, std::size_t mem_limit = 1024)
                : driver_(std::move(driver)), mem_(mem_limit),
                  tx_queue_(AllocatorLimited<TxDataWithCallback, Mallocator_t>(mem_)), rx_queue_(AllocatorLimited<RxDataWithCallback, Mallocator_t>(mem_)) {};

            ByteDevice(ByteDevice &&)                 = delete;
            ByteDevice(const ByteDevice &)            = delete;
            ByteDevice &operator=(ByteDevice &&)      = delete;
            ByteDevice &operator=(const ByteDevice &) = delete;
            ~ByteDevice()                             = default;

            void Open(const char *const daemon_thread_name = "ByteDevice")
            {
                auto result = xTaskCreate(device_framework_internal::ByteDeviceDaemon, daemon_thread_name, 512, this, 3, &spin_task_handle_);

                if (result != pdPASS) {
                    throw std::runtime_error("Failed to create ByteDevice daemon task");
                }
            }

            // std::size_t Read(void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            // {
            //     // std::lock_guard lock(rx_sem_);
            //     // // This is a blocking call, which will not cause callback to be called
            //     // return driver_->Read(data, length, timeout);
            // }

            bool Write(const void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max()) noexcept
            {
                if (InHandlerMode()) {
                    return false;
                }

                auto current_task_handle = xTaskGetCurrentTaskHandle();

                auto is_success = AsyncWrite(data, length, [current_task_handle](stpp::ErrorCode) {
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

            // bool AsyncRead(void *data, std::size_t length)
            // {
            //     // if (InHandlerMode()) {
            //     //     auto uxSavedInterruptStatus      = taskENTER_CRITICAL_FROM_ISR();
            //     //     const auto [buffer, buffer_size] = rx_buffer_.Expand(length);
            //     //     taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
            //     //     NotifySpinTaskFromISR();
            //     //     return true;

            //     // } else {
            //     //     taskENTER_CRITICAL();
            //     //     const auto [buffer, buffer_size] = rx_buffer_.Expand(length);
            //     //     taskEXIT_CRITICAL();
            //     //     NotifySpinTaskFromThread();
            //     //     return true;
            //     // }
            // }

            // bool WaitForWriteComplete(uint32_t timeout_ms = std::numeric_limits<uint32_t>::max())
            // {
            //     // return tx_sem_.Take(timeout);
            // }

            // bool WaitForReadComplete(uint32_t timeout_ms = std::numeric_limits<uint32_t>::max())
            // {
            //     // return rx_sem_.Take(timeout);
            // }

            void Spin()
            {
                TxDataWithCallback tx_data;
                RxDataWithCallback rx_data;

                NotifySpinTaskFromThread(); // 防止在 Spin 之前有数据

                while (true) {
                    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

                    if (tx_sem_.lock_from_thread(0)) { // driver_ 能够发送数据
                        std::unique_lock lock{tx_queue_.lock};
                        if (tx_queue_.queue.size() > 0) { // 有数据需要发送
                            tx_data = tx_queue_.queue.front();
                            tx_queue_.queue.pop();
                            lock.unlock();

                            this->driver_->SetWriteCpltCb([this, &tx_data](stpp::ErrorCode ec) {
                                if (tx_data.callback_) {
                                    tx_data.callback_(ec);
                                }
                                this->tx_sem_.unlock();
                            });

                            this->driver_->AsyncWrite(tx_data.data_.get(), tx_data.length_);
                        } else {
                            tx_sem_.unlock();
                        }
                    } else {
                        NotifySpinTaskFromThread(); // driver_ 正忙，下一轮继续检查
                        vTaskDelay(1);
                    }

                    if (rx_sem_.lock_from_thread(0)) { // driver_ 能够接收数据
                        std::unique_lock lock{rx_queue_.lock};
                        if (rx_queue_.queue.size() > 0) { // 有数据需要接收
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
                        } else {
                            rx_sem_.unlock();
                        }
                    } else {
                        NotifySpinTaskFromThread(); // driver_ 正忙，下一轮继续检查
                        vTaskDelay(1);
                    }
                }
            }

            // void SetWriteCallback(CallbackFunc_t cb)
            // {
            //     write_cplt_cb_ = std::move(cb);
            // }

            // void SetReadCallback(CallbackFunc_t cb)
            // {
            //     read_cplt_cb_ = std::move(cb);
            // }

            // void WriteCpltCallback()
            // {
            //     auto uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
            //     tx_buffer_.Shrink(last_write_length_);
            //     taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
            //     tx_lock_.unlock();
            // }

            // void ReadCpltCallback()
            // {
            //     rx_lock_.unlock();
            // }

            driver::ByteDriver *GetDriver() const
            {
                return driver_.get();
            }

        public:
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
        };

    }
}
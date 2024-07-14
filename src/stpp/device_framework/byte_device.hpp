#pragma once

#include <cstring>
#include <FreeRTOS.h>
#include <mutex>
#include <task.h>
#include "drivers/byte_driver.hpp"
#include "../freertos_lock.hpp"
#include <memory>
#include <functional>
#include "../error_code.hpp"
#include "../freertos_queue.hpp"
#include "../freertos_allocator.hpp"
#include <queue>
#include <atomic>

namespace stpp
{
    namespace device
    {
        class ByteDevice
        {
            using CallbackFunc_t = std::function<void(stpp::ErrorCode)>;

        public:
            ByteDevice(std::unique_ptr<driver::ByteDriver> driver, std::size_t mem_limit = 1024)
                : driver_(std::move(driver)), mem_limit_(mem_limit){};

            ByteDevice(ByteDevice &&)                 = default;
            ByteDevice(const ByteDevice &)            = delete;
            ByteDevice &operator=(ByteDevice &&)      = delete;
            ByteDevice &operator=(const ByteDevice &) = delete;
            ~ByteDevice()                             = default;

            std::size_t Read(void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                // std::lock_guard lock(rx_sem_);
                // // This is a blocking call, which will not cause callback to be called
                // return driver_->Read(data, length, timeout);
            }

            std::size_t Write(const void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                if (InHandlerMode()) {
                    auto uxSavedInterruptStatus      = taskENTER_CRITICAL_FROM_ISR();
                    const auto [buffer, buffer_size] = tx_buffer_.Expand(length);
                    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                    std::memcpy(buffer, data, buffer_size);
                    NotifySpinTaskFromISR();
                    return buffer_size;

                } else {
                    taskENTER_CRITICAL();
                    const auto [buffer, buffer_size] = tx_buffer_.Expand(length);
                    taskEXIT_CRITICAL();
                    std::memcpy(buffer, data, buffer_size);
                    NotifySpinTaskFromThread();
                    return buffer_size;
                }
            }

            bool AsyncWrite(const void *data, std::size_t length)
            {
                // if (InHandlerMode()) {
                //     auto uxSavedInterruptStatus      = taskENTER_CRITICAL_FROM_ISR();
                //     const auto [buffer, buffer_size] = tx_buffer_.Expand(length);
                //     taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                //     std::memcpy(buffer, data, buffer_size);
                //     NotifySpinTaskFromISR();
                //     return true;

                // } else {
                //     taskENTER_CRITICAL();
                //     const auto [buffer, buffer_size] = tx_buffer_.Expand(length);
                //     taskEXIT_CRITICAL();
                //     std::memcpy(buffer, data, buffer_size);
                //     NotifySpinTaskFromThread();
                //     return true;
                // }
            }

            bool AsyncRead(void *data, std::size_t length)
            {
                // if (InHandlerMode()) {
                //     auto uxSavedInterruptStatus      = taskENTER_CRITICAL_FROM_ISR();
                //     const auto [buffer, buffer_size] = rx_buffer_.Expand(length);
                //     taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                //     NotifySpinTaskFromISR();
                //     return true;

                // } else {
                //     taskENTER_CRITICAL();
                //     const auto [buffer, buffer_size] = rx_buffer_.Expand(length);
                //     taskEXIT_CRITICAL();
                //     NotifySpinTaskFromThread();
                //     return true;
                // }
            }

            bool WaitForWriteComplete(uint32_t timeout_ms = std::numeric_limits<uint32_t>::max())
            {
                // return tx_sem_.Take(timeout);
            }

            bool WaitForReadComplete(uint32_t timeout_ms = std::numeric_limits<uint32_t>::max())
            {
                // return rx_sem_.Take(timeout);
            }

            void Spin()
            {
                spin_task_handle_ = xTaskGetCurrentTaskHandle();

                DataToProcess tx_data;
                DataToProcess rx_data;

                NotifySpinTaskFromThread(); // 防止在 Spin 之前有数据

                while (true) {
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                    if (tx_sem_.lock_from_thread(0)) { // driver_ 能够发送数据
                        std::unique_lock lock{tx_queue_.lock};
                        if (tx_queue_.queue.size() > 0) { // 有数据需要发送
                            tx_data = std::move(tx_queue_.queue.front());
                            tx_queue_.queue.pop();
                            lock.unlock();

                            this->driver_->SetWriteCpltCb([this, &tx_data](stpp::ErrorCode ec) {
                                if (tx_data.callback_) {
                                    tx_data.callback_(ec);
                                    tx_data.Clear();
                                    this->tx_sem_.unlock();
                                }
                            });

                            this->driver_->AsyncWrite(tx_data.data_.get(), tx_data.length_);
                        }
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
                                    rx_data.Clear();
                                    this->rx_sem_.unlock();
                                }
                            });

                            this->driver_->AsyncRead(rx_data.data_.get(), rx_data.length_);
                        }
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

        protected:
            class DataToProcess
            {
            public:
                std::shared_ptr<uint8_t[]> data_;
                size_t length_;
                CallbackFunc_t callback_;

                DataToProcess()
                    : data_(nullptr), length_(0), callback_(){};

                DataToProcess(void *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                    : data_(static_cast<uint8_t *>(pvPortMalloc(length)), vPortFree), length_(length), callback_(std::move(callback))
                {
                    // TODO 考虑内存分配失败的情况
                    std::memcpy(data_.get(), data, length);
                }

                DataToProcess(std::shared_ptr<uint8_t[]> data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                    : data_(std::move(data)), length_(length), callback_(std::move(callback)){};

                void Clear()
                {
                    data_.reset();
                    length_   = 0;
                    callback_ = CallbackFunc_t();
                }

                bool IsEmpty() const
                {
                    return length_ == 0;
                }
            };

            struct QueueWithLock {
                std::queue<DataToProcess> queue;
                stpp::RecursiveMutex lock;
            };

            std::unique_ptr<driver::ByteDriver> driver_;

            const std::size_t mem_limit_;
            std::atomic<std::size_t> used_mem_ = 0;
            stpp::BinarySemphr mem_sem_{true}; // 当内存不足时，上锁

            QueueWithLock tx_queue_;
            stpp::BinarySemphr tx_sem_{true}; // 当 driver_ 正在发送数据时，上锁

            QueueWithLock rx_queue_;
            stpp::BinarySemphr rx_sem_{true}; // 当 driver_ 正在接收数据时，上锁

            TaskHandle_t spin_task_handle_ = nullptr;

            void NotifySpinTaskFromThread()
            {
                if (spin_task_handle_ != nullptr) {
                    xTaskNotifyGive(spin_task_handle_);
                }
            }
            void NotifySpinTaskFromISR()
            {
                if (spin_task_handle_ != nullptr) {
                    // 通知守护线程传输
                    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                    vTaskNotifyGiveFromISR(spin_task_handle_, &xHigherPriorityTaskWoken);
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }
        };

    }
}
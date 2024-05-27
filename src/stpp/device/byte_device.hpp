#pragma once

#include "../driver/byte_driver.hpp"
#include "../freertos_lock.hpp"
#include <memory>
#include "../continuous_buffer.hpp"
#include <FreeRTOS.h>
#include <task.h>

namespace stpp
{
    namespace device
    {
        class ByteDevice
        {
        public:
            std::unique_ptr<driver::ByteDriver> driver_;

            ByteDevice(std::unique_ptr<driver::ByteDriver> driver, size_t rx_buffer_size = 512, size_t tx_buffer_size = 512)
                : driver_(std::move(driver)), tx_buffer_(tx_buffer_size){};

            ByteDevice(ByteDevice &&)                 = default;
            ByteDevice(const ByteDevice &)            = delete;
            ByteDevice &operator=(ByteDevice &&)      = default;
            ByteDevice &operator=(const ByteDevice &) = delete;
            ~ByteDevice()                             = default;

            // std::size_t Read(void *data, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            // {
            //     std::lock_guard lock(rx_sem_);
            //     // This is a blocking call, which will not cause callback to be called
            //     return driver_->Read(data, length, timeout);
            // }

            std::size_t Write(const void *data, std::size_t length)
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

            void Spin()
            {
                task_to_notify_ = xTaskGetCurrentTaskHandle();
                NotifySpinTaskFromThread(); // 防止在 Spin 之前有数据
                while (true) {
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                    tx_lock_.lock_from_thread();
                    taskENTER_CRITICAL();
                    auto [buffer, buffer_size] = tx_buffer_.GetBuffer();
                    taskEXIT_CRITICAL();
                    if (buffer_size > 0) {
                        last_write_length_ = buffer_size;
                        WriteNonBlocking_(buffer, buffer_size);
                    } else {
                        tx_lock_.unlock();
                    }
                }
            }

            void WriteCpltCallback()
            {
                auto uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
                tx_buffer_.Shrink(last_write_length_);
                taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                tx_lock_.unlock();
            }

            // void ReadCpltCallback()
            // {
            //     rx_lock_.unlock();
            // }

        protected:
            BinarySemphr tx_lock_{true};
            // BinarySemphr rx_lock_{false};
            // ContinuousBuffer rx_buffer_;
            ContinuousBuffer tx_buffer_;
            size_t last_write_length_    = 0;
            TaskHandle_t task_to_notify_ = nullptr;

            // bool ReadNonBlocking_(uint8_t *data, std::size_t length)
            // {
            //     if (driver_->IsReadDmaAvailable(data)) {
            //         return driver_->ReadDma(data, length);
            //     } else if (driver_->IsReadItAvailable(data)) {
            //         return driver_->ReadIt(data, length);
            //     } else {
            //         return false;
            //     }
            // }

            bool WriteNonBlocking_(uint8_t *data, std::size_t length)
            {
                if (driver_->IsWriteDmaAvailable(data)) {
                    return driver_->WriteDma(data, length);
                } else if (driver_->IsWriteItAvailable(data)) {
                    return driver_->WriteIt(data, length);
                } else {
                    return false;
                }
            }

            void NotifySpinTaskFromThread()
            {
                if (task_to_notify_ != nullptr) {
                    xTaskNotifyGive(task_to_notify_);
                }
            }
            void NotifySpinTaskFromISR()
            {
                if (task_to_notify_ != nullptr) {
                    // 通知守护线程传输
                    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                    vTaskNotifyGiveFromISR(task_to_notify_, &xHigherPriorityTaskWoken);
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }
        };

    }
}
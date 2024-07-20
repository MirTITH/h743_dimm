#pragma once

#include <cstddef>
#include <FreeRTOS.h>
#include <limits>
#include <queue.h>
#include <stdexcept>
#include "in_handle_mode.h"
#include "freertos_delay_ms.h"

namespace stpp
{
    template <typename T>
    class FreertosQueue
    {
    public:
        QueueHandle_t queue_handle_;

        FreertosQueue(std::size_t queue_length)
        {
            queue_handle_ = xQueueCreate(queue_length, sizeof(T));
        }

        FreertosQueue(FreertosQueue &&)                 = default;
        FreertosQueue(const FreertosQueue &)            = delete;
        FreertosQueue &operator=(FreertosQueue &&)      = default;
        FreertosQueue &operator=(const FreertosQueue &) = delete;

        ~FreertosQueue()
        {
            vQueueDelete(queue_handle_);
        }

        bool SendToBackFromISR(const T &item)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            alignas(T) unsigned char buf[sizeof(T)];
            new (buf) T(item);

            auto result = xQueueSendToBackFromISR(queue_handle_, buf, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            return result == pdTRUE;
        }

        bool SendToBackFromThread(const T &item, size_t ms_to_wait = std::numeric_limits<size_t>::max())
        {
            alignas(T) unsigned char buf[sizeof(T)];
            new (buf) T(item);

            return xQueueSendToBack(queue_handle_, buf, FreeRtosMsToTick(ms_to_wait)) == pdTRUE;
        }

        bool SendToBack(const T &item, size_t ms_to_wait = std::numeric_limits<size_t>::max())
        {
            if (InHandlerMode()) {
                return SendToBackFromISR(item);
            } else {
                return SendToBackFromThread(item, ms_to_wait);
            }
        }

        bool SendToFrontFromISR(const T &item)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            alignas(T) unsigned char buf[sizeof(T)];
            new (buf) T(item);

            auto result = xQueueSendToFrontFromISR(queue_handle_, buf, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            return result == pdTRUE;
        }

        bool SendToFrontFromThread(const T &item, size_t ms_to_wait = std::numeric_limits<size_t>::max())
        {
            alignas(T) unsigned char buf[sizeof(T)];
            new (buf) T(item);
            return xQueueSendToFront(queue_handle_, buf, FreeRtosMsToTick(ms_to_wait)) == pdTRUE;
        }

        bool SendToFront(const T &item, size_t ms_to_wait = std::numeric_limits<size_t>::max())
        {
            if (InHandlerMode()) {
                return SendToFrontFromISR(item);
            } else {
                return SendToFrontFromThread(item, ms_to_wait);
            }
        }

        T ReceiveFromThread(size_t ms_to_wait = std::numeric_limits<size_t>::max())
        {
            BaseType_t result;
            alignas(T) unsigned char buf[sizeof(T)];
            result = xQueueReceive(queue_handle_, buf, FreeRtosMsToTick(ms_to_wait));

            if (result != pdTRUE) {
                throw std::runtime_error("Failed to receive from queue");
            }

            T item{std::move(*reinterpret_cast<T *>(buf))};
            reinterpret_cast<T *>(buf)->~T();

            return item;
        }

        T ReceiveFromISR()
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            alignas(T) unsigned char buf[sizeof(T)];

            auto result = xQueueReceiveFromISR(queue_handle_, buf, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

            if (result != pdTRUE) {
                throw std::runtime_error("Failed to receive from queue");
            }

            T item{std::move(*reinterpret_cast<T *>(buf))};
            reinterpret_cast<T *>(buf)->~T();

            return item;
        }

        T Receive(size_t ms_to_wait = std::numeric_limits<size_t>::max())
        {
            if (InHandlerMode()) {
                return ReceiveFromISR();
            } else {
                return ReceiveFromThread(ms_to_wait);
            }
        }

        size_t size() const
        {
            return uxQueueMessagesWaiting(queue_handle_);
        }
    };

}
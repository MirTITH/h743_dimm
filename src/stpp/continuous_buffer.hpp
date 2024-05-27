#pragma once

#include <cstdint>
#include <cstring>
#include <tuple>

namespace stpp
{
    class ContinuousBuffer
    {
    public:
        ContinuousBuffer(std::size_t size)
            : capacity_(size), head_(0), tail_(0)
        {
            buffer_ = new uint8_t[size];
        }

        ContinuousBuffer(ContinuousBuffer &&other)
            : buffer_(other.buffer_), capacity_(other.capacity_), head_(other.head_), tail_(other.tail_)
        {
            other.buffer_   = nullptr;
            other.capacity_ = 0;
            other.head_     = 0;
            other.tail_     = 0;
        }

        ContinuousBuffer &operator=(ContinuousBuffer &&other)
        {
            if (this != &other) {
                delete[] buffer_;
                buffer_         = other.buffer_;
                capacity_       = other.capacity_;
                head_           = other.head_;
                tail_           = other.tail_;
                other.buffer_   = nullptr;
                other.capacity_ = 0;
                other.head_     = 0;
                other.tail_     = 0;
            }
            return *this;
        }

        ContinuousBuffer(const ContinuousBuffer &)            = delete; // Typically, we do not want to copy the buffer. It is better to move it.
        ContinuousBuffer &operator=(const ContinuousBuffer &) = delete; // Typically, we do not want to copy the buffer. It is better to move it.

        ~ContinuousBuffer()
        {
            delete[] buffer_;
        }

        /**
         * @brief Get the max number of bytes that can be stored in the buffer
         *
         * @return std::size_t
         */
        std::size_t GetCapacity() const
        {
            return capacity_;
        }

        /**
         * @brief Get the number of bytes in the buffer
         *
         * @return std::size_t
         */
        std::size_t GetSize() const
        {
            return tail_ - head_;
        }

        /**
         * @brief Get the number of bytes that can be stored in the buffer
         * @note AvailableSpace() != GetCapacity() - GetSize() because the buffer is continuous
         * @return std::size_t
         */
        std::size_t GetAvailableSpace() const
        {
            return capacity_ - tail_;
        }

        std::size_t PushBack(const uint8_t *data, std::size_t size)
        {
            auto available_space = GetAvailableSpace();

            if (size > available_space) {
                size = available_space;
            }

            std::memcpy(buffer_ + tail_, data, size);
            tail_ += size;
            return size;
        }

        std::size_t PopFront(uint8_t *data, std::size_t size)
        {
            auto available_data = GetSize();

            if (size > available_data) {
                size = available_data;
            }

            std::memcpy(data, buffer_ + head_, size);
            head_ += size;
            Optimize();
            return size;
        }

        std::tuple<uint8_t *, std::size_t> Expand(std::size_t size)
        {
            auto available_space = GetAvailableSpace();
            if (size > available_space) {
                size = available_space;
            }

            auto old_tail = tail_;
            tail_ += size;
            return {buffer_ + old_tail, size};
        }

        std::size_t Shrink(std::size_t size)
        {
            auto available_data = GetSize();
            if (size > available_data) {
                size = available_data;
            }

            head_ += size;
            Optimize();
            return size;
        }

        void Clear()
        {
            head_ = 0;
            tail_ = 0;
        }

        std::tuple<uint8_t *, std::size_t> GetBuffer()
        {
            return {buffer_ + head_, GetSize()};
        }

    private:
        uint8_t *buffer_;
        std::size_t capacity_;
        std::size_t head_;
        std::size_t tail_;

        void Optimize()
        {
            if (head_ == tail_) {
                head_ = 0;
                tail_ = 0;
            }
        }
    };
}
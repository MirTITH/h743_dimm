#pragma once

#include <cstring>
#include <memory>
#include "callback_func.hpp"
#include "../../freertos_memory.hpp"

namespace stpp
{
    namespace device_framework_internal
    {
        template<typename Mallocator_t>
        class TxDataWithCallback
        {
        public:
            std::shared_ptr<const uint8_t[]> data_;
            size_t length_;
            CallbackFunc_t callback_;

            TxDataWithCallback()
                : data_(nullptr), length_(0), callback_() {};

            /**
             * @brief 构建一个 TxDataWithCallback 对象。该对象将数据拷贝到内部。
             *
             */
            TxDataWithCallback(Mallocator_t &mem_, const uint8_t *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                : length_(length), callback_(std::move(callback))
            {
                auto new_data = static_cast<uint8_t *>(mem_.Malloc(length));
                std::memcpy(new_data, data, length);
                data_.reset(new_data, std::bind(&Mallocator_t::Free, &mem_, std::placeholders::_1, length));
            }

            /**
             * @brief 构建一个 TxDataWithCallback 对象。该对象只保存指针，不拷贝数据，使用智能指针管理内存。
             *
             */
            TxDataWithCallback(std::shared_ptr<const uint8_t[]> data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                : data_(std::move(data)), length_(length), callback_(std::move(callback)) {};

            /**
             * @brief 构建一个 TxDataWithCallback 对象。该对象只保存指针，不拷贝数据，不负责释放内存。
             *
             */
            TxDataWithCallback(const uint8_t *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                : data_(data, [](const void *) {}), length_(length), callback_(std::move(callback)) {};

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

        class RxDataWithCallback
        {
        public:
            std::shared_ptr<uint8_t[]> data_;
            size_t length_;
            CallbackFunc_t callback_;

            RxDataWithCallback()
                : data_(nullptr), length_(0), callback_() {};

            /**
             * @brief 构建一个 RxDataWithCallback 对象。该对象使用智能指针管理内存。
             *
             */
            RxDataWithCallback(std::shared_ptr<uint8_t[]> data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                : data_(std::move(data)), length_(length), callback_(std::move(callback)) {};

            /**
             * @brief 构建一个 RxDataWithCallback 对象。该对象不负责释放内存。
             *
             */
            RxDataWithCallback(uint8_t *data, std::size_t length, CallbackFunc_t callback = CallbackFunc_t())
                : data_(data, [](uint8_t *) {}), length_(length), callback_(std::move(callback)) {};

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
    }
}
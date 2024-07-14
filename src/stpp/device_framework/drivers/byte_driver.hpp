#pragma once
#include <cstdint>
#include <functional>
#include "../../error_code.hpp"

namespace stpp
{
    namespace driver
    {
        class ByteDriver
        {
            using CallbackFunc_t = std::function<void(stpp::ErrorCode)>;

        public:
            ByteDriver()                   = default;
            ByteDriver(const ByteDriver &) = delete;
            ByteDriver(ByteDriver &&)      = default;

            virtual bool AsyncRead(uint8_t *buffer, std::size_t length)        = 0;
            virtual bool AsyncWrite(const uint8_t *buffer, std::size_t length) = 0;

            void SetReadCpltCb(CallbackFunc_t cb)
            {
                read_cplt_cb_ = std::move(cb);
            }

            void SetWriteCpltCb(CallbackFunc_t cb)
            {
                write_cplt_cb_ = std::move(cb);
            }

        protected:
            CallbackFunc_t read_cplt_cb_;
            CallbackFunc_t write_cplt_cb_;
        };
    }
}
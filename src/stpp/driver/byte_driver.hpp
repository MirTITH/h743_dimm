#pragma once
#include <cstdint>
#include <functional>
#include <limits>

namespace stpp
{
    namespace driver
    {
        class ByteDriver
        {
            using CallbackFunc_t = std::function<void(int)>;

        public:
            ByteDriver()                   = default;
            ByteDriver(const ByteDriver &) = delete;
            ByteDriver(ByteDriver &&)      = default;

            virtual std::size_t AsyncRead(uint8_t *buffer, std::size_t length)        = 0;
            virtual std::size_t AsyncWrite(const uint8_t *buffer, std::size_t length) = 0;

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
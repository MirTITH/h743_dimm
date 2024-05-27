#pragma once
#include <cstdint>

namespace stpp
{
    namespace driver
    {
        class ByteDriver
        {
        public:
            ByteDriver()                   = default;
            ByteDriver(const ByteDriver &) = delete;
            ByteDriver(ByteDriver &&)      = default;

            /**
             * @brief Read data from device in blocking mode
             *
             * @param buffer Where to store data
             * @param length How many bytes to read
             * @param timeout Timeout in milliseconds
             * @return std::size_t
             */
            virtual std::size_t Read(void *buffer, std::size_t length, uint32_t timeout_ms)        = 0;
            virtual std::size_t Write(const void *buffer, std::size_t length, uint32_t timeout_ms) = 0;

            virtual bool ReadIt(void *buffer, std::size_t length)        = 0;
            virtual bool WriteIt(const void *buffer, std::size_t length) = 0;

            virtual bool ReadDma(void *buffer, std::size_t length)        = 0;
            virtual bool WriteDma(const void *buffer, std::size_t length) = 0;

            virtual bool IsReadDmaAvailable(const void *address)  = 0;
            virtual bool IsWriteDmaAvailable(const void *address) = 0;
            virtual bool IsReadItAvailable(const void *address)   = 0;
            virtual bool IsWriteItAvailable(const void *address)  = 0;
        };
    }

}
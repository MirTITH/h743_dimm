#pragma once

#include "driver_base.hpp"
#include <cstdint>
#include <limits>

namespace stpp
{
    namespace driver
    {
        class DriverByte : public DriverBase
        {
        public:
            virtual bool Init()  = 0;
            virtual bool Open()  = 0;
            virtual bool Close() = 0;

            /**
             * @brief Read data from device in blocking mode
             *
             * @param buffer Where to store data
             * @param pos Start position of the device memory. It is used for devices with memory mapped IO. For other devices, it is ignored.
             * @param length How many bytes to read
             * @param timeout Timeout in milliseconds
             * @return std::size_t
             */
            virtual std::size_t Read(void *buffer, std::size_t pos, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())        = 0;
            virtual std::size_t Write(const void *buffer, std::size_t pos, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max()) = 0;

            virtual std::size_t ReadIt(void *buffer, std::size_t pos, std::size_t length)        = 0;
            virtual std::size_t WriteIt(const void *buffer, std::size_t pos, std::size_t length) = 0;

            virtual std::size_t ReadDma(void *buffer, std::size_t pos, std::size_t length)        = 0;
            virtual std::size_t WriteDma(const void *buffer, std::size_t pos, std::size_t length) = 0;

            virtual bool IsReadDmaAvailable(const void *address)
            {
                (void)address;
                return false;
            }

            virtual bool IsWriteDmaAvailable(const void *address)
            {
                (void)address;
                return false;
            }

            virtual bool IsReadItAvailable(const void *address)
            {
                (void)address;
                return false;
            }

            virtual bool IsWriteItAvailable(const void *address)
            {
                (void)address;
                return false;
            }
        };
    }

}
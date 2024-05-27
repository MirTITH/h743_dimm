#pragma once

#include "driver_byte.hpp"
#include <usart.h>

#ifdef DEBUG
#include <cassert>
#endif

namespace stpp
{
    namespace driver
    {
        class DriverUart : public DriverByte
        {
        public:
            UART_HandleTypeDef &huart_;

            DriverUart(UART_HandleTypeDef &huart)
                : huart_(huart){};

            virtual bool Init() override
            {
                return true;
            }

            virtual bool Open() override
            {
                return true;
            }

            virtual bool Close() override
            {
                return true;
            }

            virtual std::size_t Read(void *buffer, std::size_t pos, std::size_t length, uint32_t timeout) override
            {
                (void)pos;
#ifdef DEBUG
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());
#endif
                auto result = HAL_UART_Receive(&huart_, static_cast<uint8_t *>(buffer), length, timeout);
                return result == HAL_OK ? length : 0;
            }

            virtual std::size_t Write(const void *buffer, std::size_t pos, std::size_t length, uint32_t timeout) override
            {
                (void)pos;
#ifdef DEBUG
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());
#endif
                auto result = HAL_UART_Transmit(&huart_, static_cast<const uint8_t *>(buffer), length, timeout);
                return result == HAL_OK ? length : 0;
            }

            virtual std::size_t ReadIt(void *buffer, std::size_t pos, std::size_t length) override
            {
                (void)pos;
#ifdef DEBUG
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());
#endif
                auto result = HAL_UART_Receive_IT(&huart_, static_cast<uint8_t *>(buffer), length);
                return result == HAL_OK ? length : 0;
            }

            virtual std::size_t WriteIt(const void *buffer, std::size_t pos, std::size_t length) override
            {
                (void)pos;
#ifdef DEBUG
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());
#endif
                auto result = HAL_UART_Transmit_IT(&huart_, static_cast<const uint8_t *>(buffer), length);
                return result == HAL_OK ? length : 0;
            }

            virtual std::size_t ReadDma(void *buffer, std::size_t pos, std::size_t length) override
            {
                (void)pos;
#ifdef DEBUG
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());
#endif
                auto result = HAL_UART_Receive_DMA(&huart_, static_cast<uint8_t *>(buffer), length);
                return result == HAL_OK ? length : 0;
            }

            virtual std::size_t WriteDma(const void *buffer, std::size_t pos, std::size_t length) override
            {
                (void)pos;
#ifdef DEBUG
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());
#endif
                auto result = HAL_UART_Transmit_DMA(&huart_, static_cast<const uint8_t *>(buffer), length);
                return result == HAL_OK ? length : 0;
            }

            virtual bool IsReadDmaAvailable(const void *address) override
            {
                if (IsAddressValidForDma(address)) {
                    return huart_.hdmarx != nullptr;
                } else {
                    return false;
                }
            }

            virtual bool IsWriteDmaAvailable(const void *address) override
            {
                if (IsAddressValidForDma(address)) {
                    return huart_.hdmatx != nullptr;
                } else {
                    return false;
                }
            }

            virtual bool IsReadItAvailable(const void *address) override
            {
                (void)address;
                return true;
            }

            virtual bool IsWriteItAvailable(const void *address) override
            {
                (void)address;
                return true;
            }

        protected:
            bool IsAddressValidForDma(const void *addr)
            {
                if (((size_t)addr < (0x0 + 64 * 1024)) &&                                   // ITCMRAM
                    ((size_t)addr >= 0x20000000 && (size_t)addr < (0x20000000 + 64 * 1024)) // DTCMRAM
                ) {
                    return false;
                }
                return true;
            }
        };
    }
}

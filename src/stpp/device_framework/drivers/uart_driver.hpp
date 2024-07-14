#pragma once

#include "byte_driver.hpp"
#include <usart.h>

#include <cassert>

namespace stpp
{
    namespace driver
    {
        class UartDriver : public ByteDriver
        {
        public:
            UART_HandleTypeDef *huart_;

            UartDriver(UART_HandleTypeDef *huart)
                : huart_(huart)
            {
                assert(huart_ != nullptr);
            }

            UartDriver(UartDriver &&) = default;

            virtual bool AsyncRead(uint8_t *buffer, std::size_t length) override
            {
                if (IsAddressValidForDma(buffer)) {
                    return ReadDma(buffer, length);
                } else {
                    return ReadIt(buffer, length);
                }
            }

            virtual bool AsyncWrite(const uint8_t *buffer, std::size_t length) override
            {
                if (IsAddressValidForDma(buffer)) {
                    return WriteDma(buffer, length);
                } else {
                    return WriteIt(buffer, length);
                }
            }

            bool ReadIt(void *buffer, std::size_t length)
            {
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());

                auto result = HAL_UART_Receive_IT(huart_, static_cast<uint8_t *>(buffer), length);
                return result == HAL_OK;
            }

            bool WriteIt(const void *buffer, std::size_t length)
            {
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());

                auto result = HAL_UART_Transmit_IT(huart_, static_cast<const uint8_t *>(buffer), length);
                return result == HAL_OK;
            }

            bool ReadDma(void *buffer, std::size_t length)
            {
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());

                auto result = HAL_UART_Receive_DMA(huart_, static_cast<uint8_t *>(buffer), length);
                return result == HAL_OK;
            }

            bool WriteDma(const void *buffer, std::size_t length)
            {
                assert(buffer != nullptr);
                assert(length <= std::numeric_limits<uint16_t>::max());

                auto result = HAL_UART_Transmit_DMA(huart_, static_cast<const uint8_t *>(buffer), length);
                return result == HAL_OK;
            }

            void _RxCpltCallback()
            {
                if (read_cplt_cb_) {
                    read_cplt_cb_(ErrorCode::OK);
                }
            }

            void _TxCpltCallback()
            {
                if (write_cplt_cb_) {
                    write_cplt_cb_(ErrorCode::OK);
                }
            }

        protected:
            bool IsAddressValidForDma(const void *addr)
            {
                size_t addr_int = reinterpret_cast<size_t>(addr);

                if (
                    (addr_int < (0x0 + 64 * 1024)) ||                               // ITCMRAM
                    (addr_int >= 0x20000000 && addr_int < (0x20000000 + 64 * 1024)) // DTCMRAM
                ) {
                    return false;
                }

                return true;
            }
        };
    }
}

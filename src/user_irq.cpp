#include <cstdio>
#include <main.h>
#include <devices/devices.hpp>
#include <HighPrecisionTime/high_precision_time.h>

#ifdef __cplusplus
extern "C" {
#endif
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
#ifdef __cplusplus
}
#endif

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        devices::Uart1->WriteCpltCallback();
    }
}

void MY_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static int count         = 0;
    static uint32_t duration = 0;
    if (htim->Instance == TIM6) {
        char str[64];
        auto length   = std::snprintf(str, sizeof(str), "TIM6 PeriodElapsedCallback %d, %lu us\n", count++, duration);
        auto start_us = HPT_GetUs();
        devices::Uart1->Write(str, length);
        duration = HPT_GetUs() - start_us;
    }
}
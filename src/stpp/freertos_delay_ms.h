#pragma once

#include "FreeRTOS.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将毫秒转换为 FreeRTOS 的 Tick 数。如果 ms 为 std::numeric_limits<uint32_t>::max()，则返回 portMAX_DELAY
 *
 */
TickType_t FreeRtosMsToTick(uint32_t ms);

/**
 * @brief 等待指定的毫秒数
 * 
 * @param ms 
 */
void FreeRtosDelayMs(uint32_t ms);

#ifdef __cplusplus
}
#endif

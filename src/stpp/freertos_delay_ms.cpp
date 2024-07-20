#include "freertos_delay_ms.h"
#include "task.h"
#include <limits>

TickType_t FreeRtosMsToTick(uint32_t ms)
{
    if (ms == std::numeric_limits<uint32_t>::max()) {
        return portMAX_DELAY;
    } else {
        return pdMS_TO_TICKS(ms);
    }
}

void FreeRtosDelayMs(uint32_t ms)
{
    vTaskDelay(FreeRtosMsToTick(ms));
}
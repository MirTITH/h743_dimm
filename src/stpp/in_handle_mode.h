#pragma once

/* Determine whether we are in thread mode or handler mode. */
inline int InHandlerMode(void)
{
    int result;
    __asm volatile("MRS %0, ipsr" : "=r"(result));
    return result != 0;
}

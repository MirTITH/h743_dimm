#pragma once

#include <cstddef>
#include <FreeRTOS.h>

template <typename T>
class FreeRTOSAllocator
{
public:
    using value_type = T;

    FreeRTOSAllocator() = default;

    template <typename U>
    FreeRTOSAllocator(const FreeRTOSAllocator<U> &){};

    T *allocate(std::size_t n)
    {
        return static_cast<T *>(pvPortMalloc(n * sizeof(T)));
    }

    void deallocate(T *p, std::size_t)
    {
        vPortFree(p);
    }
};

template <typename T, typename U>
bool operator==(const FreeRTOSAllocator<T> &, const FreeRTOSAllocator<U> &)
{
    return true;
}

template <typename T, typename U>
bool operator!=(const FreeRTOSAllocator<T> &, const FreeRTOSAllocator<U> &)
{
    return false;
}

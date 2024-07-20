#pragma once

#include "../../error_code.hpp"
#include <functional>

namespace stpp
{
    namespace device_framework_internal
    {
        using CallbackFunc_t = std::function<void(stpp::ErrorCode)>;
    }
}
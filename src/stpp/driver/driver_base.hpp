#pragma once

namespace stpp
{
    namespace driver
    {
        class DriverBase
        {
        public:
            DriverBase()                              = default;
            DriverBase(DriverBase &&)                 = default;
            DriverBase(const DriverBase &)            = delete;
            // DriverBase &operator=(DriverBase &&)      = default;
            // DriverBase &operator=(const DriverBase &) = delete;
            ~DriverBase()                             = default;
        };
    }
}

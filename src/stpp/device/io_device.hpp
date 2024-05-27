#pragma once

#include "../driver/driver_byte.hpp"
#include "../freertos_lock.hpp"
#include <mutex>
#include <memory>

namespace stpp
{
    namespace device
    {
        class IoDevice
        {
        public:
            std::unique_ptr<driver::DriverByte> driver_;

            IoDevice(std::unique_ptr<driver::DriverByte> driver)
                : driver_(std::move(driver))
            {
                driver_->Init();
            }

            IoDevice(IoDevice &&)      = default;
            IoDevice(const IoDevice &) = delete;
            // IoDevice &operator=(IoDevice &&)      = default;
            // IoDevice &operator=(const IoDevice &) = delete;
            ~IoDevice() = default;

            bool Open()
            {
                std::lock_guard lock(control_mutex_);

                if (open_count_ == 0) {
                    bool result = driver_->Open();
                    if (result == true) {
                        tx_sem_.unlock();
                        is_open_ = true;
                        open_count_++;
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    open_count_++;
                    return true;
                }
            }

            bool Close()
            {
                if (open_count_ == 0) {
                    return true;
                }

                std::lock_guard lock(control_mutex_);

                if (open_count_ == 1) {
                    std::unique_lock lock_tx(tx_sem_);
                    bool result = driver_->Close();
                    if (result == true) {
                        is_open_ = false;
                        open_count_--;
                        lock_tx.release();
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    open_count_--;
                    return true;
                }
            }

            std::size_t Read(void *buffer, std::size_t pos, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                std::lock_guard lock(rx_sem_);
                return driver_->Read(buffer, pos, length, timeout);
            }

            std::size_t Write(const void *buffer, std::size_t pos, std::size_t length, uint32_t timeout = std::numeric_limits<uint32_t>::max())
            {
                std::lock_guard lock(tx_sem_);
                return driver_->Write(buffer, pos, length, timeout);
            }

        protected:
            bool is_open_      = false;
            size_t open_count_ = 0;
            Mutex control_mutex_;
            BinarySemphr tx_sem_{false};
            BinarySemphr rx_sem_{false};
        };

    }
}
#pragma once

#include <cassert>
#include <cstddef>
#include <FreeRTOS.h>
#include <limits>
#include <new>
#include <atomic>
#include <memory>
#include <cstdlib>

namespace stpp
{
    template <void *(*MallocFunc)(std::size_t), void (*FreeFunc)(void *)>
    class MallocLimited
    {
        using Alloc_t = std::atomic<std::size_t>;

    public:
        MallocLimited(std::size_t max_size)
            : max_size_(max_size), allocated_size_ptr_(std::make_shared<Alloc_t>(0)) {};

        MallocLimited(MallocLimited &&other) noexcept
            : max_size_(other.max_size_), allocated_size_ptr_(std::move(other.allocated_size_ptr_))
        {
            if (this != &other) {
                other.max_size_ = 0;
            }
        }

        MallocLimited &operator=(MallocLimited &&other) noexcept
        {
            if (this != &other) {
                max_size_           = other.max_size_;
                allocated_size_ptr_ = std::move(other.allocated_size_ptr_);
                other.max_size_     = 0;
            }

            return *this;
        }

        MallocLimited(const MallocLimited &)            = default;
        MallocLimited &operator=(const MallocLimited &) = default;

        std::size_t GetMaxSize() const noexcept
        {
            return max_size_;
        }

        std::size_t GetAllocatedSize() const noexcept
        {
            return *allocated_size_ptr_;
        }

        [[nodiscard]] void *Malloc(std::size_t bytes)
        {
            std::size_t new_size = *allocated_size_ptr_ += bytes;
            if (new_size > max_size_) {
                *allocated_size_ptr_ -= bytes;
                return nullptr;
            }

            void *p = MallocFunc(bytes);
            if (p == nullptr) {
                *allocated_size_ptr_ -= bytes;
                return nullptr;
            }

            return p;
        }

        void Free(void *p, std::size_t bytes)
        {
            FreeFunc(p);
            if (p != nullptr) {
                *allocated_size_ptr_ -= bytes;
            }
        }

    private:
        std::size_t max_size_;
        std::shared_ptr<Alloc_t> allocated_size_ptr_;
    };

    template <typename T, typename MallocLimited_t>
    class AllocatorLimited
    {
    public:
        using value_type = T;

        AllocatorLimited(MallocLimited_t mallocator) noexcept
            : mallocator_(std::move(mallocator)) {};

        template <typename U, typename V>
        friend class AllocatorLimited;

        template <typename U, typename V>
        constexpr AllocatorLimited(const AllocatorLimited<U, V> &other) noexcept
            : mallocator_(other.mallocator_){};

        [[nodiscard]] T *allocate(std::size_t n)
        {
            if (n > std::numeric_limits<size_t>::max() / sizeof(T)) throw std::bad_alloc();
            auto p = static_cast<T *>(mallocator_.Malloc(n * sizeof(T)));
            if (p == nullptr) throw std::bad_alloc();
            return p;
        }

        void deallocate(T *p, std::size_t n) noexcept
        {
            mallocator_.Free(p, n * sizeof(T));
        }

        template <typename U, typename V>
        bool operator==(const AllocatorLimited<U, V> &)
        {
            return true;
        }

        template <typename U, typename V>
        bool operator!=(const AllocatorLimited<U, V> &)
        {
            return false;
        }

    private:
        MallocLimited_t mallocator_;
    };
}

// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    template <typename T>
    class UniquePtr
    {
    public:
        using ElementType = T;

        constexpr UniquePtr() noexcept : m_ptr(nullptr) {}
        constexpr UniquePtr(nullptr_t) noexcept : m_ptr(nullptr) {}
        explicit UniquePtr(T* p) noexcept : m_ptr(p) {}

        template <typename U> requires(!IsArray<U> && IsConvertible<U*, T*>)
        UniquePtr(UniquePtr<U>&& x) noexcept : m_ptr(Exchange(x.m_ptr, nullptr)) {}

        ~UniquePtr() noexcept { Reset(); }

        [[nodiscard]] T* Get() const { return m_ptr; }
        T* Release() { T* p = m_ptr; m_ptr = nullptr; return p; }

        void Reset(T* p = nullptr) noexcept
        {
            Allocator::GetDefault().Delete(m_ptr);
            m_ptr = p;
        }

        [[nodiscard]] T* operator->() const { return m_ptr; }
        [[nodiscard]] T& operator*() const { return *m_ptr; }

        [[nodiscard]] explicit operator bool() const { return m_ptr != nullptr; }

        UniquePtr& operator=(nullptr_t) noexcept { Reset(); return *this; }

        template <typename U> requires(!IsArray<U> && IsConvertible<U*, T*>)
        UniquePtr& operator=(UniquePtr<U>&& x) noexcept { Reset(x.Release()); return *this; }

        template <typename U>
        [[nodiscard]] bool operator==(const UniquePtr<U>& x) const { return m_ptr == x.m_ptr; }

        template <typename U>
        [[nodiscard]] bool operator!=(const UniquePtr<U>& x) const { return m_ptr != x.m_ptr; }

        template <typename U>
        [[nodiscard]] bool operator<(const UniquePtr<U>& x) const { return m_ptr < x.m_ptr; }

    private:
        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

    private:
        template <typename U>
        friend class UniquePtr;

        T* m_ptr;
    };

    template <typename T>
    class UniquePtr<T[]>
    {
    public:
        using ElementType = T;

        constexpr UniquePtr() noexcept : m_ptr(nullptr) {}
        constexpr UniquePtr(nullptr_t) noexcept : m_ptr(nullptr) {}
        explicit UniquePtr(T* p) noexcept : m_ptr(p) {}

        template <typename U, typename E = typename UniquePtr<U>::ElementType>
            requires(IsArray<U> && IsConvertible<E(*)[], T(*)[]>)
        UniquePtr(UniquePtr<U>&& x) noexcept : m_ptr(Exchange(x.m_ptr, nullptr)) {}

        ~UniquePtr() noexcept { Reset(); }

        [[nodiscard]] T* Get() const { return m_ptr; }
        T* Release() const { T* p = m_ptr; m_ptr = nullptr; return p; }
        void Reset(T* p = nullptr) { Allocator::GetDefault().DeleteArray(p); m_ptr = p; }

        [[nodiscard]] T* operator->() const { return m_ptr; }
        [[nodiscard]] T& operator*() const { return *m_ptr; }

        [[nodiscard]] explicit operator bool() const { return m_ptr != nullptr; }

        UniquePtr& operator=(nullptr_t) noexcept { Reset(); return *this; }

        template <typename U, typename E = typename UniquePtr<U>::ElementType>
            requires(IsArray<U> && IsConvertible<E(*)[], T(*)[]>)
        UniquePtr& operator=(UniquePtr<U>&& x) noexcept { Reset(x.Release()); return *this; }

        template <typename U>
        [[nodiscard]] bool operator==(const UniquePtr<U>& x) const { return m_ptr == x.m_ptr; }

        template <typename U>
        [[nodiscard]] bool operator!=(const UniquePtr<U>& x) const { return m_ptr != x.m_ptr; }

        template <typename U>
        [[nodiscard]] bool operator<(const UniquePtr<U>& x) const { return m_ptr < x.m_ptr; }

        [[nodiscard]] T& operator[](uint32_t index) const { return m_ptr[index]; }

    private:
        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

    private:
        T* m_ptr;
    };

    template <typename T, typename... Args> requires(!IsArray<T>)
    [[nodiscard]] UniquePtr<T> MakeUnique(Args&&... args) noexcept
    {
        return UniquePtr<T>(Allocator::GetDefault().New<T>(Forward<Args>(args)...));
    }

    template <typename T> requires(IsArray<T> && Extent<T> == 0)
    [[nodiscard]] UniquePtr<T> MakeUnique(uint32_t size) noexcept
    {
        return UniquePtr<T>(Allocator::GetDefault().NewArray<T>(size));
    }

    template <typename T> requires(!IsArray<T>)
    [[nodiscard]] UniquePtr<T> MakeUnique(DefaultInitTag) noexcept
    {
        return UniquePtr<T>(Allocator::GetDefault().Malloc<T>(1));
    }

    template <typename T> requires(IsArray<T> && Extent<T> == 0)
    [[nodiscard]] UniquePtr<T> MakeUnique(uint32_t size, DefaultInitTag) noexcept
    {
        return UniquePtr<T>(Allocator::GetDefault().Malloc<T>(size));
    }
}

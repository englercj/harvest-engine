// Copyright Chad Engler

#pragma once

#include "he/core/macros.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

namespace he
{
    template <typename T>
    class ScopeGuard
    {
    public:
        ScopeGuard(const T& f)
            : m_func(f)
            , m_active(true)
        {}

        ScopeGuard(T&& f)
            : m_func(Move(f))
            , m_active(true)
        {}

        ScopeGuard(ScopeGuard&& other)
            : m_func(Move(other.m_func))
            , m_active(other.m_active)
        {
            other.m_active = false;
        }

        ScopeGuard& operator=(ScopeGuard&& other)
        {
            m_func = Move(other.m_func);
            m_active = other.m_active;

            other.m_active = false;
            return *this;
        }

        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;

        ~ScopeGuard()
        {
            if (m_active)
                m_func();
        }

        void Dismiss()
        {
            m_active = false;
        }

    private:
        T m_func;
        bool m_active;
    };

    template <typename T>
    inline ScopeGuard<typename std::decay<T>::type> MakeScopeGuard(T&& func)
    {
        return ScopeGuard<typename std::decay<T>::type>(Forward<T>(func));
    }
}

#define HE_AT_SCOPE_EXIT(...) auto HE_UNIQUE_NAME(scopeGuard_) = he::MakeScopeGuard(__VA_ARGS__)

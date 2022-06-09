// Copyright Chad Engler

#pragma once

#include "he/core/macros.h"
#include "he/core/utils.h"

#include <type_traits>

namespace he
{
    template <typename F>
    class ScopeGuard
    {
    public:
        static_assert(!std::is_reference_v<F> && !std::is_const_v<F> && !std::is_volatile_v<F>, "ScopeGuard stores its action by value.");

        ScopeGuard(F f)
            : m_func(Move(func))
            , m_active(true)
        {}

        ScopeGuard(ScopeGuard&& x)
            : m_func(Move(x.m_func))
            , m_active(Exchange(x.m_active, false))
        {}

        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard& operator=(ScopeGuard&& other) = delete;

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
        F m_func;
        bool m_active;
    };

    template <typename F>
    inline [[nodiscard]] ScopeGuard<std::decay_t<F>> MakeScopeGuard(F&& func)
    {
        return ScopeGuard<std::decay_t<F>>(Forward<F>(func));
    }
}

#define HE_AT_SCOPE_EXIT(...) auto HE_UNIQUE_NAME(scopeGuard_) = he::MakeScopeGuard(__VA_ARGS__)

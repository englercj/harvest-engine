// Copyright Chad Engler

#pragma once

#include "he/core/macros.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

namespace he
{
    template <typename F>
    class ScopeGuard
    {
    public:
        static_assert(!IsReference<F> && !IsConst<F> && !IsVolatile<F>, "ScopeGuard stores its action by value.");

        ScopeGuard(F func) noexcept
            : m_func(Move(func))
            , m_active(true)
        {}

        ScopeGuard(ScopeGuard&& x) noexcept
            : m_func(Move(x.m_func))
            , m_active(Exchange(x.m_active, false))
        {}

        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard& operator=(ScopeGuard&& other) = delete;

        ~ScopeGuard() noexcept
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
    [[nodiscard]] inline ScopeGuard<Decay<F>> MakeScopeGuard(F&& func) noexcept
    {
        return ScopeGuard<Decay<F>>(Forward<F>(func));
    }
}

#define HE_AT_SCOPE_EXIT(...) auto HE_UNIQUE_NAME(scopeGuard_) = he::MakeScopeGuard(__VA_ARGS__)

// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/variant.h"

namespace he
{
    template <typename... T>
    struct Formatter<Variant<T...>>
    {
        using Type = Variant<T...>;

        struct _FmtVariantVisitor
        {
            String& out;

            template <typename U, uint32_t Index>
            [[nodiscard]] constexpr void operator()(const U& value, IndexConstant<Index>) const
            {
                FormatTo(out, "{}", value);
            }
        };

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const Type& value) const
        {
            _FmtVariantVisitor visitor{ out };
            value.Visit(visitor);
        }
    };
}

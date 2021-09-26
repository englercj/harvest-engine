// Copyright Chad Engler

#pragma once

#include "he/math/types.h"
#include "he/math/vec4a.h"

#include "fmt/format.h"

#include <type_traits>

namespace he
{
    template <typename T>
    struct _VecTFormatter
    {
        constexpr auto parse(fmt::format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const T& vec, FormatContext& ctx) const
            -> std::enable_if_t<std::is_floating_point_v<typename T::Type>, decltype(ctx.out())>
        {
            static_assert(T::Size > 0 && T::Size < 5);

            if constexpr (T::Size == 4)
                return fmt::format_to(ctx.out(), "({:.1f}, {:.1f}, {:.1f}, {:.1f})", vec.x, vec.y, vec.z, vec.w);
            else if constexpr (T::Size == 3)
                return fmt::format_to(ctx.out(), "({:.1f}, {:.1f}, {:.1f})", vec.x, vec.y, vec.z);
            else if constexpr (T::Size == 2)
                return fmt::format_to(ctx.out(), "({:.1f}, {:.1f})", vec.x, vec.y);
            else
                return fmt::format_to(ctx.out(), "({:.1f})", vec.x);
        }

        template <typename FormatContext>
        auto format(const T& vec, FormatContext& ctx) const
            -> std::enable_if_t<std::is_integral_v<typename T::Type>, decltype(ctx.out())>
        {
            static_assert(T::Size > 0 && T::Size < 5);

            if constexpr (T::Size == 4)
                return fmt::format_to(ctx.out(), "({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w);
            else if constexpr (T::Size == 3)
                return fmt::format_to(ctx.out(), "({}, {}, {})", vec.x, vec.y, vec.z);
            else if constexpr (T::Size == 2)
                return fmt::format_to(ctx.out(), "({}, {})", vec.x, vec.y);
            else
                return fmt::format_to(ctx.out(), "({})", vec.x);
        }
    };
}

namespace fmt
{
    template <typename T> struct formatter<he::Vec2<T>> : he::_VecTFormatter<he::Vec2<T>> {};
    template <typename T> struct formatter<he::Vec3<T>> : he::_VecTFormatter<he::Vec3<T>> {};
    template <typename T> struct formatter<he::Vec4<T>> : he::_VecTFormatter<he::Vec4<T>> {};

    template <>
    struct formatter<he::Vec4a>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Vec4a& vec, FormatContext& ctx) const -> decltype(ctx.out())
        {
            HE_ALIGNED(16) float p[]{ 0, 0, 0, 0 };
            he::Store(p, vec);

            return format_to(ctx.out(), "({:.1f}, {:.1f}, {:.1f}, {:.1f})", p[0], p[1], p[2], p[3]);
        }
    };

    template <>
    struct formatter<he::Quat>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Quat& q, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), "({:.1f}, {:.1f}, {:.1f}, {:.1f})", q.x, q.y, q.z, q.w);
        }
    };

    template <>
    struct formatter<he::Quata>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Quata& q, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), "{}", q.v);
        }
    };

    template <>
    struct formatter<he::Mat44>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Mat44& mat, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), "(cx{}, cy{}, cz{}, cw{})", mat.cx, mat.cy, mat.cz, mat.cw);
        }
    };
}

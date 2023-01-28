// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/type_traits.h"
#include "he/math/types.h"
#include "he/math/vec4a.h"

namespace he
{
    template <typename T>
    struct Formatter<Vec2<T>>
    {
        using Type = Vec2<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Vec2<T>& vec) const
        {
            if constexpr (IsFloatingPoint<T>)
                return FormatTo(out, "({:.1f}, {:.1f})", vec.x, vec.y);
            else
                return FormatTo(out, "({}, {})", vec.x, vec.y);
        }
    };

    template <typename T>
    struct Formatter<Vec3<T>>
    {
        using Type = Vec3<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Vec3<T>& vec) const
        {
            if constexpr (IsFloatingPoint<T>)
                return FormatTo(out, "({:.1f}, {:.1f}, {:.1f})", vec.x, vec.y, vec.z);
            else
                return FormatTo(out, "({}, {}, {})", vec.x, vec.y, vec.z);
        }
    };

    template <typename T>
    struct Formatter<Vec4<T>>
    {
        using Type = Vec4<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Vec4<T>& vec) const
        {
            if constexpr (IsFloatingPoint<T>)
                return FormatTo(out, "({:.1f}, {:.1f}, {:.1f}, {:.1f})", vec.x, vec.y, vec.z, vec.w);
            else
                return FormatTo(out, "({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w);
        }
    };

    template <>
    struct Formatter<Vec4a>
    {
        using Type = Vec4a;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Vec4a& vec) const
        {
            alignas(16) float p[]{ 0, 0, 0, 0 };
            Store(p, vec);

            return FormatTo(out, "({:.1f}, {:.1f}, {:.1f}, {:.1f})", p[0], p[1], p[2], p[3]);
        }
    };

    template <>
    struct Formatter<Quat>
    {
        using Type = Quat;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Quat& q) const
        {
            return FormatTo(out, "({:.1f}, {:.1f}, {:.1f}, {:.1f})", q.x, q.y, q.z, q.w);
        }
    };

    template <>
    struct Formatter<Quata>
    {
        using Type = Quata;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Quata& q) const
        {
            return FormatTo(out, "{}", q.v);
        }
    };

    template <>
    struct Formatter<Mat44>
    {
        using Type = Mat44;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Mat44& mat) const
        {
            return FormatTo(out, "(cx{}, cy{}, cz{}, cw{})", mat.cx, mat.cy, mat.cz, mat.cw);
        }
    };
}

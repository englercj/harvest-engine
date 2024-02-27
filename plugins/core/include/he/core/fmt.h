// Copyright Chad Engler

#pragma once

#include "he/core/ascii.h"
#include "he/core/compiler.h"
#include "he/core/enum_ops.h"
#include "he/core/limits.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

// Disable unreachable code warnings that result both from our use of [[noreturn]] and from the
// fact that `FmtError` calls `std::terminate`.
// Most parses that would fail will do so at compile time. The only case where we actually hit
// the terminate at runtime is if you mark your format as `FmtRuntime()`, in which case you are
// opting into runtime-parsing.
HE_PUSH_WARNINGS();
HE_DISABLE_MSVC_WARNING(4702);

namespace he
{
    // --------------------------------------------------------------------------------------------
    using Pfn_FmtErrorHandler = void(*)(const char*);
    [[noreturn]] void FmtError(const char* msg);

    // --------------------------------------------------------------------------------------------
    class FmtParseCtx
    {
    public:

    public:
        constexpr FmtParseCtx(StringView fmt, uint32_t argCount, uint32_t nextArgId = 0, Pfn_FmtErrorHandler errorHandler = &FmtError)
            : m_fmt(fmt)
            , m_argCount(argCount)
            , m_nextArgId(nextArgId)
            , m_errorHandler(errorHandler)
        {}

        constexpr const char* Begin() const { return m_fmt.Begin(); }
        constexpr const char* End() const { return m_fmt.End(); }
        constexpr uint32_t Size() const { return m_fmt.Size(); }

        Pfn_FmtErrorHandler ErrorHandler() const { return m_errorHandler; }

        // Intentionally not constexpr to cause a compiler error when called from a consteval context.
        void OnError(const char* msg) const { m_errorHandler(msg); }

    public:
        constexpr void Advance(uint32_t count)
        {
            m_fmt = { m_fmt.Begin() + count, m_fmt.End() };
        }

        constexpr void AdvanceTo(const char* p)
        {
            m_fmt = { p, m_fmt.End() };
        }

        constexpr void CheckArgId(uint32_t id)
        {
            if (m_nextArgId != Limits<uint32_t>::Max && m_nextArgId > 0)
            {
                OnError("Cannot switch from automatic to manual argument indexing");
                return;
            }

            m_nextArgId = Limits<uint32_t>::Max;
            DoCheckArgId(id);
        }

        constexpr uint32_t NextArgId()
        {
            if (m_nextArgId == Limits<uint32_t>::Max)
            {
                OnError("Cannot switch from manual to automatic argument indexing");
                return 0;
            }

            uint32_t id = m_nextArgId++;
            DoCheckArgId(id);
            return id;
        }

    private:
        constexpr void DoCheckArgId(uint32_t id)
        {
            if (id >= m_argCount)
            {
                OnError("Argument not found");
            }
        }

    private:
        StringView m_fmt;
        uint32_t m_argCount;
        uint32_t m_nextArgId;
        Pfn_FmtErrorHandler m_errorHandler;
    };

    // --------------------------------------------------------------------------------------------
    constexpr uint32_t FmtParseUint(const char*& begin, const char* end)
    {
        if (begin >= end || *begin < '0' || *begin > '9')
            return Limits<uint32_t>::Max;

        uint32_t value = 0;
        [[maybe_unused]] uint32_t prev = 0;

        const char* p = begin;
        do
        {
            prev = value;
            value = value * 10 + static_cast<uint32_t>(*p - '0');
            ++p;
        } while (p != end && *p >= '0' && *p <= '9');

        const uint32_t digitsCount = static_cast<uint32_t>(p - begin);
        begin = p;

        constexpr uint32_t MaxBase10Digits = 9;
        if (digitsCount <= MaxBase10Digits)
            return value;

        return Limits<uint32_t>::Max;
    }

    template <typename T>
    constexpr const char* FmtVisitString_ArgId(const char* begin, const char* end, T& visitor, uint32_t& outArgId)
    {
        if (*begin != '}' && *begin != ':')
        {
            if (*begin >= '0' && *begin <= '9')
            {
                uint32_t index = 0;
                if (*begin != '0')
                    index = FmtParseUint(begin, end);
                else
                    ++begin;

                if (begin == end || (*begin != '}' && *begin != ':'))
                    visitor.OnError("Invalid format string");
                else
                    outArgId = visitor.OnArgId(index);

                return begin;
            }

            visitor.OnError("Invalid format string");
            return begin;
        }

        outArgId = visitor.OnArgId();
        return begin;
    }

    template <typename T>
    constexpr const char* FmtVisitString_Field(const char* begin, const char* end, T& visitor)
    {
        ++begin;
        if (begin == end)
        {
            visitor.OnError("Invalid format string");
            return end;
        }

        switch (*begin)
        {
            case '}':
            {
                visitor.OnReplacementField(visitor.OnArgId());
                break;
            }
            case '{':
            {
                visitor.OnText({ begin, begin + 1 });
                break;
            }
            default:
            {
                uint32_t argId = 0;
                begin = FmtVisitString_ArgId(begin, end, visitor, argId);
                const char ch = begin != end ? *begin : '\0';
                switch (ch)
                {
                    case '}':
                        visitor.OnReplacementField(argId);
                        break;
                    case ':':
                        begin = visitor.OnFormatSpec(argId, begin + 1, end);
                        if (begin == end || *begin != '}')
                        {
                            visitor.OnError("Unknown format specifier");
                            return end;
                        }
                        break;
                    default:
                        visitor.OnError("Missing '}' in format string");
                        return end;
                }
                break;
            }
        }

        return begin + 1;
    }

    template <typename T>
    constexpr void FmtVisitString_Text(const char* begin, const char* end, T& visitor)
    {
        if (begin == end)
            return;

        while (true)
        {
            const char* p = StringView(begin, end).Find('}');

            if (!p)
                return visitor.OnText({ begin, end });

            ++p;

            if (p == end || *p != '}')
                return visitor.OnError("Unmatched '}' in format string");

            visitor.OnText({ begin, p });
            begin = p + 1;
        }
    }

    template <typename T>
    constexpr void FmtVisitString_Short(StringView fmt, T& visitor)
    {
        const char* begin = fmt.Begin();
        const char* end = fmt.End();

        const char* p = begin;
        while (p != end)
        {
            const char ch = *p++;
            if (ch == '{')
            {
                visitor.OnText({ begin, p - 1 });
                begin = p = FmtVisitString_Field(p - 1, end, visitor);
            }
            else if (ch == '}')
            {
                if (p == end || *p != '}')
                    return visitor.OnError("Unmatched '}' in format string");
                visitor.OnText({ begin, p });
                begin = ++p;
            }
        }
        visitor.OnText({ begin, end });
    }

    template <typename T>
    constexpr void FmtVisitString_Long(StringView fmt, T& visitor)
    {
        const char* begin = fmt.Begin();
        const char* end = fmt.End();

        while (begin != end)
        {
            const char* p = begin;

            if (*p != '{')
            {
                StringView text(begin + 1, end);
                p = text.Find('{');
                if (!p)
                    return FmtVisitString_Text(begin, end, visitor);
            }

            FmtVisitString_Text(begin, p, visitor);
            begin = FmtVisitString_Field(p, end, visitor);
        }
    }

    template <typename T>
    constexpr void FmtVisitString(StringView fmt, T& visitor)
    {
        // Optimization for small strings to use a simpler loop.
        if (fmt.Size() < 32)
            FmtVisitString_Short(fmt, visitor);
        else
            FmtVisitString_Long(fmt, visitor);
    }

    // --------------------------------------------------------------------------------------------
    enum class FmtSpecAlign : uint8_t
    {
        None,       ///< No special alignment.  Default.
        Left,       ///< '<' = Align output to the left.
        Right,      ///< '>' = Align output to the right.
        Center,     ///< '^' = Align output to the center.
        Numeric,    ///< For internal use. Set when a number has a leading zero fill.
    };

    enum class FmtSpecSign : uint8_t
    {
        None,       ///< No special sign handling.
        Minus,      ///< '-' = Print sign for negative numbers.
        Plus,       ///< '+' = Print sign for negative and non-negative numbers.
        Space,      ///< ' ' = Print sign for negative numbers, and a space for non-negative numbers.
    };

    enum class FmtSpecIntType : uint8_t
    {
        None,       ///< No special formatting.
        Decimal,    ///< 'd' = Print as base 10.
        Char,       ///< 'c' = Print as a character.
        Octal,      ///< 'o' = Print as base 8.
        HexLower,   ///< 'x' = Print as base 16, using lower-case letters. The '#' option adds the prefix "0x" to the output.
        HexUpper,   ///< 'X' = Print as base 16, using upper-case letters. The '#' option adds the prefix "0X" to the output.
        BinLower,   ///< 'b' = Print as base 2. The '#' option adds the prefix "0b" to the output.
        BinUpper,   ///< 'B' = Print as base 2. The '#' option adds the prefix "0B" to the output.
    };

    enum class FmtSpecFloatType : uint8_t
    {
        None,           ///< No special formatting.
        HexLower,       ///< 'a' = Print as base 16, using lower-case letters. Uses 'p' to indicate the exponent.
        HexUpper,       ///< 'A' = Print as base 16, using upper-case letters. Uses 'p' to indicate the exponent.
        ExponentLower,  ///< 'e' = Print using scientific notation, using 'e' to indicate the exponent.
        ExponentUpper,  ///< 'E' = Print using scientific notation, using 'E' to indicate the exponent.
        FixedLower,     ///< 'f' = Print as a fixed point number. Uses `nan` and `inf` for NaN and Infinity, respectively.
        FixedUpper,     ///< 'F' = Print as a fixed point number. Uses `NAN` and `INF` for NaN and Infinity, respectively.
        GeneralLower,   ///< 'g' = For a given precision `p >= 1`, this rounds the number to `p` significant digits and then formats the result in either fixed-point format or in scientific notation, depending on its magnitude. A precision of `0` is treated as equivalent to a precision of `1`.
        GeneralUpper,   ///< 'G' = Same as `'g'` except switches to `'E'` if the number gets too large. The representations of infinity and NaN are uppercased, too.
    };

    enum class FmtSpecEnumType : uint8_t
    {
        None,       ///< No special formatting.
        String,     ///< 's' = Print as string using \ref EnumToString
        Decimal,    ///< 'd' = Print as base 10.
        Char,       ///< 'c' = Print as a character.
        Octal,      ///< 'o' = Print as base 8.
        HexLower,   ///< 'x' = Print as base 16, using lower-case letters. The '#' option adds the prefix "0x" to the output.
        HexUpper,   ///< 'X' = Print as base 16, using upper-case letters. The '#' option adds the prefix "0X" to the output.
        BinLower,   ///< 'b' = Print as base 2. The '#' option adds the prefix "0b" to the output.
        BinUpper,   ///< 'B' = Print as base 2. The '#' option adds the prefix "0B" to the output.
    };

    class FmtSpecFill
    {
    public:
        constexpr void Set(StringView s, const FmtParseCtx& ctx)
        {
            const uint32_t size = s.Size();

            if (size > HE_LENGTH_OF(m_data))
                ctx.OnError("Invalid fill, value is too long");

            for (uint32_t i = 0; i < size; ++i)
                m_data[i] = s[i];

            m_size = static_cast<uint8_t>(size);
        }

        constexpr uint32_t Size() const { return m_size; }
        constexpr const char* Data() const { return m_data; }

        constexpr char& operator[](uint32_t index) { return m_data[index]; }
        constexpr const char& operator[](uint32_t index) const { return m_data[index]; }

        constexpr bool operator==(const FmtSpecFill&) const = default;
        constexpr bool operator!=(const FmtSpecFill&) const = default;

    private:
        char m_data[4]{ ' ', '\0', '\0', '\0' };
        uint8_t m_size{ 1 };
    };

    struct FmtSpec
    {
        uint32_t width{ 0 };
        int32_t precision{ -1 };
        FmtSpecFill fill{};
        FmtSpecAlign align{ FmtSpecAlign::None };
        FmtSpecSign sign{ FmtSpecSign::None };
        bool alt{ false }; // alternate form ('#')

        constexpr bool operator==(const FmtSpec&) const = default;
        constexpr bool operator!=(const FmtSpec&) const = default;
    };

    struct FmtSpecInt : FmtSpec { FmtSpecIntType type{ FmtSpecIntType::None }; };
    struct FmtSpecFloat : FmtSpec { FmtSpecFloatType type{ FmtSpecFloatType::None }; };
    struct FmtSpecEnum : FmtSpec { FmtSpecEnumType type{ FmtSpecEnumType::None }; };

    constexpr FmtSpecAlign FmtCharToAlign(char ch)
    {
        switch (ch)
        {
            case '<': return FmtSpecAlign::Left;
            case '>': return FmtSpecAlign::Right;
            case '^': return FmtSpecAlign::Center;
            default: return FmtSpecAlign::None;
        }
    }

    constexpr FmtSpecSign FmtCharToSign(char ch)
    {
        switch (ch)
        {
            case '+': return FmtSpecSign::Plus;
            case '-': return FmtSpecSign::Minus;
            case ' ': return FmtSpecSign::Space;
            default: return FmtSpecSign::None;
        }
    }

    constexpr FmtSpecIntType FmtCharToType(char ch, FmtSpecIntType)
    {
        switch (ch)
        {
            case 'd': return FmtSpecIntType::Decimal;
            case 'c': return FmtSpecIntType::Char;
            case 'o': return FmtSpecIntType::Octal;
            case 'x': return FmtSpecIntType::HexLower;
            case 'X': return FmtSpecIntType::HexUpper;
            case 'b': return FmtSpecIntType::BinLower;
            case 'B': return FmtSpecIntType::BinUpper;
            default: return FmtSpecIntType::None;
        }
    }

    constexpr FmtSpecFloatType FmtCharToType(char ch, FmtSpecFloatType)
    {
        switch (ch)
        {
            case 'a': return FmtSpecFloatType::HexLower;
            case 'A': return FmtSpecFloatType::HexUpper;
            case 'e': return FmtSpecFloatType::ExponentLower;
            case 'E': return FmtSpecFloatType::ExponentUpper;
            case 'f': return FmtSpecFloatType::FixedLower;
            case 'F': return FmtSpecFloatType::FixedUpper;
            case 'g': return FmtSpecFloatType::GeneralLower;
            case 'G': return FmtSpecFloatType::GeneralUpper;
            default: return FmtSpecFloatType::None;
        }
    }

    constexpr FmtSpecEnumType FmtCharToType(char ch, FmtSpecEnumType)
    {
        switch (ch)
        {
            case 's': return FmtSpecEnumType::String;
            case 'd': return FmtSpecEnumType::Decimal;
            case 'o': return FmtSpecEnumType::Octal;
            case 'x': return FmtSpecEnumType::HexLower;
            case 'X': return FmtSpecEnumType::HexUpper;
            case 'b': return FmtSpecEnumType::BinLower;
            case 'B': return FmtSpecEnumType::BinUpper;
            default: return FmtSpecEnumType::None;
        }
    }

    constexpr uint32_t FmtGetCodePointLength(char ch)
    {
        constexpr const char CodePointLengths[] = "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\2\2\2\2\3\3\4";
        const uint32_t len = CodePointLengths[static_cast<uint8_t>(ch) >> 3];
        return len ? len : 1;
    }

    constexpr const char* ParseFmtSpecAlign(const char* begin, const char* end, const FmtParseCtx& ctx, FmtSpecAlign& out, FmtSpecFill& fill)
    {
        const char* p = begin + FmtGetCodePointLength(*begin);
        if ((end - p) <= 0)
            p = begin;

        while (true)
        {
            const FmtSpecAlign align = FmtCharToAlign(*p);
            if (align != FmtSpecAlign::None)
            {
                if (p != begin)
                {
                    const char ch = *begin;
                    if (ch == '{')
                    {
                        ctx.OnError("Invalid fill character '{'");
                        return begin;
                    }
                    const uint32_t len = static_cast<uint32_t>(p - begin);
                    fill.Set({ begin, len }, ctx);
                    begin = p + 1;
                }
                else
                {
                    ++begin;
                }
                out = align;
                break;
            }
            else if (p == begin)
            {
                break;
            }
            p = begin;
        }

        return begin;
    }

    constexpr const char* ParseFmtSpecPrecision(const char* begin, const char* end, const FmtParseCtx& ctx, int32_t& out)
    {
        if (*begin != '.')
            return begin;

        ++begin;
        const char ch = begin != end ? *begin : '\0';
        if (IsNumeric(ch))
        {
            const uint32_t precision = FmtParseUint(begin, end);
            if (precision <= Limits<int32_t>::Max)
                out = static_cast<int32_t>(precision);
            else
                ctx.OnError("Precision number is too big");
        }
        // TODO: Dynamic precision that is pulled from format args.
        // else if (ch == '{')
        // {
        //     ++begin;
        //     if (begin != end)
        //         begin = FmtVisitString_ArgId(begin, end, ..., argId);
        //     if (begin == end || *begin++ != '}')
        //     {
        //         ctx.OnError("Invalid format string");
        //         return begin;
        //     }
        // }
        else
        {
            ctx.OnError("Missing precision specifier");
        }

        return begin;
    }

    constexpr const char* ParseFmtSpecWidth(const char* begin, const char* end, const FmtParseCtx& ctx, uint32_t& out)
    {
        if (IsNumeric(*begin))
        {
            const uint32_t width = FmtParseUint(begin, end);
            if (width != Limits<uint32_t>::Max)
                out = width;
            else
                ctx.OnError("Width number is too big");
        }
        // TODO: Dynamic width that is pulled from format args.
        // else if (*begin == '{')
        // {
        //     ++begin;
        //     if (begin != end)
        //         begin = FmtVisitString_ArgId(begin, end, ..., argId);
        //     if (begin == end || *begin != '}')
        //     {
        //         ctx.OnError("Invalid format string");
        //         return begin;
        //     }
        //     ++begin;
        // }
        return begin;
    }

    template <Enum T>
    constexpr const char* ParseFmtSpecType(const char* begin, const char*, const FmtParseCtx& ctx, T& out)
    {
        T type = FmtCharToType(*begin++, T{});
        if (type == T::None)
            ctx.OnError("Invalid type specifier");

        out = type;
        return begin;
    }

    constexpr const char* ParseFmtSpec(const char* begin, const char* end, const FmtParseCtx& ctx, FmtSpec& spec)
    {
        if (begin == end)
            return begin;

        // Alignment
        begin = ParseFmtSpecAlign(begin, end, ctx, spec.align, spec.fill);
        if (begin == end)
            return begin;

        // Sign
        FmtSpecSign sign = FmtCharToSign(*begin);
        if (sign != FmtSpecSign::None)
        {
            spec.sign = sign;
            if (++begin == end)
                return begin;
        }

        // Alternate form
        if (*begin == '#')
        {
            spec.alt = true;
            if (++begin == end)
                return begin;
        }

        // Leading zeros
        if (*begin == '0')
        {
            if (spec.align == FmtSpecAlign::None)
                spec.align = FmtSpecAlign::Numeric;
            spec.fill[0] = '0';
            if (++begin == end)
                return begin;
        }

        // Width
        begin = ParseFmtSpecWidth(begin, end, ctx, spec.width);
        if (begin == end)
            return begin;

        // Precision
        begin = ParseFmtSpecPrecision(begin, end, ctx, spec.precision);
        if (begin == end)
            return begin;

        return begin;
    }

    constexpr const char* ParseFmtSpec(const FmtParseCtx& ctx, FmtSpec& spec)
    {
        return ParseFmtSpec(ctx.Begin(), ctx.End(), ctx, spec);
    }

    template <typename T>
    constexpr const char* ParseFmtSpec(const FmtParseCtx& ctx, T& spec)
    {
        const char* begin = ctx.Begin();
        const char* end = ctx.End();
        const uint32_t len = static_cast<uint32_t>(end - begin);

        // Special case for the common case of a simple format specifier
        if (len > 1 && begin[1] == '}' && IsAlpha(*begin))
        {
            return ParseFmtSpecType(begin, end, ctx, spec.type);
        }

        begin = ParseFmtSpec(begin, end, ctx, spec);
        if (begin == end)
            return begin;

        // Format type
        if (begin != end && *begin != '}')
        {
            begin = ParseFmtSpecType(begin, end, ctx, spec.type);
        }

        return begin;
    }

    constexpr const char* CheckIntFmtSpec(const char* it, const FmtParseCtx& ctx, const FmtSpecInt& spec)
    {
        if (spec.precision != -1)
        {
            ctx.OnError("Precision not allowed for integral types");
        }

        return it;
    }

    constexpr const char* CheckCharFmtSpec(const char* it, const FmtParseCtx& ctx, const FmtSpecInt& spec)
    {
        if (spec.precision != -1)
        {
            ctx.OnError("Precision not allowed for integral types");
        }

        if (spec.type == FmtSpecIntType::None || spec.type == FmtSpecIntType::Char)
        {
            if (spec.align == FmtSpecAlign::Numeric || spec.sign != FmtSpecSign::None || spec.alt)
                ctx.OnError("Invalid format specifier for char");
        }

        return it;
    }

    constexpr const char* CheckPointerFmtSpec(const char* it, const FmtParseCtx& ctx, const FmtSpec& spec)
    {
        if (spec.precision != -1)
        {
            ctx.OnError("Precision not allowed for pointer types");
        }

        return it;
    }

    // --------------------------------------------------------------------------------------------
    template <typename T> struct _FmtPtr;
    template <typename T> struct _FmtPtr<T*> { static const void* Apply(T* p) { return static_cast<const void*>(p); } };
    template <> struct _FmtPtr<nullptr_t> { static const void* Apply(nullptr_t) { return reinterpret_cast<const void*>(static_cast<uintptr_t>(0)); } };

    template <typename T>
    const void* FmtPtr(T p) { return _FmtPtr<T>::Apply(p); }

    template <typename It>
    struct FmtJoinView
    {
        It begin;
        It end;
        StringView sep;
    };

    template <typename T>
    inline FmtJoinView<T> FmtJoin(T begin, T end, StringView sep) { return { begin, end, sep }; }

    template <typename T, uint32_t N>
    inline FmtJoinView<const T*> FmtJoin(const T (&data)[N], StringView sep) { return { data, data + N, sep }; }

    template <typename R>
    inline auto FmtJoin(R&& range, StringView sep) -> FmtJoinView<decltype(DeclVal<R&>().begin())>
    {
        return { range.begin(), range.end(), sep };
    }

    struct _FmtStringRuntime { StringView fmt; };
    inline _FmtStringRuntime FmtRuntime(StringView fmt) { return { fmt }; }

    // --------------------------------------------------------------------------------------------
    template <typename T> struct Formatter { Formatter() = delete; };

    template <>
    struct Formatter<bool>
    {
        using Type = bool;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<char>
    {
        using Type = char;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckCharFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<wchar_t>
    {
        using Type = wchar_t;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<char8_t>
    {
        using Type = char8_t;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<char16_t>
    {
        using Type = char16_t;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<char32_t>
    {
        using Type = char32_t;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<signed char>
    {
        using Type = signed char;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<short>
    {
        using Type = short;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<int>
    {
        using Type = int;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<long>
    {
        using Type = long;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<long long>
    {
        using Type = long long;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<unsigned char>
    {
        using Type = unsigned char;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<unsigned short>
    {
        using Type = unsigned short;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<unsigned int>
    {
        using Type = unsigned int;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<unsigned long>
    {
        using Type = unsigned long;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<unsigned long long>
    {
        using Type = unsigned long long;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckIntFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecInt spec{};
    };

    template <>
    struct Formatter<float>
    {
        using Type = float;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecFloat spec{};
    };

    template <>
    struct Formatter<double>
    {
        using Type = double;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecFloat spec{};
    };

    template <>
    struct Formatter<long double>
    {
        using Type = long double;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpecFloat spec{};
    };

    template <>
    struct Formatter<const void*>
    {
        using Type = const void*;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return CheckPointerFmtSpec(ParseFmtSpec(ctx, spec), ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpec spec{};
    };

    template <> struct Formatter<void*> : Formatter<const void*> {};
    template <> struct Formatter<nullptr_t> : Formatter<const void*> {};

    template <>
    struct Formatter<StringView>
    {
        using Type = StringView;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, const Type& value) const;
        FmtSpec spec{};
    };

    template <>
    struct Formatter<const char*>
    {
        using Type = const char*;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, Type value) const;
        FmtSpec spec{};
    };

    template <> struct Formatter<char*> : Formatter<const char*> {};

    template <uint32_t N>
    struct Formatter<char[N]>
    {
        using Type = char[N];
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, const char (&value)[N]) const
        {
            Formatter<StringView> f;
            f.spec = spec;
            f.Format(out, { value });
        }
        FmtSpec spec{};
    };

    template <Enum T>
    struct Formatter<T>
    {
        using Type = T;
        constexpr const char* Parse(const FmtParseCtx& ctx) { return ParseFmtSpec(ctx, spec); }
        void Format(String& out, Type value) const
        {
            if (spec.type == FmtSpecEnumType::None || spec.type == FmtSpecEnumType::String)
            {
                Formatter<const char*> f;
                static_cast<FmtSpec&>(f.spec) = static_cast<const FmtSpec&>(spec);
                f.Format(out, EnumToString(value));
            }
            else
            {
                Formatter<UnderlyingType<T>> f;
                static_cast<FmtSpec&>(f.spec) = static_cast<const FmtSpec&>(spec);
                f.spec.type = FmtSpecIntType::Decimal;

                switch (spec.type)
                {
                    case FmtSpecEnumType::None:
                    case FmtSpecEnumType::String:
                        break;
                    case FmtSpecEnumType::Decimal: f.spec.type = FmtSpecIntType::Decimal; break;
                    case FmtSpecEnumType::Char: f.spec.type = FmtSpecIntType::Char; break;
                    case FmtSpecEnumType::Octal: f.spec.type = FmtSpecIntType::Octal; break;
                    case FmtSpecEnumType::HexLower: f.spec.type = FmtSpecIntType::HexLower; break;
                    case FmtSpecEnumType::HexUpper: f.spec.type = FmtSpecIntType::HexUpper; break;
                    case FmtSpecEnumType::BinLower: f.spec.type = FmtSpecIntType::BinLower; break;
                    case FmtSpecEnumType::BinUpper: f.spec.type = FmtSpecIntType::BinUpper; break;
                }

                f.Format(out, EnumToValue(value));
            }
        }
        FmtSpecEnum spec{};
    };

    template <typename T> struct _FmtIteratorValueType { using Type = typename T::ElementType;  };
    template <Pointer T> struct _FmtIteratorValueType<T> { using Type = RemovePointer<T>; };

    template <typename T>
    struct Formatter<FmtJoinView<T>>
    {
        using Type = FmtJoinView<T>;
        using ValueType = RemoveCV<typename _FmtIteratorValueType<T>::Type>;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return valueFormatter.Parse(ctx); }

        void Format(String& out, const Type& value) const
        {
            T it = value.begin;
            if (it != value.end)
            {
                valueFormatter.Format(out, *it++);
                while (it != value.end)
                {
                    out.Append(value.sep);
                    valueFormatter.Format(out, *it++);
                }
            }
        }

        Formatter<ValueType> valueFormatter;
    };

    template <typename T>
    inline constexpr bool HasFormatter = IsConstructible<Formatter<RemoveCVRef<T>>>;

    // --------------------------------------------------------------------------------------------
    struct FmtArg
    {
        const void* value{ nullptr };
        void(*format)(FmtParseCtx& ctx, String& out, const void* arg){ nullptr };
    };

    template <typename T>
    void FormatArg(FmtParseCtx& ctx, String& out, const void* value)
    {
        Formatter<RemoveCVRef<T>> formatter;
        const char* parseEnd = formatter.Parse(const_cast<const FmtParseCtx&>(ctx));
        ctx.AdvanceTo(parseEnd);
        formatter.Format(out, *static_cast<const T*>(value));
    }

    template <typename T>
    constexpr FmtArg MakeFmtArg(const T& value)
    {
        static_assert(HasFormatter<T>, "Cannot format argument, missing Formatter<T> specialization.");

        FmtArg arg;
        arg.value = &value;
        arg.format = FormatArg<T>;
        return arg;
    }

    template <typename... Args>
    struct FmtArgsStorage
    {
        static constexpr uint32_t ArgCount = sizeof...(Args);

        constexpr FmtArgsStorage(Args&&... args)
            : data{ MakeFmtArg(Forward<Args>(args))... }
        {}

        FmtArg data[ArgCount > 0 ? ArgCount : 1];
    };

    struct FmtArgs
    {
        constexpr FmtArgs() = default;

        constexpr FmtArgs(const FmtArg* args, uint32_t count)
            : args(args)
            , count(count)
        {}

        template <typename... Args>
        constexpr FmtArgs(const FmtArgsStorage<Args...>& storage)
            : args(storage.data)
            , count(FmtArgsStorage<Args...>::ArgCount)
        {}

        const FmtArg* args{ nullptr };
        const uint32_t count{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    template <typename... Args>
    class FmtCheckVisitor
    {
    public:
        static constexpr uint32_t ArgCount = sizeof...(Args);

        constexpr explicit FmtCheckVisitor(StringView fmt, Pfn_FmtErrorHandler errorHandler)
            : m_ctx{ fmt, ArgCount, 0, errorHandler }
            , m_parseFuncs{ &CheckParseThunk<Args>... }
        {
            static_assert((HasFormatter<Args> && ...), "Missing Formatter<T> for argument type. Are you missing an include?");
        }

        constexpr void OnText(StringView) {}
        constexpr uint32_t OnArgId() { return m_ctx.NextArgId(); }
        constexpr uint32_t OnArgId(uint32_t id) { m_ctx.CheckArgId(id); return id; }

        constexpr void OnReplacementField(uint32_t) {}

        constexpr const char* OnFormatSpec(uint32_t id, const char* begin, const char*)
        {
            m_ctx.Advance(static_cast<uint32_t>(begin - m_ctx.Begin()));
            return id < ArgCount ? m_parseFuncs[id](m_ctx) : begin;
        }

        // Intentionally not constexpr to cause a compiler error when called from a consteval context.
        void OnError(const char* msg) { m_ctx.OnError(msg); }

    private:
        using Pfn_Parse = const char* (*)(const FmtParseCtx&);

        template <typename T>
        static constexpr const char* CheckParseThunk(const FmtParseCtx& ctx) { return Formatter<RemoveCVRef<T>>().Parse(ctx); }

        FmtParseCtx m_ctx;
        Pfn_Parse m_parseFuncs[ArgCount > 0 ? ArgCount : 1];
    };

    // --------------------------------------------------------------------------------------------
    class FmtWriteVisitor
    {
    public:
        FmtWriteVisitor(String& out, StringView fmt, FmtArgs args, Pfn_FmtErrorHandler errorHandler)
            : m_out(out)
            , m_ctx{ fmt, args.count, 0, errorHandler }
            , m_args(args)
        {}

        void OnText(StringView text)
        {
            m_out.Append(text);
        }

        uint32_t OnArgId() { return m_ctx.NextArgId(); }
        uint32_t OnArgId(uint32_t id) { m_ctx.CheckArgId(id); return id; }

        void OnReplacementField(uint32_t id)
        {
            if (id >= m_args.count)
            {
                m_ctx.OnError("Argument not found");
                return;
            }

            FmtParseCtx ctx({}, 0, 0, m_ctx.ErrorHandler());
            const FmtArg& arg = m_args.args[id];
            arg.format(ctx, m_out, arg.value);
        }

        const char* OnFormatSpec(uint32_t id, const char* begin, const char*)
        {
            if (id >= m_args.count)
            {
                m_ctx.OnError("Argument not found");
                return m_ctx.Begin();
            }

            m_ctx.Advance(static_cast<uint32_t>(begin - m_ctx.Begin()));

            const FmtArg& arg = m_args.args[id];
            arg.format(m_ctx, m_out, arg.value);

            return m_ctx.Begin();
        }

        void OnError(const char* msg) { m_ctx.OnError(msg); }

    private:
        String& m_out;
        FmtParseCtx m_ctx;
        FmtArgs m_args;
    };

    // --------------------------------------------------------------------------------------------
    template <typename... Args>
    class _FmtString
    {
    public:
        template <typename T> requires(IsConvertible<const T&, StringView>)
        consteval inline _FmtString(const T& fmt, Pfn_FmtErrorHandler errorHandler = &FmtError)
            : m_fmt(fmt)
        {
            FmtCheckVisitor<RemoveCVRef<Args>...> checker(m_fmt, errorHandler);
            FmtVisitString(m_fmt, checker);
        }

        _FmtString(_FmtStringRuntime r)
            : m_fmt(r.fmt)
        {}

        inline operator StringView() const { return m_fmt; }

    private:
        StringView m_fmt;
    };

    template <typename... Args>
    using FmtString = _FmtString<IdentityType<Args>...>;

    // --------------------------------------------------------------------------------------------
    void VFormatTo(String& out, StringView fmt, FmtArgs args, Pfn_FmtErrorHandler errorHandler = &FmtError);

    template <Pfn_FmtErrorHandler ErrorHandler = &FmtError, typename... Args>
    void FormatTo(String& out, FmtString<Args...> fmt, Args&&... args)
    {
        FmtArgsStorage<Args...> store(Forward<Args>(args)...);
        VFormatTo(out, fmt, store, ErrorHandler);
    }

    template <Pfn_FmtErrorHandler ErrorHandler = &FmtError, typename... Args>
    [[nodiscard]] String Format(FmtString<Args...> fmt, Args&&... args)
    {
        String out;
        FmtArgsStorage<Args...> store(Forward<Args>(args)...);
        VFormatTo(out, fmt, store, ErrorHandler);
        return out;
    }
}

HE_POP_WARNINGS();

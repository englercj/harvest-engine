// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/enum_ops.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

namespace he
{
    // High-level type of an argument.
    enum class ArgType : uint8_t
    {
        Boolean,
        Integer,
        Float,
        String,
    };

    // Flags to control behavior of an argument.
    enum class ArgFlag : uint32_t
    {
        None        = 0,
        Required    = 1 << 0,
    };
    HE_ENUM_FLAGS(ArgFlag);

    constexpr ArgFlag InternalSignedFlag = ArgFlag(static_cast<uint32_t>(1) << 31);
    constexpr ArgFlag InternalVectorFlag = ArgFlag(static_cast<uint32_t>(1) << 30);

    // Gives the ArgType value based on T.
    template <typename T> struct ArgTypeOf;
    template <> struct ArgTypeOf<bool> { static constexpr ArgType Value = ArgType::Boolean; };
    template <> struct ArgTypeOf<const char*> { static constexpr ArgType Value = ArgType::String; };
    template <Integral T> struct ArgTypeOf<T> { static constexpr ArgType Value = ArgType::Integer; };
    template <FloatingPoint T> struct ArgTypeOf<T> { static constexpr ArgType Value = ArgType::Float; };

    // Gives the signed flag if T is signed.
    template <typename T> struct ArgSignedFlag { static constexpr ArgFlag Value = (IsSigned<T> ? InternalSignedFlag : ArgFlag::None); };

    // Represents the result of parsing arguments.
    struct ArgResult
    {
        enum Code
        {
            Success,
            UnknownArg,
            MissingRequiredArg,
            InvalidFormat,
            InvalidValue,
            InvalidArgDesc,
        };

        Code code{ Code::Success };
        String msg{};
        Vector<const char*> values{};

        explicit ArgResult() = default;
        explicit ArgResult(Code c) noexcept : code(c) {}
        explicit ArgResult(Code c, const char* msg) noexcept : code(c), msg(msg) {}
        explicit ArgResult(Code c, String&& msg) noexcept : code(c), msg(Move(msg)) {}

        [[nodiscard]] explicit operator bool() const { return code == Success; }
    };

    class ArgDesc
    {
    public:
        template <typename T>
        ArgDesc(T& v, char shortArg, const char* longArg = nullptr, const char* description = nullptr, ArgFlag flags = ArgFlag::None) noexcept
            : ArgDesc(ArgTypeOf<T>::Value, &v, sizeof(T), shortArg, longArg, description, flags | ArgSignedFlag<T>::Value) { }

        template <typename T>
        ArgDesc(T& v, const char* longArg, const char* description = nullptr, ArgFlag flags = ArgFlag::None) noexcept
            : ArgDesc(ArgTypeOf<T>::Value, &v, sizeof(T), '\0', longArg, description, flags | ArgSignedFlag<T>::Value) { }

        template <typename T>
        ArgDesc(Vector<T>& v, char shortArg, const char* longArg = nullptr, const char* description = nullptr, ArgFlag flags = ArgFlag::None) noexcept
            : ArgDesc(ArgTypeOf<T>::Value, &v, sizeof(T), shortArg, longArg, description, flags | InternalVectorFlag | ArgSignedFlag<T>::Value) { }

        template <typename T>
        ArgDesc(Vector<T>& v, const char* longArg, const char* description = nullptr, ArgFlag flags = ArgFlag::None) noexcept
            : ArgDesc(ArgTypeOf<T>::Value, &v, sizeof(T), '\0', longArg, description, flags | InternalVectorFlag | ArgSignedFlag<T>::Value) { }

        bool HasValue() const { return m_hasValue; }

        ArgType Type() const { return m_type; }
        uint8_t TypeSize() const { return m_size; }

        bool IsRequired() const { return HasFlag(m_flags, ArgFlag::Required); }
        bool IsSignedValue() const { return HasFlag(m_flags, InternalSignedFlag); }
        bool IsVectorValue() const { return HasFlag(m_flags, InternalVectorFlag); }

        char ShortName() const { return m_shortArg; }
        const char* LongName() const { return m_longArg; }
        const char* Description() const { return m_description; }

    private:
        friend ArgResult ParseArgs(Span<ArgDesc> descs, int32_t argc, const char* const* argv);

        static ArgResult ReadFlag(Span<ArgDesc>& descs, const char* arg, ArgDesc*& desc);

        ArgResult ReadIntValue(const char* value);
        ArgResult ReadFloatValue(const char* value);
        ArgResult ReadValue(const char* value);

        template <typename T>
        void SetOrPushValue(const T& value);

    private:
        ArgType m_type;
        void* m_buffer;
        uint8_t m_size;

        char m_shortArg;
        const char* m_longArg;
        const char* m_description;
        ArgFlag m_flags;

        bool m_hasValue{ false };

    private:
        ArgDesc(ArgType type, void* buffer, size_t size, char shortArg, const char* longArg, const char* description, ArgFlag flags) noexcept
            : m_type(type)
            , m_buffer(buffer)
            , m_size(static_cast<uint8_t>(size))
            , m_shortArg(shortArg)
            , m_longArg(longArg)
            , m_description(description)
            , m_flags(flags)
        { }
    };

    ArgResult ParseArgs(Span<ArgDesc> descs, int32_t argc, const char* const* argv);
    String MakeHelpString(Span<const ArgDesc> descs, const char* arg0, const ArgResult* result = nullptr);
}

// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"
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
    HE_ENUM_FLAGS(ArgFlag)

    constexpr ArgFlag InternalSignedFlag = ArgFlag(static_cast<uint32_t>(1) << 31);
    constexpr ArgFlag InternalVectorFlag = ArgFlag(static_cast<uint32_t>(1) << 30);

    // Gives the ArgType value based on T.
    template <typename T, typename U = decltype(nullptr)> struct ArgTypeOf;
    template <> struct ArgTypeOf<bool> { static constexpr ArgType value = ArgType::Boolean; };
    template <> struct ArgTypeOf<const char*> { static constexpr ArgType value = ArgType::String; };
    template <typename T> struct ArgTypeOf<T, HE_REQUIRED(std::is_integral_v<T>)> { static constexpr ArgType value = ArgType::Integer; };
    template <typename T> struct ArgTypeOf<T, HE_REQUIRED(std::is_floating_point_v<T>)> { static constexpr ArgType value = ArgType::Float; };

    // Gives the signed flag if T is signed.
    template <typename T> struct ArgSignedFlag { static constexpr ArgFlag value = (std::is_signed_v<T> ? InternalSignedFlag : ArgFlag::None); };

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
        };

        Code code;
        String msg;
        Vector<const char*> values;

        explicit ArgResult(Allocator& a) : code(Success), msg(a), values(a) {}
        explicit ArgResult(Allocator& a, Code c) : code(c), msg(a), values(a) {}
        explicit ArgResult(Allocator& a, Code c, const char* msg) : code(c), msg(a, msg), values(a) {}
        explicit ArgResult(Allocator& a, Code c, String&& msg) : code(c), msg(a, Move(msg)), values(a) {}

        operator bool() const { return code == Success; }
    };

    struct ArgDesc
    {
        template <typename T>
        ArgDesc(T& v, char shortArg, const char* longArg = nullptr, const char* description = nullptr, ArgFlag flags = ArgFlag::None)
            : ArgDesc(ArgTypeOf<T>::value, &v, sizeof(T), shortArg, longArg, description, flags | ArgSignedFlag<T>::value) { }

        template <typename T>
        ArgDesc(T& v, const char* longArg, const char* description = nullptr, ArgFlag flags = ArgFlag::None)
            : ArgDesc(ArgTypeOf<T>::value, &v, sizeof(T), 0, longArg, description, flags | ArgSignedFlag<T>::value) { }

        template <typename T>
        ArgDesc(Vector<T>& v, char shortArg, const char* longArg = nullptr, const char* description = nullptr, ArgFlag flags = ArgFlag::None)
            : ArgDesc(ArgTypeOf<T>::value, &v, sizeof(T), shortArg, longArg, description, flags | InternalVectorFlag | ArgSignedFlag<T>::value) { }

        template <typename T>
        ArgDesc(Vector<T>& v, const char* longArg, const char* description = nullptr, ArgFlag flags = ArgFlag::None)
            : ArgDesc(ArgTypeOf<T>::value, &v, sizeof(T), 0, longArg, description, flags | InternalVectorFlag | ArgSignedFlag<T>::value) { }

        ArgType type;
        void* buffer;
        size_t size;

        char shortArg;
        const char* longArg;
        const char* description;
        ArgFlag flags;

        bool hasValue = false;

    private:
        ArgDesc(ArgType type, void* buffer, size_t size, char shortArg, const char* longArg, const char* description, ArgFlag flags)
            : type(type)
            , buffer(buffer)
            , size(size)
            , shortArg(shortArg)
            , longArg(longArg)
            , description(description)
            , flags(flags)
        { }
    };

    ArgResult ParseArgs(Allocator& allocator, Span<ArgDesc> descs, int32_t argc, const char* const* argv);
    String MakeHelpString(Allocator& allocator, Span<ArgDesc> descs, const char* arg0, const ArgResult* result = nullptr);
}

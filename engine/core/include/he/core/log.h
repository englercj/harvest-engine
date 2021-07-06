// Copyright Chad Engler

#pragma once

#include "he/core/config.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include "fmt/format.h"

#include <type_traits>

#define HE_LOG_LEVEL_TRACE  0   ///< Detailed tracing for a system, usually disbaled unless tracking a bug.
#define HE_LOG_LEVEL_DEBUG  1   ///< Debug information useful for developers, usually disabled in non-internal builds.
#define HE_LOG_LEVEL_INFO   2   ///< Informational logging useful for general users, usually enabled.
#define HE_LOG_LEVEL_WARN   3   ///< Warning that something has gone wrong, but it is recoverable.
#define HE_LOG_LEVEL_ERROR  4   ///< Error notification that something has gone wrong, may experience strange behavior.

/// \def HE_LOG_LEVEL_ENABLED
/// Defines the log level enabled at compile time. Any level less than this defined value
/// is compiled out of the application. This defaults to Debug for internal builds and
/// Info for non-internal builds.
#if !defined(HE_LOG_LEVEL_ENABLED)
    #if HE_INTERNAL_BUILD
        #define HE_LOG_LEVEL_ENABLED HE_LOG_LEVEL_DEBUG
    #else
        #define HE_LOG_LEVEL_ENABLED HE_LOG_LEVEL_INFO
    #endif
#endif

/// \def HE_LOG_MESSAGE_KEY
/// Defines the quoted string to use as the key for the special "message" pair that is
/// created in the HE_MSG and HE_LOGF macros. By default this is defined as "message".
#if !defined(HE_LOG_MESSAGE_KEY)
    #define HE_LOG_MESSAGE_KEY "message"
#endif

/// Macro that simplifies the construction of a key-value pair for a log.
/// You can also specify a format string as the value followed by the format arguments.
///
/// \param k The unquoted name of the key
/// \param v The value of the pair, which can be integral, floating point, or a string.
#define HE_KV(k, v, ...) (he::LogKV{ #k, v, ##__VA_ARGS__ })

/// Shortcut macro for creating a key-value pair that represents the string message of the log.
/// This generates a key-value pair using the key "message".
///
/// \param fmt The format string to log.
#define HE_MSG(fmt, ...) (he::LogKV{ HE_LOG_MESSAGE_KEY, fmt, ##__VA_ARGS__ })

/// Helper macro to dispatch a KV to a function. This make calling a template function
/// that can handle the values of a KV pair much simpler.
///
/// \param kv The LogKV object to dispatch.
/// \param func The function to dispatch the key and value to.
#define HE_KV_DISPATCH(kv, func, ...) \
    do { \
        switch (kv.type) { \
            case LogKV::ValueType::Bool: func(kv.key, kv.b, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Int8: func(kv.key, kv.i8, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Int16: func(kv.key, kv.i16, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Int32: func(kv.key, kv.i32, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Int64: func(kv.key, kv.i64, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Uint8: func(kv.key, kv.u8, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Uint16: func(kv.key, kv.u16, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Uint32: func(kv.key, kv.u32, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Uint64: func(kv.key, kv.u64, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Float: func(kv.key, kv.f, ##__VA_ARGS__); break; \
            case LogKV::ValueType::Double: func(kv.key, kv.d, ##__VA_ARGS__); break; \
            case LogKV::ValueType::String: func(kv.key, kv.str.data(), ##__VA_ARGS__); break; \
        } \
    } while (0)

/// Base logging macro. Most users should not call this directly, but instead use the
/// level-specific logging macros.
///
/// \param lvl The level of the log.
/// \param cat The unquoted name of the category.
#define HE_LOG(lvl, cat, ...) \
    do { \
        if constexpr (he::LogLevel::lvl >= HE_LOG_LEVEL_ENABLED) { \
            static constexpr he::LogSource LogMsgSource_{ he::LogLevel::lvl, HE_LINE, HE_FILE, __FUNCTION__ }; \
            const he::LogKV kvLogList_[]{ __VA_ARGS__ }; \
            he::Log(LogMsgSource_, kvLogList_, HE_LENGTH_OF(kvLogList_)); \
        } \
    } while(0)

/// Base string logging macro. Most users should not call this directly, but instead use the
/// level-specific logging macros.
/// This macro is equivalent to having a single key-value pair with the key "message" and a
/// value from the format string.
///
/// \param lvl The level of the log.
/// \param cat The unquoted name of the category.
/// \param fmt The format string for the message.
#define HE_LOGF(lvl, cat, fmt, ...) \
    HE_LOG(lvl, cat, HE_KV_MSG(fmt, ##__VA_ARGS__))

/// Logs a set of key-value pairs at the Trace log level.
#define HE_LOG_TRACE(cat, ...)  HE_LOG(Trace, #cat, __VA_ARGS__)

/// Logs a string message at the Trace log level.
#define HE_LOGF_TRACE(cat, ...) HE_LOGF(Trace, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Debug log level.
#define HE_LOG_DEBUG(cat, ...)  HE_LOG(Debug, #cat, __VA_ARGS__)

/// Logs a string message at the Debug log level.
#define HE_LOGF_DEBUG(cat, ...) HE_LOGF(Debug, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Info log level.
#define HE_LOG_INFO(cat, ...)   HE_LOG(Info, #cat, __VA_ARGS__)

/// Logs a string message at the Info log level.
#define HE_LOGF_INFO(cat, ...)  HE_LOGF(Info, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Warning log level.
#define HE_LOG_WARN(cat, ...)   HE_LOG(Warn, #cat, __VA_ARGS__)

/// Logs a string message at the Warning log level.
#define HE_LOGF_WARN(cat, ...)  HE_LOGF(Warn, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Error log level.
#define HE_LOG_ERROR(cat, ...)  HE_LOG(Error, #cat, __VA_ARGS__)

/// Logs a string message at the Error log level.
#define HE_LOGF_ERROR(cat, ...) HE_LOGF(Error, #cat, __VA_ARGS__)

namespace he
{
    /// Enumeration of log levels handled by the logging system.
    enum class LogLevel : uint8_t
    {
        Trace   = HE_LOG_LEVEL_TRACE,   ///< \copydoc HE_LOG_LEVEL_TRACE
        Debug   = HE_LOG_LEVEL_DEBUG,   ///< \copydoc HE_LOG_LEVEL_DEBUG
        Info    = HE_LOG_LEVEL_INFO,    ///< \copydoc HE_LOG_LEVEL_INFO
        Warn    = HE_LOG_LEVEL_WARN,    ///< \copydoc HE_LOG_LEVEL_WARN
        Error   = HE_LOG_LEVEL_ERROR,   ///< \copydoc HE_LOG_LEVEL_ERROR
    };

    /// Returns a log level as a string.
    const char* AsString(LogLevel x);

    /// Source of a log entry.
    /// This structure is not meant to be used directly. Use the logging macros instead.
    struct LogSource
    {
        LogLevel level;         ///< The level of the log entry
        uint32_t line;          ///< The line of the file the log comes from.
        const char* file;       ///< The name of the file the log comes from.
        const char* funcName;   ///< The name of the function the log comes from.
        const char* category;   ///< The name of the category the log comes from.
    };

    /// A key-value pair for a log entry. Values are stored in a union.
    struct LogKV
    {
        enum class ValueType
        {
            Bool,
            Int8,
            Int16,
            Int32,
            Int64,
            Uint8,
            Uint16,
            Uint32,
            Uint64,
            Float,
            Double,
            String,
        };

        LogKV(const char* k, bool v) : key(k), type(ValueType::Bool), b(v) {}
        LogKV(const char* k, int8_t v) : key(k), type(ValueType::Int8), i8(v) {}
        LogKV(const char* k, int16_t v) : key(k), type(ValueType::Int16), i16(v) {}
        LogKV(const char* k, int32_t v) : key(k), type(ValueType::Int32), i32(v) {}
        LogKV(const char* k, int64_t v) : key(k), type(ValueType::Int64), i64(v) {}
        LogKV(const char* k, uint8_t v) : key(k), type(ValueType::Uint8), u8(v) {}
        LogKV(const char* k, uint16_t v) : key(k), type(ValueType::Uint16), u16(v) {}
        LogKV(const char* k, uint32_t v) : key(k), type(ValueType::Uint32), u32(v) {}
        LogKV(const char* k, uint64_t v) : key(k), type(ValueType::Uint64), u64(v) {}
        LogKV(const char* k, float v) : key(k), type(ValueType::Float), f(v) {}
        LogKV(const char* k, double v) : key(k), type(ValueType::Double), d(v) {}

        template <typename T, HE_REQUIRES(IsEnum<T>)>
        constexpr LogKV(const char* k, T v) : LogKV(k, EnumType<T>(v)) {}

        LogKV(const char* k, const char* v)
            : key(k)
            , type(ValueType::String)
        {
            const uint32_t len = String::Length(v);
            str.append(v, v + len + 1); // +1 to include null terminator
        }

        template <typename... Args>
        LogKV(const char* k, const char* fmt, Args&&... args)
            : key(k)
            , type(ValueType::String)
        {
            fmt::format_to(str, fmt, Forward<Args>(args)...);
            str.push_back('\0');
        }

        LogKV(const LogKV&) = delete;
        LogKV& operator=(const LogKV&) = delete;

        LogKV(LogKV&& x) { *this = Move(x); }
        LogKV& operator=(LogKV&& x)
        {
            key = x.key;
            type = x.type;
            switch (type)
            {
                case ValueType::Bool: b = x.b; break;
                case ValueType::Int8: i8 = x.i8; break;
                case ValueType::Int16: i16 = x.i16; break;
                case ValueType::Int32: i32 = x.i32; break;
                case ValueType::Int64: i64 = x.i64; break;
                case ValueType::Uint8: u8 = x.u8; break;
                case ValueType::Uint16: u16 = x.u16; break;
                case ValueType::Uint32: u32 = x.u32; break;
                case ValueType::Uint64: u64 = x.u64; break;
                case ValueType::Float: f = x.f; break;
                case ValueType::Double: d = x.d; break;
                case ValueType::String: str = Move(x.str); break;
            }
            return *this;
        }

        const char* key;
        ValueType type;
        union
        {
            bool b;
            int8_t i8;
            int16_t i16;
            int32_t i32;
            int64_t i64;
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
            uint64_t u64;
            float f;
            double d;
        };
        fmt::basic_memory_buffer<char, 128> str{};
    };

    /// A pointer to a function that handles processing log entries.
    ///
    /// If your sink processes data on another thread, it must copy any parameters it
    /// wants to process later. The structures that are passed in are not garuanteed to
    /// live beyond the function body. However, you don't have to copy the strings in the
    /// LogSource, just the pointers since they point to static memory.
    ///
    /// \param source The source information for this log entry.
    /// \param kvs An array of key-value pairs
    using LogSinkFunc = void(*)(const LogSource& source, const LogKV* kvs, uint32_t count);

    /// Stores the sink to be called when a log entry is created.
    void AddLogSink(LogSinkFunc sink);

    /// Log sink that logs to the debugger.
    void DebugOutputSink(const LogSource& source, const LogKV* kvs, uint32_t count);

    /// Entry point for handling a log entry.
    /// This function is not meant to be called directly. Use the logging macros instead.
    void Log(const LogSource& source, const LogKV* kvs, uint32_t count);
}

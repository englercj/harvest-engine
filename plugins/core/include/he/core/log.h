// Copyright Chad Engler

#pragma once

#include "he/core/config.h"
#include "he/core/debug.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include "fmt/format.h"

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
/// \param ... The format arguments if `v` is a format string specifier.
#define HE_KV(k, v, ...) (he::LogKV{ #k, (v), ##__VA_ARGS__ })

/// Shortcut macro for creating a key-value pair that represents the string message of the log.
/// This generates a key-value pair using the expansion of \see HE_LOG_MESSAGE_KEY as the key.
///
/// \param fmt The format string to log.
/// \param ... The format arguments.
#define HE_MSG(fmt, ...) (he::LogKV{ HE_LOG_MESSAGE_KEY, (fmt), ##__VA_ARGS__ })

/// Base logging macro. Most users should not call this directly, but instead use the
/// level-specific logging macros.
///
/// \param lvl The level of the log.
/// \param cat The quoted string name of the category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG(lvl, catStr, ...) \
    do { \
        if constexpr (static_cast<int>(he::LogLevel::lvl) >= HE_LOG_LEVEL_ENABLED) { \
            static constexpr he::LogSource LogMsgSource_{ he::LogLevel::lvl, HE_LINE, HE_FILE, __FUNCTION__, catStr }; \
            const he::LogKV kvLogList_[]{ __VA_ARGS__ }; \
            he::Log(LogMsgSource_, kvLogList_, HE_LENGTH_OF(kvLogList_)); \
        } \
    } while(0)

/// Base string logging macro. Most users should not call this directly, but instead use the
/// level-specific logging macros such as \see HE_LOGF_INFO.
///
/// This macro is equivalent to calling \see HE_LOG with a single \see HE_MSG item.
///
/// \param lvl The level of the log.
/// \param cat The quoted string name of the log category.
/// \param fmt The format string for the message.
/// \param ... The format arguments.
#define HE_LOGF(lvl, catStr, fmt, ...) \
    HE_LOG(lvl, catStr, HE_MSG(fmt, ##__VA_ARGS__))

/// Logs a set of key-value pairs at the Trace log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_TRACE(cat, ...)  HE_LOG(Trace, #cat, __VA_ARGS__)

/// Logs a string message at the Trace log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOGF_TRACE(cat, ...) HE_LOGF(Trace, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Debug log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_DEBUG(cat, ...)  HE_LOG(Debug, #cat, __VA_ARGS__)

/// Logs a string message at the Debug log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOGF_DEBUG(cat, ...) HE_LOGF(Debug, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Info log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_INFO(cat, ...)   HE_LOG(Info, #cat, __VA_ARGS__)

/// Logs a string message at the Info log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOGF_INFO(cat, ...)  HE_LOGF(Info, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Warning log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_WARN(cat, ...)   HE_LOG(Warn, #cat, __VA_ARGS__)

/// Logs a string message at the Warning log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOGF_WARN(cat, ...)  HE_LOGF(Warn, #cat, __VA_ARGS__)

/// Logs a set of key-value pairs at the Error log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_ERROR(cat, ...)  HE_LOG(Error, #cat, __VA_ARGS__)

/// Logs a string message at the Error log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
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
    ///
    /// \param[in] x The log level to get the string representation of.
    /// \return A string representing the log level.
    const char* AsString(LogLevel x);

    /// Source of a log entry.
    /// This structure is not meant to be created directly. Use the logging macros instead.
    struct LogSource
    {
        LogLevel level;         ///< The level of the log entry
        uint32_t line;          ///< The line of the file the log comes from.
        const char* file;       ///< The name of the file the log comes from.
        const char* funcName;   ///< The name of the function the log comes from.
        const char* category;   ///< The name of the category the log comes from.
    };

    /// A key-value pair for a log entry. Values are stored in a union.
    /// Keys are not copied, only the pointer is stored.
    ///
    /// \note Prefer using the HE_KV and HE_MSG macros rather than creating this structure directly.
    struct LogKV
    {
        enum class ValueType
        {
            Bool,
            Int,
            Uint,
            Double,
            String,
        };

        LogKV(const char* k, bool v) : key(k), type(ValueType::Bool), value{ .b = v } {}
        LogKV(const char* k, signed char v) : key(k), type(ValueType::Int), value{ .i = v } {}
        LogKV(const char* k, signed short v) : key(k), type(ValueType::Int), value{ .i = v } {}
        LogKV(const char* k, signed int v) : key(k), type(ValueType::Int), value{ .i = v } {}
        LogKV(const char* k, signed long v) : key(k), type(ValueType::Int), value{ .i = v } {}
        LogKV(const char* k, signed long long v) : key(k), type(ValueType::Int), value{ .i = v } {}
        LogKV(const char* k, unsigned char v) : key(k), type(ValueType::Uint), value{ .u = v } {}
        LogKV(const char* k, unsigned short v) : key(k), type(ValueType::Uint), value{ .u = v } {}
        LogKV(const char* k, unsigned int v) : key(k), type(ValueType::Uint), value{ .u = v } {}
        LogKV(const char* k, unsigned long v) : key(k), type(ValueType::Uint), value{ .u = v } {}
        LogKV(const char* k, unsigned long long v) : key(k), type(ValueType::Uint), value{ .u = v } {}
        LogKV(const char* k, float v) : key(k), type(ValueType::Double), value{ .d = v } {}
        LogKV(const char* k, double v) : key(k), type(ValueType::Double), value{ .d = v } {}

        template <Enum T>
        constexpr LogKV(const char* k, T v) : LogKV(k, EnumType<T>(v)) {}

        LogKV(const char* k, const char* v)
            : key(k)
            , type(ValueType::String)
        {
            const uint32_t len = String::Length(v);
            value.s.append(v, v + len + 1); // +1 to include null terminator
        }

        template <typename... Args>
        LogKV(const char* k, fmt::format_string<Args...> fmt, Args&&... args)
            : key(k)
            , type(ValueType::String)
        {
            fmt::format_to(fmt::appender(value.s), fmt, Forward<Args>(args)...);
            value.s.push_back('\0');
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
                case ValueType::Bool: value.b = x.value.b; break;
                case ValueType::Int: value.i = x.value.i; break;
                case ValueType::Uint: value.u = x.value.u; break;
                case ValueType::Double: value.d = x.value.d; break;
                case ValueType::String: value.s = Move(x.value.s); break;
            }
            return *this;
        }

        bool GetBool() const;
        int64_t GetInt() const;
        uint64_t GetUint() const;
        double GetDouble() const;
        const char* GetString() const;

        const char* key;
        ValueType type;

        struct
        {
            union
            {
                bool b;
                int64_t i;
                uint64_t u;
                double d;
            };
            fmt::basic_memory_buffer<char, 128> s{};
        } value;
    };
    const char* AsString(LogKV::ValueType x);

    /// A pointer to a function that handles processing log entries.
    ///
    /// If your sink processes data on another thread, it must copy any parameters it
    /// wants to process later. The structures that are passed in are not garuanteed to
    /// live beyond the function body. However, you don't have to copy the strings in the
    /// LogSource, just the pointers since they point to static memory.
    ///
    /// \param[in] source The source information for this log entry.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    using LogSinkFunc = void(*)(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count);

    /// Stores the sink to be called when a log entry is dispatched.
    ///
    /// \param[in] sink The log sink to store and send logs to.
    /// \param[in] userData Pointer to opaque data that will be passed to the sink funciton as the
    ///     first parameter.
    void AddLogSink(LogSinkFunc sink, void* userData = nullptr);

    /// Stores the sink to be called when a log entry is dispatched. The sink object must have a
    /// lifetime that extends beyond its time added as a log sink. That is, you should not destroy
    /// the sink until you have called \ref RemoveLogSink.
    ///
    /// \note This overload is a helper for a common pattern of a class witha static Handler
    /// function that expects the instance as the first parameter.
    ///
    /// \param[in] sink The log sink to add.
    template <typename T>
    void AddLogSink(T& sink) { AddLogSink(&T::LogHandler, &sink); }

    /// Removes the stored sink so it is no longer invoked when a log entry is dispatched.
    ///
    /// \param[in] sink The log sink to remove.
    /// \param[in] userData The user data pointer that was originally passed into AddLogSink.
    void RemoveLogSink(LogSinkFunc sink, void* userData = nullptr);

    /// Removes the stored sink so it is no longer invoked when a log entry is dispatched.
    ///
    /// \note This overload is a helper for a common pattern of a class witha static Handler
    /// function that expects the instance as the first parameter.
    ///
    /// \param[in] sink The log sink to remove.
    template <typename T>
    void RemoveLogSink(T& sink) { RemoveLogSink(&T::LogHandler, &sink); }

    /// Entry point for handling a log entry.
    ///
    /// \note
    /// Prefer the use of the logging macros instead of calling this function directly.
    ///
    /// \param[in] source The source information for this log entry.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    void Log(const LogSource& source, const LogKV* kvs, uint32_t count);
}

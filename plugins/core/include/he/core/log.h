// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/config.h"
#include "he/core/delegate.h"
#include "he/core/key_value.h"
#include "he/core/macros.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

#define HE_LOG_LEVEL_TRACE  0   ///< Detailed tracing for a system, usually disabled unless tracking a bug.
#define HE_LOG_LEVEL_DEBUG  1   ///< Debug information useful for developers, usually disabled in non-internal builds.
#define HE_LOG_LEVEL_INFO   2   ///< Informational logging useful for general users, usually enabled.
#define HE_LOG_LEVEL_WARN   3   ///< Warning that something has gone wrong, but behavior should remain correct.
#define HE_LOG_LEVEL_ERROR  4   ///< Error notification that something has gone wrong, may experience strange behavior.

/// \def HE_LOG_ENABLE_LEVEL
/// Defines the log level enabled at compile time. Any level less than this defined value
/// is compiled out of the application. This defaults to Trace for internal builds and
/// Info for non-internal builds.
#if !defined(HE_LOG_ENABLE_LEVEL)
    #if HE_INTERNAL_BUILD
        #define HE_LOG_ENABLE_LEVEL HE_LOG_LEVEL_TRACE
    #else
        #define HE_LOG_ENABLE_LEVEL HE_LOG_LEVEL_INFO
    #endif
#endif

/// Base logging macro. Most users should not call this directly, but instead use the
/// level-specific logging macros.
///
/// \param lvl The level of the log.
/// \param catStr The quoted string name of the category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG(lvl, catStr, ...) \
    do { \
        if constexpr (static_cast<int>(::he::LogLevel::lvl) >= HE_LOG_ENABLE_LEVEL) { \
            constexpr ::he::LogSource LogEntrySource_{ ::he::LogLevel::lvl, HE_LINE, HE_FILE, HE_FUNC_SIG, catStr }; \
            const ::he::KeyValue logKvList_[]{ {"",0}, __VA_ARGS__ }; \
            ::he::Log(LogEntrySource_, logKvList_ + 1, HE_LENGTH_OF(logKvList_) - 1); \
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
/// \param fmt The format string. If there are no format arguments the string is used as-is,
///     without formatting.
/// \param ... The format arguments.
#define HE_LOGF_TRACE(cat, fmt, ...) HE_LOGF(Trace, #cat, fmt, __VA_ARGS__)

/// Logs a set of key-value pairs at the Debug log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_DEBUG(cat, ...)  HE_LOG(Debug, #cat, __VA_ARGS__)

/// Logs a string message at the Debug log level.
///
/// \param cat The unquoted name of the log category.
/// \param fmt The format string. If there are no format arguments the string is used as-is,
///     without formatting.
/// \param ... The format arguments.
#define HE_LOGF_DEBUG(cat, fmt, ...) HE_LOGF(Debug, #cat, fmt, __VA_ARGS__)

/// Logs a set of key-value pairs at the Info log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_INFO(cat, ...)   HE_LOG(Info, #cat, __VA_ARGS__)

/// Logs a string message at the Info log level.
///
/// \param cat The unquoted name of the log category.
/// \param fmt The format string. If there are no format arguments the string is used as-is,
///     without formatting.
/// \param ... The format arguments.
#define HE_LOGF_INFO(cat, fmt, ...)  HE_LOGF(Info, #cat, fmt, __VA_ARGS__)

/// Logs a set of key-value pairs at the Warning log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_WARN(cat, ...)   HE_LOG(Warn, #cat, __VA_ARGS__)

/// Logs a string message at the Warning log level.
///
/// \param cat The unquoted name of the log category.
/// \param fmt The format string. If there are no format arguments the string is used as-is,
///     without formatting.
/// \param ... The format arguments.
#define HE_LOGF_WARN(cat, fmt, ...)  HE_LOGF(Warn, #cat, fmt, __VA_ARGS__)

/// Logs a set of key-value pairs at the Error log level.
///
/// \param cat The unquoted name of the log category.
/// \param ... A series of \see HE_KV(k, v, ...) or \see HE_MSG(fmt, ...) calls.
#define HE_LOG_ERROR(cat, ...)  HE_LOG(Error, #cat, __VA_ARGS__)

/// Logs a string message at the Error log level.
///
/// \param cat The unquoted name of the log category.
/// \param fmt The format string. If there are no format arguments the string is used as-is,
///     without formatting.
/// \param ... The format arguments.
#define HE_LOGF_ERROR(cat, fmt, ...) HE_LOGF(Error, #cat, fmt, __VA_ARGS__)

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

    /// A delegate that handles processing log entries.
    ///
    /// If your sink processes data on another thread, it must copy any parameters it
    /// wants to process later. The structures that are passed in are not guaranteed to
    /// live beyond the function body. However, you don't have to copy the strings in the
    /// LogSource, just the pointers since they point to static memory.
    ///
    /// \param[in] userData Pointer to arbitrary user-defined data for this sink.
    /// \param[in] source The source information for this log entry.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    using LogDelegate = Delegate<void(const LogSource& source, const KeyValue* kvs, uint32_t count)>;

    /// Stores the sink to be called when a log entry is dispatched.
    ///
    /// \param[in] sink The log sink to store and send logs to.
    /// \param[in] userData Pointer to opaque data that will be passed to the sink function as the
    ///     first parameter.
    void AddLogSink(LogDelegate sink);

    /// Removes the stored sink so it is no longer invoked when a log entry is dispatched.
    ///
    /// \param[in] sink The log sink to remove.
    /// \param[in] userData The user data pointer that was originally passed into AddLogSink.
    void RemoveLogSink(LogDelegate sink);

    template <typename T>
    concept LogHandler = requires(T& t)
    {
        { t.OnLogEntry(DeclVal<const LogSource&>(), DeclVal<const KeyValue*>(), DeclVal<uint32_t>()) } -> ConvertibleTo<void>;
    };

    /// Stores the sink to be called when a log entry is dispatched. The sink object must have a
    /// lifetime that extends beyond its time added as a log sink. That is, you should not destroy
    /// the sink until you have called \ref RemoveLogSink.
    ///
    /// \note This overload is a helper for a common pattern of a class with a static Handler
    /// function that expects the instance as the first parameter.
    ///
    /// \param[in] sink The log sink to add.
    template <LogHandler T>
    void AddLogSink(T& sink) { AddLogSink(LogDelegate::Make<&T::OnLogEntry>(&sink)); }

    /// Removes the stored sink so it is no longer invoked when a log entry is dispatched.
    ///
    /// \note This overload is a helper for a common pattern of a class witha static Handler
    /// function that expects the instance as the first parameter.
    ///
    /// \param[in] sink The log sink to remove.
    template <LogHandler T>
    void RemoveLogSink(T& sink) { RemoveLogSink(LogDelegate::Make<&T::OnLogEntry>(&sink)); }

    /// Entry point for handling a log entry.
    ///
    /// \note
    /// Prefer the use of the logging macros instead of calling this function directly.
    ///
    /// \param[in] source The source information for this log entry.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    void Log(const LogSource& source, const KeyValue* kvs, uint32_t count);
}

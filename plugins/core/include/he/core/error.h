// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include "fmt/format.h"

namespace he
{
    /// Enum representing the type of an error. This simultaneously represents the origin of the
    /// error, as well as the severity.
    enum class ErrorType
    {
        Assert, ///< Fatal assertion, program cannot continue.
        Except, ///< Fatal uncaught exception, program cannot continue.
        Verify, ///< Non-fatal verification, there is a bug but the program can continue.
        Expect, ///< Non-fatal expectation, there is a failure in the tests.
    };

    /// A pointer to a function that is to handle application errors.
    ///
    /// \param[in] type The error type (assert or verify).
    /// \param[in] file The name of the file where the error was generated.
    /// \param[in] line The number of the line where the error was generated.
    /// \param[in] funcName The simple name of the function where the error was generated.
    /// \param[in] expression The expression that generated the error.
    /// \param[in] msg An optional user-defined message to accompany the error.
    /// \return True if the application should break in debugger, or false otherwise.
    using ErrorHandlerFunc = bool(*)(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg);

    /// Default handler for application errors. This function is used unless \ref SetErrorHandler
    /// is called with a different function. The default handling may use platform-specific APIs.
    ///
    /// \param[in] type The error type (assert or verify).
    /// \param[in] file The name of the file where the error was generated.
    /// \param[in] line The number of the line where the error was generated.
    /// \param[in] funcName The simple name of the function where the error was generated.
    /// \param[in] expression The expression that generated the error.
    /// \param[in] msg An optional user-defined message to accompany the error.
    /// \return True if the application should break in debugger, or false otherwise.
    bool DefaultErrorHandler(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg);

    /// Stores the handler to be called when an application error occurs.
    /// You can pass nullptr to indicate that \ref DefaultErrorHandler should be used.
    ///
    /// \param[in] handler The handler to call when an error occurs, or nullptr to indicate
    ///     that \ref DefaultErrorHandler should be used.
    void SetErrorHandler(ErrorHandlerFunc handler);

    /// Gets the current handler to be called when an application error occurs. This will never
    /// return nullptr, even in cases where \ref DefaultErrorHandler is used.
    ///
    /// \return Pointer to the currently assigned error handler function.
    ErrorHandlerFunc GetErrorHandler();

    /// Called to handle an application error. Not meant to be called directly.
    ///
    /// \internal
    /// \param[in] type The error type (assert or verify).
    /// \param[in] file The name of the file where the error was generated.
    /// \param[in] line The number of the line where the error was generated.
    /// \param[in] funcName The simple name of the function where the error was generated.
    /// \param[in] expression The expression that generated the error.
    /// \param[in] msg An optional user-defined message to accompany the error.
    /// \return True if the application should break in debugger, or false otherwise.
    bool HandleError(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg = "");

    /// Called to handle an application error and format a message. Not meant to be called directly.
    ///
    /// \internal
    /// \param[in] type The error type (assert or verify).
    /// \param[in] file The name of the file where the error was generated.
    /// \param[in] line The number of the line where the error was generated.
    /// \param[in] funcName The simple name of the function where the error was generated.
    /// \param[in] expression The expression that generated the error.
    /// \param[in] fmt A format specifier to build a message from.
    /// \param[in] args Format arguments for the specifier.
    /// \return True if the application should break in debugger, or false otherwise.
    template <typename... Args>
    bool HandleError(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, fmt::format_string<Args...> fmt, Args&&... args)
    {
        String buf(Allocator::GetTemp());
        fmt::format_to(Appender(buf), fmt, Forward<Args>(args)...);
        return HandleError(type, file, line, funcName, expression, buf.Data());
    }
}

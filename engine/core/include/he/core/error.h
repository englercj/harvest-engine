// Copyright Chad Engler

#pragma once

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

    /// Returns an error type as a string.
    const char* AsString(ErrorType x);

    /// A pointer to a function that is to handle application errors.
    ///
    /// \return True if the application should break in debugger, or false otherwise.
    using ErrorHandlerFunc = bool(*)(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg);

    /// Default handler for application errors. This function is used unless SetErrorHandler is
    /// called with a different function. The default handling is platform-specific.
    bool DefaultErrorHandler(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg);

    /// Stores the handler to be called when an application error occurs.
    void SetErrorHandler(ErrorHandlerFunc handler);

    /// Gets the current handler to be called when an application error occurs.
    ErrorHandlerFunc GetErrorHandler();

    /// Called to handle an application error. Not meant to be called directly.
    bool HandleError(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg = "");

    /// Called to handle an application error. Not meant to be called directly.
    template <typename... Args>
    bool HandleError(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, fmt::format_string<Args...> fmt, Args&&... args)
    {
        fmt::memory_buffer buf;
        fmt::format_to(fmt::appender(buf), fmt, Forward<Args>(args)...);
        buf.push_back('\0');
        return HandleError(type, file, line, funcName, expression, buf.data());
    }
}

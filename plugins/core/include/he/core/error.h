// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/appender.h"
#include "he/core/log.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/utils.h"

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

    /// Source of an error.
    /// This structure is not meant to be created directly. Use the assertion macros instead.
    struct ErrorSource
    {
        ErrorType type;         ///< The type of error.
        uint32_t line;          ///< The line of the file the error comes from.
        const char* file;       ///< The name of the file the error comes from.
        const char* funcName;   ///< The name of the function the error comes from.
        const char* expression; ///< The expression that triggered the error.
    };

    /// A pointer to a function that is to handle application errors.
    ///
    /// \param[in] userData Pointer to arbitrary user-defined data for this handler.
    /// \param[in] source The source information for this error.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    /// \return True if the application should break in debugger, or false otherwise.
    using ErrorHandlerFunc = bool(*)(void* userData, const ErrorSource& source, const LogKV* kvs, uint32_t count);

    /// Default handler for application errors. This function is used unless \ref SetErrorHandler
    /// is called with a different function. The default handling may use platform-specific APIs.
    ///
    /// \param[in] userData Pointer to arbitrary user-defined data for this handler.
    /// \param[in] source The source information for this error.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    /// \return True if the application should break in debugger, or false otherwise.
    bool DefaultErrorHandler(void* userData, const ErrorSource& source, const LogKV* kvs, uint32_t count);

    /// Stores the handler to be called when an application error occurs.
    /// You can pass nullptr to indicate that \ref DefaultErrorHandler should be used.
    ///
    /// \param[in] handler The handler to call when an error occurs, or nullptr to indicate
    ///     that \ref DefaultErrorHandler should be used.
    /// \param[in] userData Pointer to arbitrary user-defined data to be passed to the handler.
    void SetErrorHandler(ErrorHandlerFunc handler, void* userData = nullptr);

    /// Gets the current handler to be called when an application error occurs. This will never
    /// return nullptr, even in cases where \ref DefaultErrorHandler is used.
    ///
    /// \return Pointer to the currently assigned error handler function.
    ErrorHandlerFunc GetErrorHandler();

    /// Called to handle an application error. Not meant to be called directly.
    ///
    /// \internal
    /// \param[in] source The source information for this error.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    /// \return True if the application should break in debugger, or false otherwise.
    bool HandleError(const ErrorSource& source, const LogKV* kvs, uint32_t count);

    /// Helper to change the error handler function within a scope.
    class ScopedErrorHandler
    {
    public:
        ScopedErrorHandler(ErrorHandlerFunc handler)
        {
            m_old = GetErrorHandler();
            SetErrorHandler(handler);
        }

        ~ScopedErrorHandler()
        {
            SetErrorHandler(m_old);
        }

    private:
        ErrorHandlerFunc m_old;
    };
}

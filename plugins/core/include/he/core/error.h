// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/debug.h"
#include "he/core/key_value.h"
#include "he/core/macros.h"
#include "he/core/types.h"

#define HE_ERROR_IF(kind, expr, ...) \
    (HE_LIKELY(expr) || ([&](const char* errorFuncName_) -> bool { \
        const ::he::ErrorSource errorSource_{ ::he::ErrorKind::kind, HE_LINE, HE_FILE, errorFuncName_ }; \
        const ::he::KeyValue errorKvList_[]{ HE_KV(error_kind, ::he::ErrorKind::kind), HE_KV(error_expr, #expr), __VA_ARGS__ }; \
        return ::he::HandleError(errorSource_, errorKvList_, HE_LENGTH_OF(errorKvList_)); \
    }(HE_FUNC_SIG) && HE_DEBUG_BREAK()))

namespace he
{
    /// Enum representing the type of an error. This simultaneously represents the origin of the
    /// error, as well as the severity.
    enum class ErrorKind : uint8_t
    {
        Assert, ///< Fatal assertion, program cannot continue.
        Except, ///< Fatal uncaught exception, program cannot continue.
        Expect, ///< Non-fatal expectation, there is a failure in the tests.
        Verify, ///< Non-fatal verification, there is a bug but the program can continue.
    };

    /// Source of an error.
    /// This structure is not meant to be created directly. Use the assertion macros instead.
    struct ErrorSource
    {
        ErrorKind kind;         ///< The kind of error has occurred.
        uint32_t line;          ///< The line of the file the error comes from.
        const char* file;       ///< The name of the file the error comes from.
        const char* funcName;   ///< The name of the function the error comes from.
    };

    /// A pointer to a function that is to handle application errors.
    ///
    /// \param[in] userData Pointer to arbitrary user-defined data for this handler.
    /// \param[in] source The source information for this error.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    /// \return True if the application should break in debugger, or false otherwise.
    using Pfn_ErrorHandler = bool(*)(void* userData, const ErrorSource& source, const KeyValue* kvs, uint32_t count);

    /// Default handler for application errors. This function is used unless \ref SetErrorHandler
    /// is called with a different function. The default handling may use platform-specific APIs.
    ///
    /// \param[in] userData Pointer to arbitrary user-defined data for this handler.
    /// \param[in] source The source information for this error.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    /// \return True if the application should break in debugger, or false otherwise.
    bool DefaultErrorHandler(void* userData, const ErrorSource& source, const KeyValue* kvs, uint32_t count);

    /// Stores the handler to be called when an application error occurs.
    /// You can pass nullptr to indicate that \ref DefaultErrorHandler should be used.
    ///
    /// \param[in] handler The handler to call when an error occurs, or nullptr to indicate
    ///     that \ref DefaultErrorHandler should be used.
    /// \param[in] userData Pointer to arbitrary user-defined data to be passed to the handler.
    void SetErrorHandler(Pfn_ErrorHandler handler, void* userData = nullptr);

    /// Gets the current handler to be called when an application error occurs. This will never
    /// return nullptr, even in cases where \ref DefaultErrorHandler is used.
    ///
    /// \param[out] userData The user data that was registered for the error handler.
    /// \return Pointer to the currently assigned error handler function.
    Pfn_ErrorHandler GetErrorHandler(void*& userData);

    /// Called to handle an application error. Not meant to be called directly.
    ///
    /// \internal
    /// \param[in] source The source information for this error.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    /// \return True if the application should break in debugger, or false otherwise.
    bool HandleError(const ErrorSource& source, const KeyValue* kvs, uint32_t count);

    /// Helper to change the error handler function within a scope.
    class ScopedErrorHandler
    {
    public:
        ScopedErrorHandler(Pfn_ErrorHandler handler, void* userData = nullptr)
        {
            m_old = GetErrorHandler(m_oldUser);
            SetErrorHandler(handler, userData);
        }

        ~ScopedErrorHandler()
        {
            SetErrorHandler(m_old, m_oldUser);
        }

    private:
        Pfn_ErrorHandler m_old;
        void* m_oldUser;
    };
}

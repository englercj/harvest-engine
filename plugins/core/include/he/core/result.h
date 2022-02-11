// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/config.h"
#include "he/core/debug.h"
#include "he/core/error.h"
#include "he/core/macros.h"
#include "he/core/string.h"
#include "he/core/types.h"

#if defined(_PREFAST_)
    // Prevent code analysis from reporting errors already checked by assertions
    #define HE_ASSERT_RESULT(r) __assume(!!(r))
    #define HE_VERIFY_RESULT(r) (!!(r))
#elif HE_ENABLE_ASSERTIONS
    #define HE_ASSERT_RESULT(r) (void)(HE_LIKELY(!!(r)) || (he::ExpectResultFailed(he::ErrorType::Assert, HE_FILE, HE_LINE, __FUNCTION__, #r, r) && HE_DEBUG_BREAK()))
    #define HE_VERIFY_RESULT(r) (HE_LIKELY(!!(r)) || (he::ExpectResultFailed(he::ErrorType::Verify, HE_FILE, HE_LINE, __FUNCTION__, #r, r) && HE_DEBUG_BREAK()))
#else
    #define HE_ASSERT_RESULT(r) HE_UNUSED(r)
    #define HE_VERIFY_RESULT(r) (!!(r))
#endif

namespace he
{
    /// Class for dealing with system results. It stores the raw error code from the OS and
    /// provides mechanisms for converting it to a string. The strings are from the OS and
    /// are therefore not garuanteed to be the same across platforms.
    class Result
    {
    public:
        /// The operation completed successfully.
        static Result Success;

        /// The operation cannot be completed because a parameter was invalid.
        static Result InvalidParameter;

        /// The operation cannot be completed because it is not supported on this platform.
        static Result NotSupported;

    public:
        /// Create a Result from the last system error code. For example, from `errno` on posix.
        static Result FromLastError();

    public:
        /// Constructs a default Result, which is the same as \ref Success.
        Result() = default;

        /// Constructs a Result from a system error code. Casting may be necessary to call this
        /// constructor. Platform-specific helper functions exist to make this easier. See
        /// \ref Win32Result and \ref PosixResult.
        ///
        /// \param[in] code The system error code to construct from.
        Result(uint32_t code) : m_code(code) {}

        /// Returns the system code this Result represents.
        ///
        /// \return The stored system code.
        uint32_t GetCode() const { return m_code; }

        /// Returns true if the Result is a successful one. That is, the code is zero.
        ///
        /// \return True if representing success, false otherwise.
        bool IsOk() const { return m_code == 0; }

        /// Looks up the error string for the system code using platform-specific APIs. The
        /// current system locale is used for the lookup.
        String ToString(Allocator& allocator = Allocator::GetDefault()) const;

        /// Converts the Result to a boolean. This value is the same as \ref IsOK().
        explicit operator bool() const { return IsOk(); }

        /// Checks if two Result store the same system code.
        ///
        /// \return True if the codes are the same, false otherwise.
        bool operator==(const Result& x) const { return m_code == x.m_code; }

        /// Checks if two Result store different system codes.
        ///
        /// \return True if the codes are different, false otherwise.
        bool operator!=(const Result& x) const { return m_code != x.m_code; }

    private:
        uint32_t m_code{ 0 };
    };

    /// Helper to create a Result from a win32 error code (not an HRESULT).
    ///
    /// \param[in] err The Win32 error code.
    /// \return A Result storing the Win32 error code.
    inline Result Win32Result(unsigned long err) { return Result(static_cast<uint32_t>(err)); }

    /// Helper to create a Result from a posix error code.
    ///
    /// \param[in] err The posix error code.
    /// \return A Result storing the posix error code.
    inline Result PosixResult(int err) { return Result(static_cast<uint32_t>(err)); }

    /// Internal function used to handle failures from the \ref HE_ASSERT_RESULT and
    /// \ref HE_VERIFY_RESULT macros.
    ///
    /// \internal
    /// \param[in] type The error type (assert or verify).
    /// \param[in] file The name of the file where the error was generated.
    /// \param[in] line The number of the line where the error was generated.
    /// \param[in] funcName The simple name of the function where the error was generated.
    /// \param[in] expression The expression that generated the error.
    /// \param[in] result The result of the expression.
    /// \return Returns true if the debugger should break at the erroneous line.
    inline bool ExpectResultFailed(ErrorType type, const char* file, const int line, const char* funcName, const char* expression, Result result)
    {
        String msg = result.ToString(CrtAllocator::Get());
        return HandleError(type, file, line, funcName, expression, msg.Data());
    }
}

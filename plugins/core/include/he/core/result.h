// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    class String;

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
        ///
        /// \param[out] out The string to format the error message into.
        void ToString(String& out) const;

        /// Converts the Result to a boolean. This value is the same as \ref IsOK().
        [[nodiscard]] explicit operator bool() const { return IsOk(); }

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
}

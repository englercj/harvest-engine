// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    /// Attempts to get random bytes from hardware, and if that fails falls back to getting
    /// them from the system.
    ///
    /// \param dst The buffer to write the result to. Expected to be at least `count` bytes large.
    /// \param count The number of bytes to get.
    /// \return True if bytes were successfully retrieved, false otherwise.
    bool GetSecureRandomBytes(uint8_t* dst, size_t count);

    /// Attempts to get random bytes from hardware, and if that fails falls back to getting
    /// them from the system.
    ///
    /// \param dst The fixed size buffer to write the result to.
    /// \return True if bytes were successfully retrieved, false otherwise.
    template <size_t N>
    bool GetSecureRandomBytes(uint8_t (&dst)[N]) { return GetSecureRandomBytes(dst, N); }

    /// Attempts to get random bytes from hardware. This uses rdrand for x86 and reads the rndr
    /// register for ARM. On some systems these may be emulated by the OS.
    ///
    /// \param dst The buffer to write the result to. Expected to be at least `count` bytes large.
    /// \param count The number of bytes to get.
    /// \return True if bytes were successfully retrieved, false otherwise.
    bool GetHardwareRandomBytes(uint8_t* dst, size_t count);

    /// Attempts to get random bytes from hardware. This uses rdrand for x86 and reads the rndr
    /// register for ARM. On some systems these may be emulated by the OS.
    ///
    /// \param dst The fixed size buffer to write the result to.
    /// \return True if bytes were successfully retrieved, false otherwise.
    template <size_t N>
    bool GetHardwareRandomBytes(uint8_t (&dst)[N]) { return GetHardwareRandomBytes(dst, N); }

    /// Attempts to get random bytes from the system. This uses BCrypt on win32 and /dev/urandom
    /// on posix.
    ///
    /// \param dst The buffer to write the result to. Expected to be at least `count` bytes large.
    /// \param count The number of bytes to get.
    /// \return True if bytes were successfully retrieved, false otherwise.
    bool GetSystemRandomBytes(uint8_t* dst, size_t count);

    /// Attempts to get random bytes from the system. This uses BCrypt on win32 and /dev/urandom
    /// on posix.
    ///
    /// \param dst The fixed size buffer to write the result to.
    /// \return True if bytes were successfully retrieved, false otherwise.
    template <size_t N>
    bool GetSystemRandomBytes(uint8_t (&dst)[N]) { return GetSystemRandomBytes(dst, N); }
}

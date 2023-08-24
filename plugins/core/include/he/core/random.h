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

    /// A fast, deterministic, uniformly distributed, pseudo random number generator.
    class Random64
    {
    public:
        /// Construct the random state from a random seed.
        ///
        /// This will attempt to use secure random to fill the seed if available, and falls back
        /// to using the Monotonic clock if that fails.
        Random64() noexcept;

        /// Construct the random state with a predetermined seed.
        ///
        /// \param[in] seed The seed to use for the random number generator.
        explicit Random64(uint64_t seed) noexcept;

        /// Generate a random unsigned integer value in the range [0, UINT64_MAX]
        ///
        /// \return The random value.
        uint64_t Next();

        /// Generate a random unsigned integer value in the range [min, max)
        ///
        /// \param[in] min The minimum (inclusive) value of the value range.
        /// \param[in] max The maximum (exclusive) value of the value range.
        /// \return The random value.
        uint64_t Next(uint64_t min, uint64_t max);

        /// Generate a random floating point value in the range [0, 1)
        ///
        /// \return The random value.
        double Real();

        /// Generate a random floating point value in the range [min, max)
        ///
        /// \param[in] min The minimum (inclusive) value of the value range.
        /// \param[in] max The maximum (exclusive) value of the value range.
        /// \return The random value.
        double Real(double min, double max);

        /// Generate a random value from an approximate Gaussian distribution with mean=0 and std=1
        ///
        /// \return The random value.
        double Gauss();

        /// Generate a random value from an approximate Gaussian distribution
        ///
        /// \param[in] mean The mean of the distribution.
        /// \param[in] std The standard deviation of the distribution.
        /// \return The random value.
        double Gauss(double mean, double std);

        /// Generate a stream of `count` random bytes and store them in `data`.
        ///
        /// \param[out] dst The destination buffer to write the random bytes into.
        /// \param[in] count The number of bytes to generate. The `dst` buffer is expected to be
        ///     large enough to hold this many bytes.
        void Bytes(uint8_t* dst, uint32_t count);

        /// \copydoc Bytes
        template <uint32_t N>
        void Bytes(uint8_t (&dst)[N]) { Bytes(dst, N); }

        /// The current state value used for the pseudo random generation.
        ///
        /// If this value is used to seed another Random64 generator, then it will generate the
        /// same sequence of values that this generator will.
        uint64_t m_state;
    };
}

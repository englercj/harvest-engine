// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    class Allocator;

    /// A 128-bit RFC 4122 UUID. The octets are defined by that RFC to be:
    ///
    ///   0                   1                   2                   3
    ///    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   |                          time_low                             |
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   |       time_mid                |         time_hi_and_version   |
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   |                         node (2-5)                            |
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///
    /// For version 3 or 5 the timestamp, clock sequence, and node values are
    /// constructed from a name as described in Section 4.3.
    ///
    /// For version 4 the timestamp, clock sequence, and node values are randomly
    /// generated as described in Section 4.4.
    ///
    class Uuid
    {
    public:
        /// Create a Uuid from a string in the form: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX, where
        /// each 'X' is a hex digit and the dashes are optional.
        ///
        /// \param[in] src The source string to parse into a UUID.
        /// \return The newly constructed UUID. If `src` is not valid, then \see UuidZero is returned.
        static Uuid FromString(StringView src);

        /// Creates a version 4 (random) UUID.
        ///
        /// \return The newly constructed UUID.
        static Uuid CreateV4();

    public:
        /// Gets the version number of a Uuid. This is usually a value in the range [0, 5].
        ///
        /// \return The version number of the UUID.
        constexpr uint8_t GetVersion() const { return (m_bytes[6] & 0xf0) >> 4; }

        /// Gets the first 8 bytes of a Uuid as a uint64.
        /// This can be useful for interop with third-party systems.
        ///
        /// \return The first 8 bytes of the Uuid as a uint64.
        uint64_t GetLow() const;

        /// Gets the last 8 bytes of a Uuid as a uint64.
        /// This can be useful for interop with third-party systems.
        ///
        /// \return The last 8 bytes of the Uuid as a uint64.
        uint64_t GetHigh() const;

        /// Creates a cannonical string representation of the UUID.
        ///
        /// \param[in] allocator The allocator to use to construct the string.
        /// \return The cannonical UUID string.
        String ToString(Allocator& allocator = Allocator::GetDefault()) const;

        /// Checks if two UUIDs are the same.
        ///
        /// \return True if the UUIDs are the same, false otherwise.
        bool operator==(const Uuid& x) const;

        /// Checks if two UUIDs are different.
        ///
        /// \return True if the UUIDs are different, false otherwise.
        bool operator!=(const Uuid& x) const;

        /// Checks if the UUID `x` is less than `y`.Not semantically useful but can be functionally
        /// useful for storing UUIDs in ordered containers (such as map/set).
        ///
        /// \return True if `x` is stricly less than `y`.
        bool operator<(const Uuid& x) const;

    public:
        uint8_t m_bytes[16];
    };

    /// A UUID of all zeroes. Sometimes called the nil, zero, or empty UUID.
    /// 00000000-0000-0000-0000-000000000000
    inline constexpr Uuid Uuid_Zero{ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };
}

// Hash overloads
namespace std
{
    template <typename> struct hash;

    template <>
    struct hash<he::Uuid>
    {
        size_t operator()(const he::Uuid& value) const;
    };
}

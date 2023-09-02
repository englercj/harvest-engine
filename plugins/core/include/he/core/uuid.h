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

        /// Creates a version 3 (MD5) UUID from a name and namespace ID.
        ///
        /// \param[in] name The name to use.
        /// \param[in] nsid The UUID of the namespace to use.
        /// \return The newly constructed UUID.
        static Uuid CreateV3(StringView name, const Uuid& nsid);

        /// Creates a version 4 (random) UUID.
        ///
        /// \return The newly constructed UUID.
        static Uuid CreateV4();

        /// Creates a version 5 (SHA-1) UUID from a name and namespace ID.
        ///
        /// \param[in] name The name to use.
        /// \param[in] nsid The UUID of the namespace to use.
        /// \return The newly constructed UUID.
        static Uuid CreateV5(StringView name, const Uuid& nsid);

        /// Returns a non-cryptographic hash of the uuid.
        /// Since uuids are already either random or hashed bits this simply returns a well
        /// defined slice of the uuid's bytes.
        ///
        /// \return The hash value.
        [[nodiscard]] uint64_t HashCode() const noexcept;

    public:
        /// Gets the version number of a Uuid. This is usually a value in the range [0, 5].
        ///
        /// \return The version number of the UUID.
        constexpr uint8_t Version() const { return (m_bytes[6] & 0xf0) >> 4; }

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

    /// Name string is a fully-qualified domain name.
    /// 6ba7b810-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceDNS{ { 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

    /// Name string is a URL.
    /// 6ba7b811-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceURL{ { 0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

    /// Name string is an ISO OID.
    /// 6ba7b812-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceOID{ { 0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

    /// Name string is an X.500 DN (in DER or a text output format).
    /// 6ba7b814-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceX500{ { 0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };
}

// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
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
        /// The number of bytes in a UUID.
        static constexpr uint32_t Size = 16;

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

    public:
        /// Construct an empty (all zeroes) UUID.
        ///
        /// \note A default constructed UUID is not a valud RFC 4122 UUID. To create a new valid UUID
        ///     call one of the static \ref CreateV3, \ref CreateV4, or \ref CreateV5 functions.
        constexpr Uuid() noexcept : m_bytes() {}

        /// Construct a Uuid from a series of byte values.
        ///
        /// \param[in] b0 The first byte of the UUID.
        /// \param[in] b1 The second byte of the UUID.
        /// \param[in] b2 The third byte of the UUID.
        /// \param[in] b3 The fourth byte of the UUID.
        /// \param[in] b4 The fifth byte of the UUID.
        /// \param[in] b5 The sixth byte of the UUID.
        /// \param[in] b6 The seventh byte of the UUID.
        /// \param[in] b7 The eighth byte of the UUID.
        /// \param[in] b8 The ninth byte of the UUID.
        /// \param[in] b9 The tenth byte of the UUID.
        /// \param[in] b10 The eleventh byte of the UUID.
        /// \param[in] b11 The twelfth byte of the UUID.
        /// \param[in] b12 The thirteenth byte of the UUID.
        /// \param[in] b13 The fourteenth byte of the UUID.
        /// \param[in] b14 The fifteenth byte of the UUID.
        /// \param[in] b15 The sixteenth byte of the UUID.
        constexpr Uuid(
            uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7,
            uint8_t b8, uint8_t b9, uint8_t b10, uint8_t b11, uint8_t b12, uint8_t b13, uint8_t b14, uint8_t b15) noexcept
            : m_bytes{ b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 }
        {}

        /// Construct a Uuid from an array of \ref Size bytes.
        ///
        /// \param[in] bytes The array of bytes to copy into the UUID. Must be at least
        ///     \ref Size bytes in length.
        Uuid(const uint8_t bytes[Size]) noexcept;

        /// Returns a non-cryptographic hash of the uuid.
        /// Since uuids are already either random or hashed bits this simply returns a well
        /// defined slice of the uuid's bytes.
        ///
        /// \return The hash value.
        [[nodiscard]] uint64_t HashCode() const noexcept;

        /// Gets the version number of a Uuid. This is usually a value in the range [0, 5].
        ///
        /// \return The version number of the UUID.
        [[nodiscard]] constexpr uint8_t Version() const { return (m_bytes[6] & 0xf0) >> 4; }

        /// Checks if two UUIDs are the same.
        ///
        /// \return True if the UUIDs are the same, false otherwise.
        [[nodiscard]] bool operator==(const Uuid& x) const;

        /// Checks if two UUIDs are different.
        ///
        /// \return True if the UUIDs are different, false otherwise.
        [[nodiscard]] bool operator!=(const Uuid& x) const;

        /// Checks if the UUID `x` is less than `y`.Not semantically useful but can be functionally
        /// useful for storing UUIDs in ordered containers (such as map/set).
        ///
        /// \return True if `x` is stricly less than `y`.
        [[nodiscard]] bool operator<(const Uuid& x) const;

    public:
        uint8_t m_bytes[Size];
    };

    /// A UUID of all zeroes. Sometimes called the nil, zero, or empty UUID.
    /// 00000000-0000-0000-0000-000000000000
    inline constexpr Uuid Uuid_Zero{};

    /// Name string is a fully-qualified domain name.
    /// 6ba7b810-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceDNS{ 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };

    /// Name string is a URL.
    /// 6ba7b811-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceURL{ 0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };

    /// Name string is an ISO OID.
    /// 6ba7b812-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceOID{ 0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };

    /// Name string is an X.500 DN (in DER or a text output format).
    /// 6ba7b814-9dad-11d1-80b4-00c04fd430c8
    inline constexpr Uuid Uuid_NamespaceX500{ 0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };
}

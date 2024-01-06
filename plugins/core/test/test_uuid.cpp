// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/uuid.h"
#include "he/core/uuid_fmt.h"

#include "he/core/fmt.h"
#include "he/core/hash.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, FromString)
{
    // Dashed format, lower-case
    HE_EXPECT_EQ(Uuid::FromString("00000000-0000-0000-0000-000000000000"), Uuid_Zero);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b810-9dad-11d1-80b4-00c04fd430c8"), Uuid_NamespaceDNS);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b811-9dad-11d1-80b4-00c04fd430c8"), Uuid_NamespaceURL);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b812-9dad-11d1-80b4-00c04fd430c8"), Uuid_NamespaceOID);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b814-9dad-11d1-80b4-00c04fd430c8"), Uuid_NamespaceX500);

    // No dashes format, lower-case
    HE_EXPECT_EQ(Uuid::FromString("00000000000000000000000000000000"), Uuid_Zero);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b8109dad11d180b400c04fd430c8"), Uuid_NamespaceDNS);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b8119dad11d180b400c04fd430c8"), Uuid_NamespaceURL);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b8129dad11d180b400c04fd430c8"), Uuid_NamespaceOID);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b8149dad11d180b400c04fd430c8"), Uuid_NamespaceX500);

    // Dashed format, upper-case
    HE_EXPECT_EQ(Uuid::FromString("6BA7B810-9DAD-11D1-80B4-00C04FD430C8"), Uuid_NamespaceDNS);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B811-9DAD-11D1-80B4-00C04FD430C8"), Uuid_NamespaceURL);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B812-9DAD-11D1-80B4-00C04FD430C8"), Uuid_NamespaceOID);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B814-9DAD-11D1-80B4-00C04FD430C8"), Uuid_NamespaceX500);

    // No dashes format, upper-case
    HE_EXPECT_EQ(Uuid::FromString("6BA7B8109DAD11D180B400C04FD430C8"), Uuid_NamespaceDNS);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B8119DAD11D180B400C04FD430C8"), Uuid_NamespaceURL);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B8129DAD11D180B400C04FD430C8"), Uuid_NamespaceOID);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B8149DAD11D180B400C04FD430C8"), Uuid_NamespaceX500);

    // All mixed up
    HE_EXPECT_EQ(Uuid::FromString("6ba7b8109Dad11D1-80B4-00c04fd430c8"), Uuid_NamespaceDNS);
    HE_EXPECT_EQ(Uuid::FromString("6BA7B811-9dad11d180b4-00C04FD430C8"), Uuid_NamespaceURL);
    HE_EXPECT_EQ(Uuid::FromString("6Ba7B812---9DaD11d1------80b400C04FD430c8"), Uuid_NamespaceOID);
    HE_EXPECT_EQ(Uuid::FromString("00000000-0000-0000-0000-000000000000ABCDEF"), Uuid_Zero);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b810-9dad-11d1-80b4-00c04fd430c8-987654321"), Uuid_NamespaceDNS);

    // Invalid
    HE_EXPECT_EQ(Uuid::FromString("6ba7b810-9dad-11d1-80b4-00c04fd430c"), Uuid_Zero);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b810"), Uuid_Zero);
    HE_EXPECT_EQ(Uuid::FromString("6ba7b810-9dad-11d1-80b4-"), Uuid_Zero);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, CreateV3)
{
    {
        const Uuid uuid = Uuid::CreateV3("hello.example.com", Uuid_NamespaceDNS);
        HE_EXPECT_EQ(uuid.Version(), 3);
        HE_EXPECT_EQ(Format("{}", uuid), "9125a8dc-52ee-365b-a5aa-81b0b3681cf6");
    }

    {
        const Uuid uuid = Uuid::CreateV3("http://example.com/hello", Uuid_NamespaceURL);
        HE_EXPECT_EQ(uuid.Version(), 3);
        HE_EXPECT_EQ(Format("{}", uuid), "c6235813-3ba4-3801-ae84-e0a6ebb7d138");
    }

    {
        const Uuid customNamespace{ 0x0f, 0x5a, 0xbc, 0xd1, 0xc1, 0x94, 0x47, 0xf3, 0x90, 0x5b, 0x2d, 0xf7, 0x26, 0x3a, 0x08, 0x4b };
        const Uuid uuid = Uuid::CreateV3("hello", customNamespace);
        HE_EXPECT_EQ(uuid.Version(), 3);
        HE_EXPECT_EQ(Format("{}", uuid), "a981a0c2-68b1-35dc-bcfc-296e52ab01ec");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, CreateV4)
{
    Uuid uuid = Uuid::CreateV4();

    HE_EXPECT_NE(uuid, Uuid_Zero);
    HE_EXPECT_EQ(uuid.Version(), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, CreateV5)
{
    {
        const Uuid uuid = Uuid::CreateV5("hello.example.com", Uuid_NamespaceDNS);
        HE_EXPECT_EQ(uuid.Version(), 5);
        HE_EXPECT_EQ(Format("{}", uuid), "fdda765f-fc57-5604-a269-52a7df8164ec");
    }

    {
        const Uuid uuid = Uuid::CreateV5("http://example.com/hello", Uuid_NamespaceURL);
        HE_EXPECT_EQ(uuid.Version(), 5);
        HE_EXPECT_EQ(Format("{}", uuid), "3bbcee75-cecc-5b56-8031-b6641c1ed1f1");
    }

    {
        const Uuid customNamespace{ 0x0f, 0x5a, 0xbc, 0xd1, 0xc1, 0x94, 0x47, 0xf3, 0x90, 0x5b, 0x2d, 0xf7, 0x26, 0x3a, 0x08, 0x4b };
        const Uuid uuid = Uuid::CreateV5("hello", customNamespace);
        HE_EXPECT_EQ(uuid.Version(), 5);
        HE_EXPECT_EQ(Format("{}", uuid), "90123e1c-7512-523e-bb28-76fab9f2f73d");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Construct)
{
    // Default constructs to all zeroes
    {
        const uint8_t zeroes[16]{};
        static_assert(sizeof(Uuid::m_bytes) == sizeof(zeroes));

        const Uuid uuid;
        HE_EXPECT_EQ(uuid.Version(), 0);
        HE_EXPECT_EQ_MEM(uuid.m_bytes, zeroes, sizeof(zeroes));
    }

    // Default constructor is constexpr
    {
        constexpr Uuid uuid;
        static_assert(uuid.Version() == 0);
        static_assert(uuid.m_bytes[0] == 0 && uuid.m_bytes[1] == 0 && uuid.m_bytes[2] == 0 && uuid.m_bytes[3] == 0);
        static_assert(uuid.m_bytes[4] == 0 && uuid.m_bytes[5] == 0 && uuid.m_bytes[6] == 0 && uuid.m_bytes[7] == 0);
        static_assert(uuid.m_bytes[8] == 0 && uuid.m_bytes[9] == 0 && uuid.m_bytes[10] == 0 && uuid.m_bytes[11] == 0);
        static_assert(uuid.m_bytes[12] == 0 && uuid.m_bytes[13] == 0 && uuid.m_bytes[14] == 0 && uuid.m_bytes[15] == 0);
    }

    // Constructs with individual byte values
    {
        const uint8_t iota[16]{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
        static_assert(sizeof(Uuid::m_bytes) == sizeof(iota));

        const Uuid uuid(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        HE_EXPECT_EQ(uuid.Version(), 0);
        HE_EXPECT_EQ_MEM(uuid.m_bytes, iota, sizeof(iota));
    }

    // Individual bytes constructor is constexpr
    {
        constexpr Uuid uuid(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        static_assert(uuid.Version() == 0);
        static_assert(uuid.m_bytes[0] == 1 && uuid.m_bytes[1] == 2 && uuid.m_bytes[2] == 3 && uuid.m_bytes[3] == 4);
        static_assert(uuid.m_bytes[4] == 5 && uuid.m_bytes[5] == 6 && uuid.m_bytes[6] == 7 && uuid.m_bytes[7] == 8);
        static_assert(uuid.m_bytes[8] == 9 && uuid.m_bytes[9] == 10 && uuid.m_bytes[10] == 11 && uuid.m_bytes[11] == 12);
        static_assert(uuid.m_bytes[12] == 13 && uuid.m_bytes[13] == 14 && uuid.m_bytes[14] == 15 && uuid.m_bytes[15] == 16);
    }

    // Constructs from a byte array
    {
        const uint8_t iota[16]{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
        static_assert(sizeof(Uuid::m_bytes) == sizeof(iota));

        const Uuid uuid(iota);
        HE_EXPECT_EQ(uuid.Version(), 0);
        HE_EXPECT_EQ_MEM(uuid.m_bytes, iota, sizeof(iota));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Version)
{
    HE_EXPECT_EQ(Uuid_Zero.Version(), 0);
    HE_EXPECT_EQ(Uuid_NamespaceDNS.Version(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceURL.Version(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceOID.Version(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceX500.Version(), 1);

    for (uint32_t i = 0; i < 32; ++i)
    {
        const Uuid id = Uuid::CreateV4();
        HE_EXPECT_EQ(id.Version(), 4);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Operators)
{
    constexpr Uuid UuidOne{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    HE_EXPECT(Uuid_Zero == Uuid_Zero);
    HE_EXPECT(UuidOne == UuidOne);
    HE_EXPECT(!(UuidOne == Uuid_Zero));
    HE_EXPECT(!(Uuid_Zero == UuidOne));

    HE_EXPECT(UuidOne != Uuid_Zero);
    HE_EXPECT(Uuid_Zero != UuidOne);
    HE_EXPECT(!(Uuid_Zero != Uuid_Zero));
    HE_EXPECT(!(UuidOne != UuidOne));

    HE_EXPECT(Uuid_Zero < UuidOne);
    HE_EXPECT(!(UuidOne < Uuid_Zero));
    HE_EXPECT(!(Uuid_Zero < Uuid_Zero));
    HE_EXPECT(!(UuidOne < UuidOne));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Constants)
{
    static_assert(Uuid_Zero.Version() == 0);
    static_assert(Uuid_NamespaceDNS.Version() == 1);
    static_assert(Uuid_NamespaceURL.Version() == 1);
    static_assert(Uuid_NamespaceOID.Version() == 1);
    static_assert(Uuid_NamespaceX500.Version() == 1);

    uint8_t zero[16]{};
    HE_EXPECT_EQ_MEM(Uuid_Zero.m_bytes, zero, 16);

    uint8_t dns[16]{ 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };
    HE_EXPECT_EQ_MEM(Uuid_NamespaceDNS.m_bytes, dns, 16);

    uint8_t url[16]{ 0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };
    HE_EXPECT_EQ_MEM(Uuid_NamespaceURL.m_bytes, url, 16);

    uint8_t oid[16]{ 0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };
    HE_EXPECT_EQ_MEM(Uuid_NamespaceOID.m_bytes, oid, 16);

    uint8_t x500[16]{ 0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };
    HE_EXPECT_EQ_MEM(Uuid_NamespaceX500.m_bytes, x500, 16);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Hasher)
{
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_Zero), 0x00000000);

#if HE_CPU_64_BIT
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceDNS), 0xd111ad9d10b8a76b);
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceURL), 0xd111ad9d11b8a76b);
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceOID), 0xd111ad9d12b8a76b);
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceX500), 0xd111ad9d14b8a76b);
#else
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceDNS), 0x10b8a76b);
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceURL), 0x11b8a76b);
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceOID), 0x12b8a76b);
    HE_EXPECT_EQ(Hasher<Uuid>()(Uuid_NamespaceX500), 0x14b8a76b);
#endif
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, fmt)
{
    String str;

    // Test formatting each of the well-known UUIDs
    str = Format("{}", Uuid_Zero);
    HE_EXPECT_EQ(str, "00000000-0000-0000-0000-000000000000");

    str = Format("{}", Uuid_NamespaceDNS);
    HE_EXPECT_EQ(str, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");

    str = Format("{}", Uuid_NamespaceURL);
    HE_EXPECT_EQ(str, "6ba7b811-9dad-11d1-80b4-00c04fd430c8");

    str = Format("{}", Uuid_NamespaceOID);
    HE_EXPECT_EQ(str, "6ba7b812-9dad-11d1-80b4-00c04fd430c8");

    str = Format("{}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6ba7b814-9dad-11d1-80b4-00c04fd430c8");

    // Test formatting with the lower specifier.
    str = Format("{:x}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6ba7b814-9dad-11d1-80b4-00c04fd430c8");

    str = Format("{:sx}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6ba7b8149dad11d180b400c04fd430c8");

    str = Format("{:dx}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6ba7b814-9dad-11d1-80b4-00c04fd430c8");

    // Test formatting with the upper specifier.
    str = Format("{:X}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6BA7B814-9DAD-11D1-80B4-00C04FD430C8");

    str = Format("{:sX}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6BA7B8149DAD11D180B400C04FD430C8");

    str = Format("{:dX}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6BA7B814-9DAD-11D1-80B4-00C04FD430C8");

    // Test formatting with simple/dashed specifiers.
    str = Format("{:s}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6ba7b8149dad11d180b400c04fd430c8");

    str = Format("{:d}", Uuid_NamespaceX500);
    HE_EXPECT_EQ(str, "6ba7b814-9dad-11d1-80b4-00c04fd430c8");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Uuid_String_Roundtrip)
{
    const Uuid uuid = Uuid::CreateV4();

    const String str = Format("{}", uuid);

    const Uuid uuid2 = Uuid::FromString(str);
    HE_EXPECT_EQ(uuid, uuid2);

    const String str2 = Format("{:x}", uuid2);
    HE_EXPECT_EQ(str, str2);

    const Uuid uuid3 = Uuid::FromString(str2);
    HE_EXPECT_EQ(uuid, uuid3);

    const String str3 = Format("{:X}", uuid3);
    // HE_EXPECT_EQ(str, str3); // case mismatch

    const Uuid uuid4 = Uuid::FromString(str3);
    HE_EXPECT_EQ(uuid, uuid4);
}

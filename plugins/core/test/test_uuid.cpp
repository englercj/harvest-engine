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
HE_TEST(core, uuid, CreateUuidV4)
{
    Uuid uuid = Uuid::CreateV4();

    HE_EXPECT_NE(uuid, Uuid_Zero);
    HE_EXPECT_EQ(uuid.GetVersion(), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, GetVersion)
{
    HE_EXPECT_EQ(Uuid_Zero.GetVersion(), 0);
    HE_EXPECT_EQ(Uuid_NamespaceDNS.GetVersion(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceURL.GetVersion(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceOID.GetVersion(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceX500.GetVersion(), 1);

    for (uint32_t i = 0; i < 32; ++i)
    {
        const Uuid id = Uuid::CreateV4();
        HE_EXPECT_EQ(id.GetVersion(), 4);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, uuid, Operators)
{
    constexpr Uuid UuidOne{ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } };

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
    static_assert(Uuid_Zero.GetVersion() == 0);
    static_assert(Uuid_NamespaceDNS.GetVersion() == 1);
    static_assert(Uuid_NamespaceURL.GetVersion() == 1);
    static_assert(Uuid_NamespaceOID.GetVersion() == 1);
    static_assert(Uuid_NamespaceX500.GetVersion() == 1);

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
HE_TEST(core, uuid, Hash)
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

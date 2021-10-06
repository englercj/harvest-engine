// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/uuid.h"

#include "he/core/test.h"
#include "he/core/uuid_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
// Name string is a fully-qualified domain name.
// 6ba7b810-9dad-11d1-80b4-00c04fd430c8
constexpr Uuid Uuid_NamespaceDNS{ { 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

// Name string is a URL.
// 6ba7b811-9dad-11d1-80b4-00c04fd430c8
constexpr Uuid Uuid_NamespaceURL{ { 0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

// Name string is an ISO OID.
// 6ba7b812-9dad-11d1-80b4-00c04fd430c8
constexpr Uuid Uuid_NamespaceOID{ { 0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

// Name string is an X.500 DN (in DER or a text output format).
// 6ba7b814-9dad-11d1-80b4-00c04fd430c8
constexpr Uuid Uuid_NamespaceX500{ { 0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 } };

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Uuid, Uuid_Zero)
{
    static_assert(Uuid_Zero.GetVersion() == 0);

    uint8_t zero[16]{};
    HE_EXPECT_EQ_MEM(Uuid_Zero.m_bytes, zero, 16);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Uuid, Operators)
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
HE_TEST(core, Uuid, ToString)
{
    String str(CrtAllocator::Get());

    str = Uuid_Zero.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(str, "00000000-0000-0000-0000-000000000000");

    str = Uuid_NamespaceDNS.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(str, "6ba7b810-9dad-11d1-80b4-00c04fd430c8");

    str = Uuid_NamespaceURL.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(str, "6ba7b811-9dad-11d1-80b4-00c04fd430c8");

    str = Uuid_NamespaceOID.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(str, "6ba7b812-9dad-11d1-80b4-00c04fd430c8");

    str = Uuid_NamespaceX500.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(str, "6ba7b814-9dad-11d1-80b4-00c04fd430c8");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Uuid, FromString)
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
HE_TEST(core, Uuid, GetVersion)
{
    HE_EXPECT_EQ(Uuid_Zero.GetVersion(), 0);
    HE_EXPECT_EQ(Uuid_NamespaceDNS.GetVersion(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceURL.GetVersion(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceOID.GetVersion(), 1);
    HE_EXPECT_EQ(Uuid_NamespaceX500.GetVersion(), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Uuid, CreateUuidV4)
{
    Uuid uuid = Uuid::CreateV4();

    HE_EXPECT_NE(uuid, Uuid_Zero);
    HE_EXPECT_EQ(uuid.GetVersion(), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Uuid, Uuid_String_Roundtrip)
{
    Uuid uuid = Uuid::CreateV4();

    String str = uuid.ToString(CrtAllocator::Get());

    Uuid uuid2 = Uuid::FromString(str);
    HE_EXPECT_EQ(uuid, uuid2);

    String str2 = uuid2.ToString(CrtAllocator::Get());
    HE_EXPECT_EQ(str, str2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Uuid, Hash)
{
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_Zero), 0x00000000);

#if HE_CPU_64_BIT
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceDNS), 0xd111ad9d10b8a76b);
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceURL), 0xd111ad9d11b8a76b);
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceOID), 0xd111ad9d12b8a76b);
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceX500), 0xd111ad9d14b8a76b);
#else
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceDNS), 0x10b8a76b);
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceURL), 0x11b8a76b);
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceOID), 0x12b8a76b);
    HE_EXPECT_EQ(std::hash<Uuid>()(Uuid_NamespaceX500), 0x14b8a76b);
#endif
}

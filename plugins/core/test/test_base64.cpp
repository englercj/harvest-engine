// Copyright Chad Engler

#include "he/core/base64.h"

#include "he/core/test.h"
#include "he/core/vector.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Base64EncodedSize)
{
    static_assert(Base64EncodedSize(1111) == 1484);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Base64MaxDecodedSize)
{
    static_assert(Base64MaxDecodedSize(1484) == 1113);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Base64Encode)
{
    // Validate the test vectors from RFC4648
    HE_EXPECT_EQ(Base64Encode(""), "");
    HE_EXPECT_EQ(Base64Encode("f"), "Zg==");
    HE_EXPECT_EQ(Base64Encode("fo"), "Zm8=");
    HE_EXPECT_EQ(Base64Encode("foo"), "Zm9v");
    HE_EXPECT_EQ(Base64Encode("foob"), "Zm9vYg==");
    HE_EXPECT_EQ(Base64Encode("fooba"), "Zm9vYmE=");
    HE_EXPECT_EQ(Base64Encode("foobar"), "Zm9vYmFy");

    // Larger encode from the beginning of Moby Dick
    HE_EXPECT_EQ(Base64Encode(
        "Call me Ishmael. Some years ago--never mind how long precisely--having\n"
        "little or no money in my purse, and nothing particular to interest me on\n"
        "shore, I thought I would sail about a little and see the watery part of\n"
        "the world. It is a way I have of driving off the spleen and regulating\n"
        "the circulation. Whenever I find myself growing grim about the mouth;\n"
        "whenever it is a damp, drizzly November in my soul; whenever I find\n"
        "myself involuntarily pausing before coffin warehouses, and bringing up\n"
        "the rear of every funeral I meet; and especially whenever my hypos get\n"
        "such an upper hand of me, that it requires a strong moral principle to\n"
        "prevent me from deliberately stepping into the street, and methodically\n"
        "knocking people's hats off--then, I account it high time to get to sea\n"
        "as soon as I can. This is my substitute for pistol and ball. With a\n"
        "philosophical flourish Cato throws himself upon his sword; I quietly\n"
        "take to the ship. There is nothing surprising in this. If they but knew\n"
        "it, almost all men in their degree, some time or other, cherish very\n"
        "nearly the same feelings towards the ocean with me.\n"),
        "Q2FsbCBtZSBJc2htYWVsLiBTb21lIHllYXJzIGFnby0tbmV2ZXIgbWluZCBob3cgbG9uZ"
        "yBwcmVjaXNlbHktLWhhdmluZwpsaXR0bGUgb3Igbm8gbW9uZXkgaW4gbXkgcHVyc2UsIG"
        "FuZCBub3RoaW5nIHBhcnRpY3VsYXIgdG8gaW50ZXJlc3QgbWUgb24Kc2hvcmUsIEkgdGh"
        "vdWdodCBJIHdvdWxkIHNhaWwgYWJvdXQgYSBsaXR0bGUgYW5kIHNlZSB0aGUgd2F0ZXJ5"
        "IHBhcnQgb2YKdGhlIHdvcmxkLiBJdCBpcyBhIHdheSBJIGhhdmUgb2YgZHJpdmluZyBvZ"
        "mYgdGhlIHNwbGVlbiBhbmQgcmVndWxhdGluZwp0aGUgY2lyY3VsYXRpb24uIFdoZW5ldm"
        "VyIEkgZmluZCBteXNlbGYgZ3Jvd2luZyBncmltIGFib3V0IHRoZSBtb3V0aDsKd2hlbmV"
        "2ZXIgaXQgaXMgYSBkYW1wLCBkcml6emx5IE5vdmVtYmVyIGluIG15IHNvdWw7IHdoZW5l"
        "dmVyIEkgZmluZApteXNlbGYgaW52b2x1bnRhcmlseSBwYXVzaW5nIGJlZm9yZSBjb2Zma"
        "W4gd2FyZWhvdXNlcywgYW5kIGJyaW5naW5nIHVwCnRoZSByZWFyIG9mIGV2ZXJ5IGZ1bm"
        "VyYWwgSSBtZWV0OyBhbmQgZXNwZWNpYWxseSB3aGVuZXZlciBteSBoeXBvcyBnZXQKc3V"
        "jaCBhbiB1cHBlciBoYW5kIG9mIG1lLCB0aGF0IGl0IHJlcXVpcmVzIGEgc3Ryb25nIG1v"
        "cmFsIHByaW5jaXBsZSB0bwpwcmV2ZW50IG1lIGZyb20gZGVsaWJlcmF0ZWx5IHN0ZXBwa"
        "W5nIGludG8gdGhlIHN0cmVldCwgYW5kIG1ldGhvZGljYWxseQprbm9ja2luZyBwZW9wbG"
        "UncyBoYXRzIG9mZi0tdGhlbiwgSSBhY2NvdW50IGl0IGhpZ2ggdGltZSB0byBnZXQgdG8"
        "gc2VhCmFzIHNvb24gYXMgSSBjYW4uIFRoaXMgaXMgbXkgc3Vic3RpdHV0ZSBmb3IgcGlz"
        "dG9sIGFuZCBiYWxsLiBXaXRoIGEKcGhpbG9zb3BoaWNhbCBmbG91cmlzaCBDYXRvIHRoc"
        "m93cyBoaW1zZWxmIHVwb24gaGlzIHN3b3JkOyBJIHF1aWV0bHkKdGFrZSB0byB0aGUgc2"
        "hpcC4gVGhlcmUgaXMgbm90aGluZyBzdXJwcmlzaW5nIGluIHRoaXMuIElmIHRoZXkgYnV"
        "0IGtuZXcKaXQsIGFsbW9zdCBhbGwgbWVuIGluIHRoZWlyIGRlZ3JlZSwgc29tZSB0aW1l"
        "IG9yIG90aGVyLCBjaGVyaXNoIHZlcnkKbmVhcmx5IHRoZSBzYW1lIGZlZWxpbmdzIHRvd"
        "2FyZHMgdGhlIG9jZWFuIHdpdGggbWUuCg==");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Base64Encode_Container)
{
    String str;

    // Validate the test vectors from RFC4648
    str.Clear();
    Base64Encode(str, "");
    HE_EXPECT_EQ(str, "");

    str.Clear();
    Base64Encode(str, "f");
    HE_EXPECT_EQ(str, "Zg==");

    str.Clear();
    Base64Encode(str, "fo");
    HE_EXPECT_EQ(str, "Zm8=");

    str.Clear();
    Base64Encode(str, "foo");
    HE_EXPECT_EQ(str, "Zm9v");

    str.Clear();
    Base64Encode(str, "foob");
    HE_EXPECT_EQ(str, "Zm9vYg==");

    str.Clear();
    Base64Encode(str, "fooba");
    HE_EXPECT_EQ(str, "Zm9vYmE=");

    str.Clear();
    Base64Encode(str, "foobar");
    HE_EXPECT_EQ(str, "Zm9vYmFy");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Base64Decode)
{
    // Validate the test vectors from RFC4648
    const StringView inputStrings[] =
    {
        "",
        "Zg==",
        "Zm8=",
        "Zm9v",
        "Zm9vYg==",
        "Zm9vYmE=",
        "Zm9vYmFy",
    };
    const char expected[] = "foobar";
    static_assert(HE_LENGTH_OF(expected) == HE_LENGTH_OF(inputStrings));

    uint8_t bytes[64];
    for (uint32_t i = 0; i < HE_LENGTH_OF(expected); ++i)
    {
        const uint32_t len = Base64Decode(bytes, sizeof(bytes), inputStrings[i]);
        HE_EXPECT_EQ(len, i);

        if (len > 0)
        {
            HE_EXPECT_EQ_MEM(bytes, expected, len);
        }
    }

    // Invalid decodes
    uint32_t len = 0;
    len = Base64Decode(bytes, sizeof(bytes), "Zm9vYg=");
    HE_EXPECT_EQ(len, 0);
    len = Base64Decode(bytes, sizeof(bytes), "Zm9vYg");
    HE_EXPECT_EQ(len, 0);
    len = Base64Decode(bytes, sizeof(bytes), "Zm9vY");
    HE_EXPECT_EQ(len, 0);
    len = Base64Decode(bytes, sizeof(bytes), "Zm9vYmF=Zm9v");
    HE_EXPECT_EQ(len, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Base64Decode_Container)
{
    // Validate the test vectors from RFC4648
    const StringView inputStrings[] =
    {
        "",
        "Zg==",
        "Zm8=",
        "Zm9v",
        "Zm9vYg==",
        "Zm9vYmE=",
        "Zm9vYmFy",
    };
    const char expected[] = "foobar";
    static_assert(HE_LENGTH_OF(expected) == HE_LENGTH_OF(inputStrings));

    Vector<uint8_t> bytes;
    for (uint32_t i = 0; i < HE_LENGTH_OF(expected); ++i)
    {
        bytes.Clear();
        HE_EXPECT(Base64Decode(bytes, inputStrings[i]));
        HE_EXPECT_EQ(bytes.Size(), i);

        if (i > 0)
        {
            HE_EXPECT_EQ_MEM(bytes.Data(), expected, bytes.Size());
        }
    }

    // Invalid decodes
    HE_EXPECT(!Base64Decode(bytes, "Zm9vYg="));
    HE_EXPECT(!Base64Decode(bytes, "Zm9vYg"));
    HE_EXPECT(!Base64Decode(bytes, "Zm9vY"));
    HE_EXPECT(!Base64Decode(bytes, "Zm9vYmF=Zm9v"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, Roundtrip)
{
    constexpr StringView SourceMessage = "This is a test message for the base64 encoder & decoder.";

    String encoded = Base64Encode(SourceMessage);

    Vector<uint8_t> bytes;
    HE_EXPECT(Base64Decode(bytes, encoded));
    HE_EXPECT_EQ(bytes.Size(), SourceMessage.Size());
    HE_EXPECT_EQ_MEM(bytes.Data(), SourceMessage.Data(), SourceMessage.Size());
}

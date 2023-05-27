// Copyright Chad Engler

#include "he/core/base64.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
static void TestB64(StringView input, StringView expected)
{
    const String result = Base64Encode(reinterpret_cast<const uint8_t*>(input.Data()), input.Size());
    HE_EXPECT_EQ(result, expected);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, base64, test)
{
    TestB64("", "");
    TestB64("f", "Zg==");
    TestB64("fo", "Zm8=");
    TestB64("foo", "Zm9v");
    TestB64("foob", "Zm9vYg==");
    TestB64("fooba", "Zm9vYmE=");
    TestB64("foobar", "Zm9vYmFy");

    TestB64(R"(Call me Ishmael. Some years ago--never mind how long precisely--having
little or no money in my purse, and nothing particular to interest me on
shore, I thought I would sail about a little and see the watery part of
the world. It is a way I have of driving off the spleen and regulating
the circulation. Whenever I find myself growing grim about the mouth;
whenever it is a damp, drizzly November in my soul; whenever I find
myself involuntarily pausing before coffin warehouses, and bringing up
the rear of every funeral I meet; and especially whenever my hypos get
such an upper hand of me, that it requires a strong moral principle to
prevent me from deliberately stepping into the street, and methodically
knocking people's hats off--then, I account it high time to get to sea
as soon as I can. This is my substitute for pistol and ball. With a
philosophical flourish Cato throws himself upon his sword; I quietly
take to the ship. There is nothing surprising in this. If they but knew
it, almost all men in their degree, some time or other, cherish very
nearly the same feelings towards the ocean with me.)",
    "Q2FsbCBtZSBJc2htYWVsLiBTb21lIHllYXJzIGFnby0tbmV2ZXIgbWluZCBob3cgbG9uZyBwcmVjaXNlbHktLWhhdmluZwpsaXR0bGUgb3Igbm8gbW9uZXkgaW4gbXkgcHVyc2UsIGFuZCBub3RoaW5nIHBhcnRpY3VsYXIgdG8gaW50ZXJlc3QgbWUgb24Kc2hvcmUsIEkgdGhvdWdodCBJIHdvdWxkIHNhaWwgYWJvdXQgYSBsaXR0bGUgYW5kIHNlZSB0aGUgd2F0ZXJ5IHBhcnQgb2YKdGhlIHdvcmxkLiBJdCBpcyBhIHdheSBJIGhhdmUgb2YgZHJpdmluZyBvZmYgdGhlIHNwbGVlbiBhbmQgcmVndWxhdGluZwp0aGUgY2lyY3VsYXRpb24uIFdoZW5ldmVyIEkgZmluZCBteXNlbGYgZ3Jvd2luZyBncmltIGFib3V0IHRoZSBtb3V0aDsKd2hlbmV2ZXIgaXQgaXMgYSBkYW1wLCBkcml6emx5IE5vdmVtYmVyIGluIG15IHNvdWw7IHdoZW5ldmVyIEkgZmluZApteXNlbGYgaW52b2x1bnRhcmlseSBwYXVzaW5nIGJlZm9yZSBjb2ZmaW4gd2FyZWhvdXNlcywgYW5kIGJyaW5naW5nIHVwCnRoZSByZWFyIG9mIGV2ZXJ5IGZ1bmVyYWwgSSBtZWV0OyBhbmQgZXNwZWNpYWxseSB3aGVuZXZlciBteSBoeXBvcyBnZXQKc3VjaCBhbiB1cHBlciBoYW5kIG9mIG1lLCB0aGF0IGl0IHJlcXVpcmVzIGEgc3Ryb25nIG1vcmFsIHByaW5jaXBsZSB0bwpwcmV2ZW50IG1lIGZyb20gZGVsaWJlcmF0ZWx5IHN0ZXBwaW5nIGludG8gdGhlIHN0cmVldCwgYW5kIG1ldGhvZGljYWxseQprbm9ja2luZyBwZW9wbGUncyBoYXRzIG9mZi0tdGhlbiwgSSBhY2NvdW50IGl0IGhpZ2ggdGltZSB0byBnZXQgdG8gc2VhCmFzIHNvb24gYXMgSSBjYW4uIFRoaXMgaXMgbXkgc3Vic3RpdHV0ZSBmb3IgcGlzdG9sIGFuZCBiYWxsLiBXaXRoIGEKcGhpbG9zb3BoaWNhbCBmbG91cmlzaCBDYXRvIHRocm93cyBoaW1zZWxmIHVwb24gaGlzIHN3b3JkOyBJIHF1aWV0bHkKdGFrZSB0byB0aGUgc2hpcC4gVGhlcmUgaXMgbm90aGluZyBzdXJwcmlzaW5nIGluIHRoaXMuIElmIHRoZXkgYnV0IGtuZXcKaXQsIGFsbW9zdCBhbGwgbWVuIGluIHRoZWlyIGRlZ3JlZSwgc29tZSB0aW1lIG9yIG90aGVyLCBjaGVyaXNoIHZlcnkKbmVhcmx5IHRoZSBzYW1lIGZlZWxpbmdzIHRvd2FyZHMgdGhlIG9jZWFuIHdpdGggbWUuCg==");
}

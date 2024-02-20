// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/file.h"
#include "he/core/random.h"
#include "he/core/result_fmt.h"
#include "he/core/test.h"

namespace he
{
    void TestAllocator(Allocator& alloc)
    {
        // Malloc -> Realloc -> Free
        {
            void* mem = alloc.Malloc(16);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

            mem = alloc.Realloc(mem, 32);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

            alloc.Free(mem);
        }

        // Malloc overaligned
        {
            void* mem = alloc.Malloc(16, 128);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, 128));

            mem = alloc.Realloc(mem, 32, 128);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, 128));

            alloc.Free(mem);
        }

        // Malloc unique
        {
            void* mem1 = alloc.Malloc(16);
            void* mem2 = alloc.Malloc(16);
            HE_EXPECT(mem1 != mem2);

            alloc.Free(mem1);
            alloc.Free(mem2);
        }

        // Realloc as all 3 operations
        {
            void* mem = alloc.Realloc(nullptr, 16);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

            mem = alloc.Realloc(mem, 32);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, Allocator::DefaultAlignment));

            alloc.Realloc(mem, 0);
        }

        // Malloc<T> trivial
        {
            uint32_t* mem = alloc.Malloc<uint32_t>(16);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(uint32_t)));

            alloc.Free(mem);
        }

        // Malloc<T> non-trivial
        {
            static uint32_t s_ctorCount = 0;
            static uint32_t s_dtorCount = 0;
            struct CtorTest { CtorTest() { ++s_ctorCount; } ~CtorTest() { ++s_dtorCount; } };

            CtorTest* mem = alloc.Malloc<CtorTest>(16);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(CtorTest)));
            HE_EXPECT_EQ(s_ctorCount, 0);
            HE_EXPECT_EQ(s_dtorCount, 0);

            alloc.Free(mem);
            HE_EXPECT_EQ(s_ctorCount, 0);
            HE_EXPECT_EQ(s_dtorCount, 0);
        }

        // New -> Delete trivial
        {
            uint32_t* mem = alloc.New<uint32_t>();
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(uint32_t)));

            alloc.Delete(mem);
        }

        // New -> Delete non-trivial
        {
            static uint32_t s_ctorCount = 0;
            static uint32_t s_dtorCount = 0;
            struct CtorTest { CtorTest(uint32_t x) { s_ctorCount += x; } ~CtorTest() { ++s_dtorCount; } };

            s_ctorCount = 0;
            s_dtorCount = 0;

            CtorTest* mem = alloc.New<CtorTest>(16u);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(CtorTest)));
            HE_EXPECT_EQ(s_ctorCount, 16);
            HE_EXPECT_EQ(s_dtorCount, 0);

            alloc.Delete(mem);
            HE_EXPECT_EQ(s_ctorCount, 16);
            HE_EXPECT_EQ(s_dtorCount, 1);
        }

        // New -> Delete overaligned
        {
            static uint32_t s_ctorCount = 0;
            static uint32_t s_dtorCount = 0;
            struct alignas(128) OverAligned { OverAligned(uint32_t x) { s_ctorCount += x; } ~OverAligned() { ++s_dtorCount; } char x[128]; };

            s_ctorCount = 0;
            s_dtorCount = 0;

            OverAligned* mem = alloc.New<OverAligned>(16u);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(OverAligned)));
            HE_EXPECT_EQ(s_ctorCount, 16);
            HE_EXPECT_EQ(s_dtorCount, 0);

            alloc.Delete(mem);
            HE_EXPECT_EQ(s_ctorCount, 16);
            HE_EXPECT_EQ(s_dtorCount, 1);
        }

        // NewArray -> DeleteArray trivial
        {
            uint32_t* mem = alloc.NewArray<uint32_t>(16);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(uint32_t)));

            alloc.DeleteArray(mem);
        }

        // NewArray -> DeleteArray non-trivial
        {
            static uint32_t s_ctorCount = 0;
            static uint32_t s_dtorCount = 0;
            struct CtorTest { CtorTest(uint32_t x) { s_ctorCount += x; } ~CtorTest() { ++s_dtorCount; } };

            s_ctorCount = 0;
            s_dtorCount = 0;

            CtorTest* mem = alloc.NewArray<CtorTest>(16, 2u);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(CtorTest)));
            HE_EXPECT_EQ(s_ctorCount, 32);
            HE_EXPECT_EQ(s_dtorCount, 0);

            alloc.DeleteArray(mem);
            HE_EXPECT_EQ(s_ctorCount, 32);
            HE_EXPECT_EQ(s_dtorCount, 16);
        }

        // NewArray -> DeleteArray overaligned
        {
            static uint32_t s_ctorCount = 0;
            static uint32_t s_dtorCount = 0;
            struct alignas(128) OverAligned { OverAligned(uint32_t x) { s_ctorCount += x; } ~OverAligned() { ++s_dtorCount; } char x[128]; };

            s_ctorCount = 0;
            s_dtorCount = 0;

            OverAligned* mem = alloc.NewArray<OverAligned>(16, 2u);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(OverAligned)));
            HE_EXPECT_EQ(s_ctorCount, 32);
            HE_EXPECT_EQ(s_dtorCount, 0);

            alloc.DeleteArray(mem);
            HE_EXPECT_EQ(s_ctorCount, 32);
            HE_EXPECT_EQ(s_dtorCount, 16);
        }

        // Soak test the allocator with some randomly sized allocations
        {
            Random64 rng;

            static constexpr uint32_t NumAllocs = 5000;
            static constexpr uint32_t MaxAllocSize = 2048;

            uint8_t expected[MaxAllocSize];

            size_t sizes[NumAllocs];
            void* ptrs[NumAllocs];

            // Allocate a bunch of memory of random sizes
            for (uint32_t i = 0; i < NumAllocs; ++i)
            {
                sizes[i] = rng.Next(1, MaxAllocSize);
                ptrs[i] = alloc.Malloc(sizes[i]);

                HE_EXPECT_NE_PTR(ptrs[i], nullptr);
                MemSet(ptrs[i], i, sizes[i]);

                // There is a small chance to free the allocation immediately
                if (rng.Real() > 0.95)
                {
                    alloc.Free(ptrs[i]);
                    ptrs[i] = nullptr;
                    sizes[i] = 0;
                }
            }

            // Check & free all the allocations
            for (uint32_t i = 0; i < NumAllocs; ++i)
            {
                // Check if there have been any memory stomps from the allocations
                if (ptrs[i])
                {
                    MemSet(expected, i, sizes[i]);
                    HE_EXPECT_EQ_MEM(ptrs[i], expected, sizes[i]);
                }

                // Free it up
                alloc.Free(ptrs[i]);
            }
        }
    }

    void TouchTestFile(const char* path, const void* data, uint32_t len)
    {
        File f;
        Result r = f.Open(path, FileOpenMode::WriteTruncate);
        HE_EXPECT(r, r);

        if (data && len > 0)
        {
            r = f.Write(data, len);
            HE_EXPECT(r, r);
        }

        f.Close();
    }

    StringView GetTestTomlDocument()
    {
        static const StringView s_document = R"(# Copyright Chad Engler

[boolean]
bool1 = true
bool2 = false

[integer]
int1 = +99
int2 = 42
int3 = 0
int4 = -17
int5 = 1_000
int6 = 5_349_221
int7 = 53_49_221
int8 = 1_2_3_4_5     # valid but inadvisable

# hexadecimal with prefix `0x`
hex1 = 0xDEADBEEF
hex2 = 0xdeadbeef
hex3 = 0xdead_beef

# octal with prefix `0o`
oct1 = 0o01234567
oct2 = 0o755 # useful for Unix file permissions

# binary with prefix `0b`
bin1 = 0b11010110

[float]
# fractional
flt1 = +1.0
flt2 = 3.1415
flt3 = -0.01

# exponent
flt4 = 5e+22
flt5 = 1e06
flt6 = -2E-2

# both
flt7 = 6.626e-34
flt8 = 224_617.445_991_228

# infinity
sf1 = inf  # positive infinity
sf2 = +inf # positive infinity
sf3 = -inf # negative infinity

# not a number
sf4 = nan  # actual sNaN/qNaN encoding is implementation-specific
sf5 = +nan # same as `nan`
sf6 = -nan # valid, actual encoding is implementation-specific

[string]
str = "I'm a string. \"You can quote me\". Name\tJos\xE9\nLocation\tSF."
str1 = """
Roses are red
Violets are blue"""

# On a Unix system, the above multi-line string will most likely be the same as:
str2 = "Roses are red\nViolets are blue"

# On a Windows system, it will most likely be equivalent to:
str3 = "Roses are red\r\nViolets are blue"

# The following strings are byte-for-byte equivalent:
str4 = "The quick brown fox jumps over the lazy dog."

str5 = """
The quick brown \


  fox jumps over \
    the lazy dog."""

str6 = """\
       The quick brown \
       fox jumps over \
       the lazy dog.\
       """

str7 = """Here are two quotation marks: "". Simple enough."""
str8 = """Here are three quotation marks: ""\"."""
str9 = """Here are fifteen quotation marks: ""\"""\"""\"""\"""\"."""

# What you see is what you get.
winpath  = 'C:\Users\nodejs\templates'
winpath2 = '\\ServerX\admin$\system32\'
quoted   = 'Tom "Dubs" Preston-Werner'
regex    = '<\i\c*\s*>'
regex2 = '''I [dw]on't need \d{2} apples'''
lines  = '''
The first newline is
trimmed in literal strings.
   All other whitespace
   is preserved.
'''
quot15 = '''Here are fifteen quotation marks: """""""""""""""'''
apos15 = "Here are fifteen apostrophes: '''''''''''''''"

# 'That,' she said, 'is still pointless.'
str11 = ''''That,' she said, 'is still pointless.''''
str12 = ''''That,' she said, 'is still pointless.'''''

[datetime]
odt1 = 1979-05-27T07:32:00Z
odt2 = 1979-05-27T00:32:00-07:00
odt3 = 1979-05-27T00:32:00.999999-07:00
odt4 = 1979-05-27 07:32:00Z
odt5 = 1979-05-27 07:32Z
odt6 = 1979-05-27 00:32-07:00

ldt1 = 1979-05-27T00:32:00
ldt2 = 1979-05-27T00:32:00.999999
ldt3 = 1979-05-27T00:32

ld1 = 1979-05-27

[time]
lt1 = 07:32:00
lt2 = 00:32:00.999999
lt3 = 07:32

[array]
integers = [ 1, 2, 3 ]
colors = [ "red", "yellow", "green" ]
nested_arrays_of_ints = [ [ 1, 2 ], [3, 4, 5] ]
nested_mixed_array = [ [ 1, 2 ], ["a", "b", "c"] ]
string_array = [ "all", 'strings', """are the same""", '''type''' ]

# Mixed-type arrays are allowed
numbers = [ 0.1, 0.2, 0.5, 1, 2, 5 ]
contributors = [
    "Foo Bar <foo@example.com>",
    { name = "Baz Qux", email = "bazqux@example.com", url = "https://example.com/bazqux" }
]
integers2 = [
  1, 2, 3
]

integers3 = [
  1,
  2, # this is ok
]

[table]
[table-1]
key1 = "some string"
key2 = 123

[table-2]
key1 = "another string"
key2 = 456

[dog."tater.man"]
type.name = "pug"

[a.b.c]            # this is best practice
[ d.e.f ]          # same as [d.e.f]
[ g .  h  . i ]    # same as [g.h.i]
[ j . "ʞ" . 'l' ]  # same as [j."ʞ".'l']

# [x] you
# [x.y] don't
# [x.y.z] need these
[x.y.z.w] # for this to work

[x] # defining a super-table afterward is ok

[fruit]
apple.color = "red"
apple.taste.sweet = true

[fruit.apple.texture]  # you can add sub-tables
smooth = true

[inline.table]
name = { first = "Tom", last = "Preston-Werner" }
point = {x=1, y=2}
animal = { type.name = "pug" }
contact = {
    personal = {
        name = "Donald Duck",
        email = "donald@duckburg.com",
    },
    work = {
        name = "Coin cleaner",
        email = "donald@ScroogeCorp.com",
    },
}

[[product]]
name = "Hammer"
sku = 738594937

[[product]]  # empty table within the array

[[product]]
name = "Nail"
sku = 284758393

color = "gray"

points = [ { x = 1, y = 2, z = 3 },
           { x = 7, y = 8, z = 9 },
           { x = 2, y = 4, z = 8 } ]
)";
        return s_document;
    }
}

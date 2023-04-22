// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/file.h"
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

            CtorTest* mem = alloc.New<CtorTest>(16);
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

            OverAligned* mem = alloc.New<OverAligned>(16);
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

            CtorTest* mem = alloc.NewArray<CtorTest>(16, 2);
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

            OverAligned* mem = alloc.NewArray<OverAligned>(16, 2);
            HE_EXPECT(mem);
            HE_EXPECT(IsAligned(mem, alignof(OverAligned)));
            HE_EXPECT_EQ(s_ctorCount, 32);
            HE_EXPECT_EQ(s_dtorCount, 0);

            alloc.DeleteArray(mem);
            HE_EXPECT_EQ(s_ctorCount, 32);
            HE_EXPECT_EQ(s_dtorCount, 16);
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
}

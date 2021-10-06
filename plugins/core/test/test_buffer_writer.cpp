// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/buffer_writer.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Construct)
{
    Allocator& a = CrtAllocator::Get();

    {
        BufferWriter b(a);
        HE_EXPECT(b.IsEmpty());
        HE_EXPECT_EQ(b.Capacity(), 0);
        HE_EXPECT(!b.Data());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Construct_Copy)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint8_t Datas[]{ 0x12, 0x23, 0x45, 0x67, 0x89 };

    BufferWriter buf(BufferWriter::GrowthStrategy::Fixed, 20.5f, a);
    buf.Write(Datas);
    HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
    HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());

    {
        BufferWriter copy(buf, a);
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ_MEM(copy.Data(), buf.Data(), buf.Size());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(copy), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(copy), BufferWriterTestAttorney::GetStrategy(buf));
    }

    {
        AnotherAllocator a2;
        BufferWriter copy(buf, a2);
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ_MEM(copy.Data(), buf.Data(), buf.Size());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(copy), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(copy), BufferWriterTestAttorney::GetStrategy(buf));
    }

    {
        BufferWriter copy(buf);
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ_MEM(copy.Data(), buf.Data(), buf.Size());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &buf.GetAllocator());
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(copy), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(copy), BufferWriterTestAttorney::GetStrategy(buf));
    }

    {
        BufferWriter copy = buf;
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ_MEM(copy.Data(), buf.Data(), buf.Size());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &buf.GetAllocator());
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(copy), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(copy), BufferWriterTestAttorney::GetStrategy(buf));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Construct_Move)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint8_t Datas[]{ 0x12, 0x23, 0x45, 0x67, 0x89 };

    {
        BufferWriter buf(a);
        buf.Write(Datas);
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());
        const uint8_t* ptr = buf.Data();

        BufferWriter moved(Move(buf), a);
        HE_EXPECT_EQ(moved.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_PTR(moved.Data(), ptr);
        HE_EXPECT_EQ_MEM(moved.Data(), Datas, moved.Size());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(moved), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(moved), BufferWriterTestAttorney::GetStrategy(buf));

        HE_EXPECT(buf.IsEmpty());
        HE_EXPECT(!buf.Data());
    }

    {
        BufferWriter buf(a);
        buf.Write(Datas);
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());
        const uint8_t* ptr = buf.Data();

        AnotherAllocator a2;
        BufferWriter moved(Move(buf), a2);
        HE_EXPECT_EQ(moved.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_NE_PTR(moved.Data(), ptr);
        HE_EXPECT_EQ_MEM(moved.Data(), Datas, moved.Size());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(moved), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(moved), BufferWriterTestAttorney::GetStrategy(buf));

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Data(), ptr);
    }

    {
        BufferWriter buf(a);
        buf.Write(Datas);
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());
        const uint8_t* ptr = buf.Data();

        BufferWriter moved(Move(buf));
        HE_EXPECT_EQ(moved.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_PTR(moved.Data(), ptr);
        HE_EXPECT_EQ_MEM(moved.Data(), Datas, moved.Size());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &buf.GetAllocator());
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(moved), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(moved), BufferWriterTestAttorney::GetStrategy(buf));

        HE_EXPECT(buf.IsEmpty());
        HE_EXPECT(!buf.Data());
    }

    {
        BufferWriter buf(a);
        buf.Write(Datas);
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());
        const uint8_t* ptr = buf.Data();

        BufferWriter moved = Move(buf);
        HE_EXPECT_EQ(moved.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_PTR(moved.Data(), ptr);
        HE_EXPECT_EQ_MEM(moved.Data(), Datas, moved.Size());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &buf.GetAllocator());
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(moved), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(moved), BufferWriterTestAttorney::GetStrategy(buf));

        HE_EXPECT(buf.IsEmpty());
        HE_EXPECT(!buf.Data());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, operator_assign_copy)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint8_t Datas[]{ 0x12, 0x23, 0x45, 0x67, 0x89 };

    BufferWriter buf(BufferWriter::GrowthStrategy::Fixed, 20.5f, a);
    buf.Write(Datas);
    HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
    HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());

    {
        BufferWriter copy(a);
        copy = buf;
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ_MEM(copy.Data(), buf.Data(), buf.Size());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(copy), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(copy), BufferWriterTestAttorney::GetStrategy(buf));
    }

    {
        AnotherAllocator a2;
        BufferWriter copy(a2);
        copy = buf;
        HE_EXPECT_EQ(copy.Size(), buf.Size());
        HE_EXPECT_EQ_MEM(copy.Data(), buf.Data(), buf.Size());
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(copy), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(copy), BufferWriterTestAttorney::GetStrategy(buf));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, operator_assign_move)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint8_t Datas[]{ 0x12, 0x23, 0x45, 0x67, 0x89 };

    {
        BufferWriter buf(a);
        buf.Write(Datas);
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());
        const uint8_t* ptr = buf.Data();

        BufferWriter moved(a);
        moved = Move(buf);
        HE_EXPECT_EQ(moved.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_PTR(moved.Data(), ptr);
        HE_EXPECT_EQ_MEM(moved.Data(), Datas, moved.Size());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(moved), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(moved), BufferWriterTestAttorney::GetStrategy(buf));

        HE_EXPECT(buf.IsEmpty());
        HE_EXPECT(!buf.Data());
    }

    {
        BufferWriter buf(a);
        buf.Write(Datas);
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());
        const uint8_t* ptr = buf.Data();

        AnotherAllocator a2;
        BufferWriter moved(a2);
        moved = Move(buf);
        HE_EXPECT_EQ(moved.Size(), HE_LENGTH_OF(Datas));
        HE_EXPECT_NE_PTR(moved.Data(), ptr);
        HE_EXPECT_EQ_MEM(moved.Data(), Datas, moved.Size());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetGrowth(moved), BufferWriterTestAttorney::GetGrowth(buf));
        HE_EXPECT_EQ(BufferWriterTestAttorney::GetStrategy(moved), BufferWriterTestAttorney::GetStrategy(buf));

        HE_EXPECT(!buf.IsEmpty());
        HE_EXPECT_EQ_PTR(buf.Data(), ptr);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, operator_index)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint8_t Datas[]{ 0x12, 0x23, 0x45, 0x67, 0x89 };

    BufferWriter buf(a);
    buf.Write(Datas);
    HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));

    for (uint32_t i = 0; i < HE_LENGTH_OF(Datas); ++i)
    {
        HE_EXPECT_EQ(buf[i], Datas[i]);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, IsEmpty)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT(buf.IsEmpty());

    buf.Write(1);
    HE_EXPECT(!buf.IsEmpty());

    buf.Clear();
    HE_EXPECT(buf.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Capacity)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT_EQ(buf.Capacity(), 0);

    buf.Resize(1);
    HE_EXPECT_EQ(buf.Capacity(), BufferWriter::MinBytes);

    buf.Resize(BufferWriter::MinBytes * 2);
    HE_EXPECT_GE(buf.Capacity(), BufferWriter::MinBytes * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Size)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT_EQ(buf.Size(), 0);

    buf.Resize(1);
    HE_EXPECT_EQ(buf.Size(), 1);

    buf.Resize(BufferWriter::MinBytes * 2);
    HE_EXPECT_EQ(buf.Size(), BufferWriter::MinBytes * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Reserve)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT_EQ(buf.Capacity(), 0);

    buf.Reserve(1);
    HE_EXPECT_EQ(buf.Capacity(), BufferWriter::MinBytes);

    buf.Reserve(BufferWriter::MinBytes * 2);
    HE_EXPECT_GE(buf.Capacity(), BufferWriter::MinBytes * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Resize)
{
    Allocator& a = CrtAllocator::Get();

    {
        BufferWriter buf(a);
        buf.Resize(16);
        HE_EXPECT_EQ(buf.Size(), 16);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, ShrinkToFit)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_EQ(buf.Capacity(), 0);

    buf.Reserve(1024);
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_GE(buf.Capacity(), 1024);

    buf.ShrinkToFit();
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_GE(buf.Capacity(), 0);

    buf.Resize(2048);
    HE_EXPECT_EQ(buf.Size(), 2048);
    HE_EXPECT_GE(buf.Capacity(), 2048);

    buf.ShrinkToFit();
    HE_EXPECT_EQ(buf.Size(), 2048);
    HE_EXPECT_GE(buf.Capacity(), 2048);

    buf.Clear();
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_GE(buf.Capacity(), 2048);

    buf.ShrinkToFit();
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_GE(buf.Capacity(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Data)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT(!buf.Data());

    buf.Resize(16);
    HE_EXPECT_EQ_PTR(buf.Data(), BufferWriterTestAttorney::GetPtr(buf));

    buf.Clear();
    HE_EXPECT_EQ_PTR(buf.Data(), BufferWriterTestAttorney::GetPtr(buf));

    buf.ShrinkToFit();
    HE_EXPECT(!buf.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Release)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT(!buf.Data());

    buf.Resize(16);

    uint8_t* mem = buf.Release();
    HE_EXPECT(mem);

    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_EQ(buf.Capacity(), 0);
    HE_EXPECT(!buf.Data());

    a.Free(mem);

    // Call a few functions to ensure it continues to work after release
    buf.Resize(128);
    HE_EXPECT_EQ(buf.Size(), 128);
    HE_EXPECT_EQ(buf.Capacity(), 1024);

    buf.Clear();
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_EQ(buf.Capacity(), 1024);

    buf.ShrinkToFit();
    HE_EXPECT_EQ(buf.Size(), 0);
    HE_EXPECT_EQ(buf.Capacity(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, GetAllocator)
{
    Allocator& a = CrtAllocator::Get();
    BufferWriter b(a);
    HE_EXPECT_EQ_PTR(&b.GetAllocator(), &a);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Clear)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);
    HE_EXPECT(buf.IsEmpty());

    buf.Clear();
    HE_EXPECT(buf.IsEmpty());

    buf.Reserve(16);
    HE_EXPECT(buf.IsEmpty());

    buf.Resize(10);
    HE_EXPECT(!buf.IsEmpty());

    buf.Clear();
    HE_EXPECT(buf.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, Write)
{
    Allocator& a = CrtAllocator::Get();

    BufferWriter buf(a);

    // memory
    constexpr uint8_t Datas[]{ 0x12, 0x23, 0x45, 0x67, 0x89 };
    buf.Clear();
    buf.Write(Datas, HE_LENGTH_OF(Datas));
    HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(Datas));
    HE_EXPECT_EQ_MEM(buf.Data(), Datas, buf.Size());

    // string
    constexpr const char StrDatas[] = "testing";
    buf.Clear();
    buf.Write(StrDatas);
    HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(StrDatas) - 1);
    HE_EXPECT_EQ_MEM(buf.Data(), StrDatas, buf.Size());

    // object
    struct ObjData { int a = 66; float b = 1.5f; };
    constexpr ObjData ObjDatas{};
    buf.Clear();
    buf.Write(ObjDatas);
    HE_EXPECT_EQ(buf.Size(), sizeof(ObjDatas));
    HE_EXPECT_EQ_MEM(buf.Data(), &ObjDatas, buf.Size());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, BufferWriter, WriteRepeat)
{
    Allocator& a = CrtAllocator::Get();

    {
        uint8_t zeroes[32]{};

        BufferWriter buf(a);
        buf.WriteRepeat(0, HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ_MEM(buf.Data(), zeroes, buf.Size());
    }

    {
        uint8_t fives[64]{};
        MemSet(fives, 0x55, HE_LENGTH_OF(fives));

        BufferWriter buf(a);
        buf.WriteRepeat(0x55, HE_LENGTH_OF(fives));
        HE_EXPECT_EQ(buf.Size(), HE_LENGTH_OF(fives));
        HE_EXPECT_EQ_MEM(buf.Data(), fives, buf.Size());
    }
}

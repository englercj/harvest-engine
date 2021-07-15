// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/vector.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Constants)
{
    // Changing these are potentially breaking so checking them here so a change is made with thoughtfulness.
    static_assert(IsSame<Vector<int>::ElementType, int>);
    static_assert(IsSame<Vector<NonTrivial>::ElementType, NonTrivial>);
    static_assert(Vector<int>::MinElements == 8);
    static_assert(Vector<int>::MaxElements == 0xffffffff);
}
// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Construct)
{
    Allocator& a = CrtAllocator::Get();

    {
        Vector<int> v(a);
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<NonTrivial> v(a);
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Construct_Copy)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint32_t ExpectedSize = 10;

    Vector<int> v(a);
    v.Resize(ExpectedSize, 12345);
    HE_EXPECT_EQ(v.Size(), ExpectedSize);

    {
        Vector<int> copy(a, v);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);
    }

    {
        AnotherAllocator a2;
        Vector<int> copy(a2, v);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    {
        Vector<int> copy(v);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v.GetAllocator());
    }

    {
        Vector<int> copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v.GetAllocator());
    }

    Vector<NonTrivial> v2(a);
    v2.Resize(ExpectedSize);
    HE_EXPECT_EQ(v2.Size(), ExpectedSize);

    {
        Vector<NonTrivial> copy(a, v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v2.Data(), v2.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);
    }

    {
        AnotherAllocator a2;
        Vector<NonTrivial> copy(a2, v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v2.Data(), v2.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    {
        Vector<NonTrivial> copy(v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v2.Data(), v2.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v2.GetAllocator());
    }

    {
        Vector<NonTrivial> copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v2.Data(), v2.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v2.GetAllocator());
    }

    Vector<CopyOnly> v3(a);
    v3.Resize(ExpectedSize);
    HE_EXPECT_EQ(v3.Size(), ExpectedSize);

    {
        Vector<CopyOnly> copy(a, v3);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        Vector<CopyOnly> copy(a2, v3);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> copy(v3);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v3.GetAllocator());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v3.GetAllocator());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Construct_Move)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint32_t ExpectedSize = 10;

    {
        Vector<int> v(a);
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved(a, Move(v));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v(a);
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<int> moved(a2, Move(v));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v(a);
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved(Move(v));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v(a);
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<NonTrivial> v2(a);
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<NonTrivial> moved(a, Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<NonTrivial> v2(a);
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<NonTrivial> moved(a2, Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<NonTrivial> v2(a);
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<NonTrivial> moved(Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<NonTrivial> v2(a);
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<NonTrivial> moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyOnly> v3(a);
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        Vector<CopyOnly> moved(a, Move(v3));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> v3(a);
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<CopyOnly> moved(a2, Move(v3));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> v3(a);
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        Vector<CopyOnly> moved(Move(v3));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v3.GetAllocator());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> v3(a);
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        Vector<CopyOnly> moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v3.GetAllocator());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<MoveOnly> v4(a);
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        Vector<MoveOnly> moved(a, Move(v4));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }

    {
        Vector<MoveOnly> v4(a);
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<MoveOnly> moved(a2, Move(v4));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }

    {
        Vector<MoveOnly> v4(a);
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        Vector<MoveOnly> moved(Move(v4));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v4.GetAllocator());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }

    {
        Vector<MoveOnly> v4(a);
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        Vector<MoveOnly> moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v4.GetAllocator());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, operator_assign_copy)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint32_t ExpectedSize = 10;

    Vector<int> v(a);
    v.Resize(ExpectedSize, 12345);
    HE_EXPECT_EQ(v.Size(), ExpectedSize);

    {
        Vector<int> copy(a);
        copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);
    }

    {
        AnotherAllocator a2;
        Vector<int> copy(a2);
        copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    Vector<NonTrivial> v2(a);
    v2.Resize(ExpectedSize);
    HE_EXPECT_EQ(v2.Size(), ExpectedSize);

    {
        Vector<NonTrivial> copy(a);
        copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v2.Data(), v2.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);
    }

    {
        AnotherAllocator a2;
        Vector<NonTrivial> copy(a2);
        copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v2.Data(), v2.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    Vector<CopyOnly> v3(a);
    v3.Resize(ExpectedSize);
    HE_EXPECT_EQ(v3.Size(), ExpectedSize);

    {
        Vector<CopyOnly> copy(a);
        copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        Vector<CopyOnly> copy(a2);
        copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, operator_assign_move)
{
    Allocator& a = CrtAllocator::Get();

    constexpr uint32_t ExpectedSize = 10;

    {
        Vector<int> v(a);
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved(a);
        moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v(a);
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<int> moved(a2);
        moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<NonTrivial> v2(a);
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<NonTrivial> moved(a);
        moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<NonTrivial> v2(a);
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<NonTrivial> moved(a2);
        moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyOnly> v3(a);
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        Vector<CopyOnly> moved(a);
        moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> v3(a);
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<CopyOnly> moved(a2);
        moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<MoveOnly> v4(a);
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        Vector<MoveOnly> moved(a);
        moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a);
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }

    {
        Vector<MoveOnly> v4(a);
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<MoveOnly> moved(a2);
        moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, operator_index)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    v.Resize(10, 12345);
    HE_EXPECT_EQ(v[0], 12345);
    HE_EXPECT_EQ(v[3], 12345);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, IsEmpty)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT(v.IsEmpty());

    v.Resize(10);
    HE_EXPECT(!v.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Capacity)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT_EQ(v.Capacity(), 0);

    v.Resize(1);
    HE_EXPECT_EQ(v.Capacity(), Vector<int>::MinElements);

    v.Resize(Vector<int>::MinElements * 2);
    HE_EXPECT_GE(v.Capacity(), Vector<int>::MinElements * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Size)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT_EQ(v.Size(), 0);

    v.Resize(1);
    HE_EXPECT_EQ(v.Size(), 1);

    v.Resize(Vector<int>::MinElements * 2);
    HE_EXPECT_EQ(v.Size(), Vector<int>::MinElements * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Reserve)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT_EQ(v.Capacity(), 0);

    v.Reserve(1);
    HE_EXPECT_EQ(v.Capacity(), Vector<int>::MinElements);

    v.Reserve(Vector<int>::MinElements * 2);
    HE_EXPECT_GE(v.Capacity(), Vector<int>::MinElements * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Resize)
{
    Allocator& a = CrtAllocator::Get();

    uint8_t zeroes[16]{};

    {
        Vector<uint8_t> v(a);
        v.Resize(HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ_MEM(v.Data(), zeroes, v.Size());
    }

    {
        Vector<uint8_t> v(a);
        v.Resize(HE_LENGTH_OF(zeroes), DefaultInit);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(zeroes));
    }

    {
        static bool s_constructed = false;

        struct TestObj { TestObj() { s_constructed = true; } };

        Vector<TestObj> v(a);
        v.Resize(1);
        HE_EXPECT(s_constructed);
        HE_EXPECT_EQ(v.Size(), 1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, ShrinkToFit)
{
    Allocator& a = CrtAllocator::Get();

    Vector<uint8_t> v(a);
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_EQ(v.Capacity(), 0);

    v.Reserve(16);
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_GE(v.Capacity(), 16);

    v.ShrinkToFit();
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_GE(v.Capacity(), 0);

    v.Resize(32);
    HE_EXPECT_EQ(v.Size(), 32);
    HE_EXPECT_GE(v.Capacity(), 32);

    v.ShrinkToFit();
    HE_EXPECT_EQ(v.Size(), 32);
    HE_EXPECT_GE(v.Capacity(), 32);

    v.Clear();
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_GE(v.Capacity(), 32);

    v.ShrinkToFit();
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_GE(v.Capacity(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Data)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT(!v.Data());

    v.Resize(16);
    HE_EXPECT_EQ_PTR(v.Data(), VectorTestAttorney::GetPtr(v));

    v.Clear();
    HE_EXPECT_EQ_PTR(v.Data(), VectorTestAttorney::GetPtr(v));

    v.ShrinkToFit();
    HE_EXPECT(!v.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Begin)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT(!v.Begin());

    v.Resize(1);
    HE_EXPECT_EQ_PTR(v.Begin(), VectorTestAttorney::GetPtr(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, End)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT(!v.End());

    v.Resize(1);
    HE_EXPECT_EQ_PTR(v.End(), VectorTestAttorney::GetPtr(v) + 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Clear)
{
    Allocator& a = CrtAllocator::Get();

    {
        Vector<int> v(a);
        HE_EXPECT(v.IsEmpty());

        v.Clear();
        HE_EXPECT(v.IsEmpty());

        v.Reserve(16);
        HE_EXPECT(v.IsEmpty());

        v.Resize(10);
        HE_EXPECT(!v.IsEmpty());

        v.Clear();
        HE_EXPECT(v.IsEmpty());
    }

    {
        static bool s_destructed = false;

        struct TestObj { ~TestObj() { s_destructed = true; } };

        Vector<TestObj> v(a);
        v.Resize(1);
        HE_EXPECT(!s_destructed);
        HE_EXPECT_EQ(v.Size(), 1);

        v.Clear();
        HE_EXPECT(s_destructed);
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Insert)
{
    Allocator& a = CrtAllocator::Get();

    Vector<uint8_t> v(a);
    HE_EXPECT_EQ(v.Size(), 0);

    {
        const uint8_t expected[]{ 1 };

        v.Insert(0, 1);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
        HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());
    }

    {
        const uint8_t expected[]{ 1, 3 };

        v.Insert(1, 3);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
        HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());
    }

    {
        const uint8_t expected[]{ 1, 2, 3 };

        v.Insert(1, 2);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
        HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());
    }

    {
        const uint8_t expected[]{ 1, 2, 60, 3 };

        v.Insert(2, 60);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
        HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());
    }

    {
        const uint8_t insert[]{ 9, 8, 7 };
        const uint8_t expected[]{ 1, 2, 9, 8, 7, 60, 3 };

        v.Insert(2, insert, insert + HE_LENGTH_OF(insert));
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
        HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, Erase)
{
    Allocator& a = CrtAllocator::Get();

    const uint8_t expected[]{ 1, 2, 60, 3 };

    Vector<uint8_t> v(a);

    v.Insert(0, expected, expected + HE_LENGTH_OF(expected));
    HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
    HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());

    v.Erase(0, 1);
    HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected) - 1);
    HE_EXPECT_EQ_MEM(v.Data(), expected + 1, v.Size());

    {
        const uint8_t expected2[]{ 2, 3 };

        v.Erase(1, 1);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected2));
        HE_EXPECT_EQ_MEM(v.Data(), expected2, v.Size());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, PushBack)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    HE_EXPECT_EQ(v.Size(), 0);

    v.PushBack(25);
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(*v.Data(), 25);

    v.PushBack(50);
    HE_EXPECT_EQ(v.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, PopBack)
{
    Allocator& a = CrtAllocator::Get();

    Vector<int> v(a);
    v.PushBack(10);
    v.PushBack(20);
    HE_EXPECT_EQ(v.Size(), 2);
    HE_EXPECT_EQ(v[0], 10);
    HE_EXPECT_EQ(v[1], 20);

    v.PopBack();
    HE_EXPECT_EQ(v.Size(), 1);

    v.PopBack();
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, Vector, EmplaceBack)
{
    Allocator& a = CrtAllocator::Get();

    {
        Vector<int> v(a);

        int value = v.EmplaceBack();
        HE_EXPECT_EQ(value, 0);
        HE_EXPECT_EQ(v.Size(), 1);

        value = v.EmplaceBack(12);
        HE_EXPECT_EQ(value, 12);
        HE_EXPECT_EQ(v.Size(), 2);

        value = v.EmplaceBack(DefaultInit);
        // HE_EXPECT_EQ(value, 0);
        HE_EXPECT_EQ(v.Size(), 3);
    }

    {
        struct TestObj { TestObj(int a, int b) : a(a), b(b) {} int a, b; };

        Vector<TestObj> v(a);

        TestObj& value0 = v.EmplaceBack(5, 6);
        HE_EXPECT_EQ(value0.a, 5);
        HE_EXPECT_EQ(value0.b, 6);
        HE_EXPECT_EQ(v.Size(), 1);

        // Does not compile
        //v.EmplaceBack(DefaultInit);

        // Does not compile
        //v.EmplaceBack();
    }
}

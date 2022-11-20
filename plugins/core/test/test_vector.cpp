// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/vector.h"

#include "he/core/test.h"

#include "fmt/ranges.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Constants)
{
    // Changing these are potentially breaking so checking them here so any changes are made with thoughtfulness.
    static_assert(std::is_same_v<Vector<int>::ElementType, int>);
    static_assert(std::is_same_v<Vector<CopyAndMove>::ElementType, CopyAndMove>);
    static_assert(Vector<int>::MinElements == 8);
    static_assert(Vector<int>::MaxElements == 0xffffffff);
}
// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Construct)
{
    {
        Vector<int> v;
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<CopyAndMove> v;
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Construct_Copy)
{
    constexpr uint32_t ExpectedSize = 10;

    Vector<int> v;
    v.Resize(ExpectedSize, 12345);
    HE_EXPECT_EQ(v.Size(), ExpectedSize);

    {
        Vector<int> copy(v);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        Vector<int> copy(v, a2);
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

    Vector<CopyAndMove> v2;
    v2.Resize(ExpectedSize);
    HE_EXPECT_EQ(v2.Size(), ExpectedSize);

    {
        Vector<CopyAndMove> copy(v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        Vector<CopyAndMove> copy(v2, a2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        Vector<CopyAndMove> copy(v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v2.GetAllocator());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        Vector<CopyAndMove> copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v2.GetAllocator());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    Vector<CopyOnly> v3;
    v3.Resize(ExpectedSize);
    HE_EXPECT_EQ(v3.Size(), ExpectedSize);

    {
        Vector<CopyOnly> copy(v3);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        Vector<CopyOnly> copy(v3, a2);
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
HE_TEST(core, vector, Construct_Move)
{
    constexpr uint32_t ExpectedSize = 10;

    {
        Vector<int> v;
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved(Move(v));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v;
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<int> moved(Move(v), a2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v;
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved(Move(v));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v;
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<CopyAndMove> v2;
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<CopyAndMove> moved(Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyAndMove> v2;
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<CopyAndMove> moved(Move(v2), a2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyAndMove> v2;
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<CopyAndMove> moved(Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyAndMove> v2;
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<CopyAndMove> moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyOnly> v3;
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        Vector<CopyOnly> moved(Move(v3));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> v3;
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<CopyOnly> moved(Move(v3), a2);
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
        Vector<CopyOnly> v3;
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
        Vector<CopyOnly> v3;
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
        Vector<MoveOnly> v4;
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        Vector<MoveOnly> moved(Move(v4));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }

    {
        Vector<MoveOnly> v4;
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<MoveOnly> moved(Move(v4), a2);
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
        Vector<MoveOnly> v4;
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
        Vector<MoveOnly> v4;
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
HE_TEST(core, vector, operator_assign_copy)
{
    constexpr uint32_t ExpectedSize = 10;

    Vector<int> v;
    v.Resize(ExpectedSize, 12345);
    HE_EXPECT_EQ(v.Size(), ExpectedSize);

    {
        Vector<int> copy;
        copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        Vector<int> copy(a2);
        copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    Vector<CopyAndMove> v2;
    v2.Resize(ExpectedSize);
    HE_EXPECT_EQ(v2.Size(), ExpectedSize);

    {
        Vector<CopyAndMove> copy;
        copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        Vector<CopyAndMove> copy(a2);
        copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(copy[i].copyConstructed);
            HE_EXPECT(!copy[i].copyAssigned);
        }
    }

    Vector<CopyOnly> v3;
    v3.Resize(ExpectedSize);
    HE_EXPECT_EQ(v3.Size(), ExpectedSize);

    {
        Vector<CopyOnly> copy;
        copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

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
HE_TEST(core, vector, operator_assign_range)
{
    Vector<int> v;
    v.PushBack(1);
    v.PushBack(2);
    v.PushBack(3);

    Vector<int> v2;
    v2.PushBack(5);
    v = v2;
    HE_EXPECT_EQ(v.Size(), v2.Size());
    HE_EXPECT_EQ(v, v2);

    const int v3Data[]{ 5, 4, 3 };
    Span<const int> v3(v3Data);
    v = v3;
    HE_EXPECT_EQ(v.Size(), v3.Size());
    HE_EXPECT_EQ_MEM(v.Data(), v3.Data(), v.Size() * sizeof(int));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, operator_assign_move)
{
    constexpr uint32_t ExpectedSize = 10;

    {
        Vector<int> v;
        v.Resize(ExpectedSize, 12345);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        Vector<int> moved;
        moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        Vector<int> v;
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
        Vector<CopyAndMove> v2;
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        Vector<CopyAndMove> moved;
        moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyAndMove> v2;
        v2.Resize(ExpectedSize);
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        AnotherAllocator a2;
        Vector<CopyAndMove> moved(a2);
        moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        Vector<CopyOnly> v3;
        v3.Resize(ExpectedSize);
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        Vector<CopyOnly> moved;
        moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].copyConstructed);
            HE_EXPECT(!moved[i].copyAssigned);
        }
    }

    {
        Vector<CopyOnly> v3;
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
        Vector<MoveOnly> v4;
        v4.Resize(ExpectedSize);
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        Vector<MoveOnly> moved;
        moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (uint32_t i = 0; i < ExpectedSize; ++i)
        {
            HE_EXPECT(!moved[i].moveConstructed);
            HE_EXPECT(!moved[i].moveAssigned);
        }
    }

    {
        Vector<MoveOnly> v4;
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
HE_TEST(core, vector, operator_index)
{
    Vector<int> v;
    v.Resize(10, 12345);
    HE_EXPECT_EQ(v[0], 12345);
    HE_EXPECT_EQ(v[3], 12345);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, operator_eq)
{
    Vector<int> v;
    v.Resize(10, 12345);

    Vector<int> v2;
    v2.Resize(10, 12345);

    HE_EXPECT(v == v2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, operator_ne)
{
    Vector<int> v;
    v.Resize(10, 12345);

    Vector<int> v2;
    v2.Resize(11, 12345);

    HE_EXPECT(v != v2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, IsEmpty)
{
    Vector<int> v;
    HE_EXPECT(v.IsEmpty());

    v.Resize(10);
    HE_EXPECT(!v.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Capacity)
{
    Vector<int> v;
    HE_EXPECT_EQ(v.Capacity(), 0);

    v.Resize(1);
    HE_EXPECT_EQ(v.Capacity(), Vector<int>::MinElements);

    v.Resize(Vector<int>::MinElements * 2);
    HE_EXPECT_GE(v.Capacity(), Vector<int>::MinElements * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Size)
{
    Vector<int> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.Resize(1);
    HE_EXPECT_EQ(v.Size(), 1);

    v.Resize(Vector<int>::MinElements * 2);
    HE_EXPECT_EQ(v.Size(), Vector<int>::MinElements * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Reserve)
{
    Vector<int> v;
    HE_EXPECT_EQ(v.Capacity(), 0);

    v.Reserve(1);
    HE_EXPECT_EQ(v.Capacity(), Vector<int>::MinElements);

    v.Reserve(Vector<int>::MinElements * 2);
    HE_EXPECT_GE(v.Capacity(), Vector<int>::MinElements * 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Resize)
{
    uint8_t zeroes[16]{};

    {
        Vector<uint8_t> v;
        v.Resize(HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ_MEM(v.Data(), zeroes, v.Size());
    }

    {
        Vector<uint8_t> v;
        v.Resize(HE_LENGTH_OF(zeroes), DefaultInit);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(zeroes));
    }

    {
        static bool s_constructed = false;

        struct TestObj { TestObj() { s_constructed = true; } };

        Vector<TestObj> v;
        v.Resize(1);
        HE_EXPECT(s_constructed);
        HE_EXPECT_EQ(v.Size(), 1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Expand)
{
    uint8_t zeroes[16]{};

    {
        Vector<uint8_t> v;
        v.Expand(HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(zeroes));
        HE_EXPECT_EQ_MEM(v.Data(), zeroes, v.Size());
    }

    {
        Vector<uint8_t> v;
        v.Expand(HE_LENGTH_OF(zeroes), DefaultInit);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(zeroes));
    }

    {
        static int s_constructed = 0;

        struct TestObj { TestObj() { ++s_constructed; } };

        Vector<TestObj> v;
        v.Expand(1);
        HE_EXPECT_EQ(s_constructed, 1);
        HE_EXPECT_EQ(v.Size(), 1);
        v.Expand(2);
        HE_EXPECT_EQ(s_constructed, 3);
        HE_EXPECT_EQ(v.Size(), 3);
        v.Expand(24);
        HE_EXPECT_EQ(s_constructed, 27);
        HE_EXPECT_EQ(v.Size(), 27);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, ShrinkToFit)
{
    Vector<uint8_t> v;
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_EQ(v.Capacity(), 0);

    v.Reserve(16);
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_GE(v.Capacity(), 16);

    v.ShrinkToFit();
    HE_EXPECT_EQ(v.Size(), 0);
    // HE_EXPECT_GE(v.Capacity(), 0);

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
    // HE_EXPECT_GE(v.Capacity(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Data)
{
    Vector<int> v;
    HE_EXPECT(!v.Data());

    v.Resize(16);
    HE_EXPECT_EQ_PTR(v.Data(), VectorTestAttorney::GetPtr(v));

    v.Clear();
    HE_EXPECT_EQ_PTR(v.Data(), VectorTestAttorney::GetPtr(v));

    v.ShrinkToFit();
    HE_EXPECT(!v.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Adopt_Release)
{
    static int s_destructed = 0;

    struct TestObj { ~TestObj() { ++s_destructed; } };

    const TestObj* Null = nullptr;

    {
        Vector<TestObj> v;
        v.EmplaceBack();

        const TestObj* data = v.Data();
        const uint32_t size = v.Size();
        const uint32_t cap = v.Capacity();

        HE_EXPECT_NE_PTR(data, Null);
        HE_EXPECT_EQ(size, 1);
        HE_EXPECT_EQ(cap, Vector<TestObj>::MinElements);
        HE_EXPECT_EQ(s_destructed, 0);

        TestObj* p = v.Release();

        HE_EXPECT_EQ_PTR(data, p);
        HE_EXPECT_EQ_PTR(v.Data(), Null);
        HE_EXPECT_EQ(v.Size(), 0);
        HE_EXPECT_EQ(v.Capacity(), 0);
        HE_EXPECT_EQ(s_destructed, 0);

        v.EmplaceBack();

        HE_EXPECT_NE_PTR(v.Data(), Null);
        HE_EXPECT_NE_PTR(v.Data(), p);
        HE_EXPECT_EQ(v.Size(), 1);
        HE_EXPECT_EQ(v.Capacity(), Vector<TestObj>::MinElements);
        HE_EXPECT_EQ(s_destructed, 0);

        v.Adopt(p, size, cap);

        HE_EXPECT_NE_PTR(v.Data(), Null);
        HE_EXPECT_EQ_PTR(v.Data(), p);
        HE_EXPECT_EQ(v.Size(), size);
        HE_EXPECT_EQ(v.Capacity(), cap);
        HE_EXPECT_EQ(s_destructed, 1);
    }

    HE_EXPECT_EQ(s_destructed, 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Front_Back)
{
    Vector<int> v;
    v.PushBack(1);
    v.PushBack(2);
    v.PushBack(3);
    HE_EXPECT_EQ(v.Front(), 1);
    HE_EXPECT_EQ(v.Back(), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Begin)
{
    Vector<int> v;
    HE_EXPECT(!v.Begin());

    v.Resize(1);
    HE_EXPECT_EQ_PTR(v.Begin(), VectorTestAttorney::GetPtr(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, End)
{
    Vector<int> v;
    HE_EXPECT(!v.End());

    v.Resize(1);
    HE_EXPECT_EQ_PTR(v.End(), VectorTestAttorney::GetPtr(v) + 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, RangeBasedFor)
{
    uint8_t values[]{ 1, 2, 3, 4, 5, 6, 7, 8 };

    Vector<uint8_t> v;
    v.Insert(0, values, values + HE_LENGTH_OF(values));

    HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(values));

    uint32_t i = 0;
    for (uint8_t b : v)
    {
        HE_EXPECT_EQ(b, values[i++]);
    }
    HE_EXPECT_EQ(i, HE_LENGTH_OF(values));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Clear)
{
    {
        Vector<int> v;
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

        Vector<TestObj> v;
        v.Resize(1);
        HE_EXPECT(!s_destructed);
        HE_EXPECT_EQ(v.Size(), 1);

        v.Clear();
        HE_EXPECT(s_destructed);
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, Insert)
{
    Vector<uint8_t> v;
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
HE_TEST(core, vector, Erase_Trivial)
{
    const uint8_t expected[]{ 1, 2, 60, 3 };

    Vector<uint8_t> v;

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
HE_TEST(core, vector, Erase_NonTrivial)
{
    static int s_destructed = 0;

    struct TestObj
    {
        ~TestObj() noexcept { ++s_destructed; }
        TestObj() = default;
        TestObj(const TestObj&) noexcept { copyConstructed = true; }
        TestObj& operator=(const TestObj&) noexcept { copyAssigned = true; return *this; }
        TestObj(TestObj&&) noexcept { moveConstructed = true; }
        TestObj& operator=(TestObj&&) noexcept { moveAssigned = true; return *this; }

        bool copyConstructed{ false };
        bool copyAssigned{ false };
        bool moveConstructed{ false };
        bool moveAssigned{ false };
    };

    // Case 1: size = 5, index = 1, count = 2 (two middle, two moves, two destruct)
    {
        Vector<TestObj> v;
        v.Resize(5);
        v.Erase(1, 2);
        HE_EXPECT_EQ(s_destructed, 2);
        HE_EXPECT_EQ(v.Size(), 3);
        HE_EXPECT(!v[0].copyConstructed && !v[0].copyAssigned && !v[0].moveConstructed && !v[0].moveAssigned);
        HE_EXPECT(!v[1].copyConstructed && !v[1].copyAssigned && !v[1].moveConstructed && v[1].moveAssigned);
        HE_EXPECT(!v[2].copyConstructed && !v[2].copyAssigned && !v[2].moveConstructed && v[2].moveAssigned);
    }
    HE_EXPECT_EQ(s_destructed, 5);

    // Case 2: size = 5, index = 3, count = 2 (two trailing, zero moves, two destruct)
    {
        Vector<TestObj> v;
        v.Resize(5);
        v.Erase(3, 2);
        HE_EXPECT_EQ(s_destructed, 7);
        HE_EXPECT_EQ(v.Size(), 3);
        HE_EXPECT(!v[0].copyConstructed && !v[0].copyAssigned && !v[0].moveConstructed && !v[0].moveAssigned);
        HE_EXPECT(!v[1].copyConstructed && !v[1].copyAssigned && !v[1].moveConstructed && !v[1].moveAssigned);
        HE_EXPECT(!v[2].copyConstructed && !v[2].copyAssigned && !v[2].moveConstructed && !v[2].moveAssigned);
    }
    HE_EXPECT_EQ(s_destructed, 10);

    // Case 3: size = 5, index = 0, count = 3 (three start, two moves, three destruct)
    {
        Vector<TestObj> v;
        v.Resize(5);
        v.Erase(0, 3);
        HE_EXPECT_EQ(s_destructed, 13);
        HE_EXPECT_EQ(v.Size(), 2);
        HE_EXPECT(!v[0].copyConstructed && !v[0].copyAssigned && !v[0].moveConstructed && v[0].moveAssigned);
        HE_EXPECT(!v[1].copyConstructed && !v[1].copyAssigned && !v[1].moveConstructed && v[1].moveAssigned);
    }
    HE_EXPECT_EQ(s_destructed, 15);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, EraseUnordered)
{
    const uint8_t fixture[]{ 1, 2, 60, 3 };
    const uint8_t expected[]{ 3, 2, 60 };

    Vector<uint8_t> v;

    v.Insert(0, fixture, fixture + HE_LENGTH_OF(fixture));
    HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(fixture));
    HE_EXPECT_EQ_MEM(v.Data(), fixture, v.Size());

    v.EraseUnordered(0, 1);
    HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
    HE_EXPECT_EQ_MEM(v.Data(), expected, v.Size());

    {
        const uint8_t expected2[]{ 3, 60 };

        v.Erase(1, 1);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected2));
        HE_EXPECT_EQ_MEM(v.Data(), expected2, v.Size());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, EraseUnordered_NonTrivial)
{
    static int s_destructed = 0;

    struct TestObj
    {
        ~TestObj() noexcept { ++s_destructed; }
        TestObj() = default;
        TestObj(const TestObj&) noexcept { copyConstructed = true; }
        TestObj& operator=(const TestObj&) noexcept { copyAssigned = true; return *this; }
        TestObj(TestObj&&) noexcept { moveConstructed = true; }
        TestObj& operator=(TestObj&&) noexcept { moveAssigned = true; return *this; }

        bool copyConstructed{ false };
        bool copyAssigned{ false };
        bool moveConstructed{ false };
        bool moveAssigned{ false };
    };

    // Case 1: size = 5, index = 1, count = 2 (two middle, two moves, zero dtor)
    {
        Vector<TestObj> v;
        v.Resize(5);
        v.EraseUnordered(1, 2);
        HE_EXPECT_EQ(s_destructed, 2);
        HE_EXPECT_EQ(v.Size(), 3);
        HE_EXPECT(!v[0].copyConstructed && !v[0].copyAssigned && !v[0].moveConstructed && !v[0].moveAssigned);
        HE_EXPECT(!v[1].copyConstructed && !v[1].copyAssigned && !v[1].moveConstructed && v[1].moveAssigned);
        HE_EXPECT(!v[2].copyConstructed && !v[2].copyAssigned && !v[2].moveConstructed && v[2].moveAssigned);
    }
    HE_EXPECT_EQ(s_destructed, 5);

    // Case 2: size = 5, index = 3, count = 2 (two trailing, zero moves, two dtor)
    {
        Vector<TestObj> v;
        v.Resize(5);
        v.EraseUnordered(3, 2);
        HE_EXPECT_EQ(s_destructed, 7);
        HE_EXPECT_EQ(v.Size(), 3);
        HE_EXPECT(!v[0].copyConstructed && !v[0].copyAssigned && !v[0].moveConstructed && !v[0].moveAssigned);
        HE_EXPECT(!v[1].copyConstructed && !v[1].copyAssigned && !v[1].moveConstructed && !v[1].moveAssigned);
        HE_EXPECT(!v[2].copyConstructed && !v[2].copyAssigned && !v[2].moveConstructed && !v[2].moveAssigned);
    }
    HE_EXPECT_EQ(s_destructed, 10);

    // Case 3: size = 5, index = 0, count = 3 (three start, two moves, three dtor)
    {
        Vector<TestObj> v;
        v.Resize(5);
        v.EraseUnordered(0, 3);
        HE_EXPECT_EQ(s_destructed, 13);
        HE_EXPECT_EQ(v.Size(), 2);
        HE_EXPECT(!v[0].copyConstructed && !v[0].copyAssigned && !v[0].moveConstructed && v[0].moveAssigned);
        HE_EXPECT(!v[1].copyConstructed && !v[1].copyAssigned && !v[1].moveConstructed && v[1].moveAssigned);
    }
    HE_EXPECT_EQ(s_destructed, 15);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, PushBack)
{
    Vector<int> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.PushBack(25);
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(*v.Data(), 25);

    v.PushBack(50);
    HE_EXPECT_EQ(v.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, PushFront)
{
    Vector<int> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.PushFront(25);
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(*v.Data(), 25);

    v.PushFront(50);
    HE_EXPECT_EQ(v.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, PopBack)
{
    Vector<int> v;
    v.PushBack(10);
    v.PushBack(20);
    HE_EXPECT_EQ(v.Size(), 2);
    HE_EXPECT_EQ(v[0], 10);
    HE_EXPECT_EQ(v[1], 20);

    v.PopBack();
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(v[0], 10);

    v.PopBack();
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, PopFront)
{
    Vector<int> v;
    v.PushBack(10);
    v.PushBack(20);
    HE_EXPECT_EQ(v.Size(), 2);
    HE_EXPECT_EQ(v[0], 10);
    HE_EXPECT_EQ(v[1], 20);

    v.PopFront();
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(v[0], 20);

    v.PopFront();
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, vector, EmplaceBack)
{
    {
        Vector<int> v;

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

        Vector<TestObj> v;

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

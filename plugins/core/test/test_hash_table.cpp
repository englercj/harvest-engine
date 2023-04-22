// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/hash_table.h"

#include "he/core/random.h"
#include "he/core/test.h"
#include "he/core/utils.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename T> struct _SetTraits : HashSetTraits<T, Hasher<T>> {};
template <typename K, typename V> struct _MapTraits : HashMapTraits<K, V, Hasher<K>> {};

static Random64 s_randHashCode;

struct HashableCopyOnly
{
    HashableCopyOnly() : hash(s_randHashCode.Next()) {}
    HashableCopyOnly(const HashableCopyOnly& x) noexcept { copyConstructed = true; hash = x.hash; }
    HashableCopyOnly& operator=(const HashableCopyOnly& x) noexcept { copyAssigned = true; hash = x.hash; return *this; }
    HashableCopyOnly(HashableCopyOnly&&) = delete;
    HashableCopyOnly& operator=(HashableCopyOnly&&) = delete;

    void Reset() { copyConstructed = copyAssigned = false; }

    [[nodiscard]] uint64_t HashCode() const noexcept { return hash; }

    bool operator==(const HashableCopyOnly& x) const { return hash == x.hash; }

    bool copyConstructed{ false };
    bool copyAssigned{ false };
    uint64_t hash{ 0 };
};

struct HashableMoveOnly
{
    HashableMoveOnly() : hash(s_randHashCode.Next()) {}
    HashableMoveOnly(const HashableMoveOnly&) = delete;
    HashableMoveOnly& operator=(const HashableMoveOnly&) = delete;
    HashableMoveOnly(HashableMoveOnly&& x) noexcept { moveConstructed = true; hash = Exchange(x.hash, s_randHashCode.Next()); }
    HashableMoveOnly& operator=(HashableMoveOnly&& x) noexcept { moveAssigned = true; hash = Exchange(x.hash, s_randHashCode.Next()); return *this; }

    void Reset() { moveConstructed = moveAssigned = false; }

    [[nodiscard]] uint64_t HashCode() const noexcept { return hash; }

    bool operator==(const HashableMoveOnly& x) const { return hash == x.hash; }

    bool moveConstructed{ false };
    bool moveAssigned{ false };
    uint64_t hash{ 0 };
};

struct HashableCopyAndMove
{
    HashableCopyAndMove() : hash(s_randHashCode.Next()) {}
    HashableCopyAndMove(const HashableCopyAndMove& x) noexcept { copyConstructed = true; hash = x.hash; }
    HashableCopyAndMove& operator=(const HashableCopyAndMove& x) noexcept { copyAssigned = true; hash = x.hash; return *this; }
    HashableCopyAndMove(HashableCopyAndMove&& x) noexcept { moveConstructed = true; hash = Exchange(x.hash, s_randHashCode.Next()); }
    HashableCopyAndMove& operator=(HashableCopyAndMove&& x) noexcept { moveAssigned = true; hash = Exchange(x.hash, s_randHashCode.Next()); return *this; }

    void Reset() { copyConstructed = copyAssigned = moveConstructed = moveAssigned = false; }

    [[nodiscard]] uint64_t HashCode() const noexcept { return hash; }

    bool operator==(const HashableCopyAndMove& x) const { return hash == x.hash; }

    bool copyConstructed{ false };
    bool copyAssigned{ false };
    bool moveConstructed{ false };
    bool moveAssigned{ false };
    uint64_t hash{ 0 };
};

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Construct)
{
    {
        HashTable<_SetTraits<int32_t>> v;
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v;
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Construct_Copy)
{
    constexpr uint32_t ExpectedSize = 10;

    HashTable<_SetTraits<int>> v;
    for (int i = 0; i < ExpectedSize; ++i)
        v.Emplace(i);
    HE_EXPECT_EQ(v.Size(), ExpectedSize);

    {
        HashTable<_SetTraits<int>> copy(v);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v.GetAllocator());
    }

    {
        AnotherAllocator a2;
        HashTable<_SetTraits<int>> copy(v, a2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    {
        HashTable<_SetTraits<int>> copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v.GetAllocator());
    }

    HashTable<_SetTraits<HashableCopyAndMove>> v2;
    for (int i = 0; i < ExpectedSize; ++i)
        v2.Emplace(HashableCopyAndMove{});
    HE_EXPECT_EQ(v2.Size(), ExpectedSize);

    {
        HashTable<_SetTraits<HashableCopyAndMove>> copy(v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (const HashableCopyAndMove& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyAndMove>> copy(v2, a2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (const HashableCopyAndMove& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> copy(v2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v2.GetAllocator());

        for (const HashableCopyAndMove& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v2.GetAllocator());

        for (const HashableCopyAndMove& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    HashTable<_SetTraits<HashableCopyOnly>> v3;
    for (int i = 0; i < ExpectedSize; ++i)
    {
        const HashableCopyOnly c{};
        v3.Emplace(c);
    }
    HE_EXPECT_EQ(v3.Size(), ExpectedSize);

    {
        HashTable<_SetTraits<HashableCopyOnly>> copy(v3);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (const HashableCopyOnly& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyOnly>> copy(v3, a2);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (const HashableCopyOnly& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> copy(v3);
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v3.GetAllocator());

        for (const HashableCopyOnly& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &v3.GetAllocator());

        for (const HashableCopyOnly& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Construct_Move)
{
    constexpr uint32_t ExpectedSize = 10;

    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < ExpectedSize; ++i)
            v.Emplace(i);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        HashTable<_SetTraits<int>> moved(Move(v));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < ExpectedSize; ++i)
            v.Emplace(i);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        AnotherAllocator a2;
        HashTable<_SetTraits<int>> moved(Move(v), a2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < ExpectedSize; ++i)
            v.Emplace(i);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        HashTable<_SetTraits<int>> moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v2;
        for (int i = 0; i < ExpectedSize; ++i)
            v2.Emplace(HashableCopyAndMove{});
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        HashTable<_SetTraits<HashableCopyAndMove>> moved(Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v2;
        for (int i = 0; i < ExpectedSize; ++i)
            v2.Emplace(HashableCopyAndMove{});
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyAndMove>> moved(Move(v2), a2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v2;
        for (int i = 0; i < ExpectedSize; ++i)
            v2.Emplace(HashableCopyAndMove{});
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        HashTable<_SetTraits<HashableCopyAndMove>> moved(Move(v2));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v2;
        for (int i = 0; i < ExpectedSize; ++i)
            v2.Emplace(HashableCopyAndMove{});
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        HashTable<_SetTraits<HashableCopyAndMove>> moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> v3;
        for (int i = 0; i < ExpectedSize; ++i)
        {
            const HashableCopyOnly c{};
            v3.Emplace(c);
        }
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        for (HashableCopyOnly& entry : v3)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableCopyOnly>> moved(Move(v3));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (const HashableCopyOnly& entry : moved)
        {
            HE_EXPECT(!entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> v3;
        for (int i = 0; i < ExpectedSize; ++i)
        {
            const HashableCopyOnly c{};
            v3.Emplace(c);
        }
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        for (HashableCopyOnly& entry : v3)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
            entry.Reset();
        }

        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyOnly>> moved(Move(v3), a2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (const HashableCopyOnly& entry : moved)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> v3;
        for (int i = 0; i < ExpectedSize; ++i)
        {
            const HashableCopyOnly c{};
            v3.Emplace(c);
        }
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        for (HashableCopyOnly& entry : v3)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableCopyOnly>> moved(Move(v3));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v3.GetAllocator());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (const HashableCopyOnly& entry : moved)
        {
            HE_EXPECT(!entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> v3;
        for (int i = 0; i < ExpectedSize; ++i)
        {
            const HashableCopyOnly c{};
            v3.Emplace(c);
        }
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        for (HashableCopyOnly& entry : v3)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableCopyOnly>> moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v3.GetAllocator());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (const HashableCopyOnly& entry : moved)
        {
            HE_EXPECT(!entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableMoveOnly>> v4;
        for (int i = 0; i < ExpectedSize; ++i)
            v4.Emplace(HashableMoveOnly{});
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        for (HashableMoveOnly& entry : v4)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableMoveOnly>> moved(Move(v4));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (const HashableMoveOnly& entry : moved)
        {
            HE_EXPECT(!entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableMoveOnly>> v4;
        for (int i = 0; i < ExpectedSize; ++i)
            v4.Emplace(HashableMoveOnly{});
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        for (HashableMoveOnly& entry : v4)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
            entry.Reset();
        }

        AnotherAllocator a2;
        HashTable<_SetTraits<HashableMoveOnly>> moved(Move(v4), a2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (const HashableMoveOnly& entry : moved)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableMoveOnly>> v4;
        for (int i = 0; i < ExpectedSize; ++i)
            v4.Emplace(HashableMoveOnly{});
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        for (HashableMoveOnly& entry : v4)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableMoveOnly>> moved(Move(v4));
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v4.GetAllocator());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (const HashableMoveOnly& entry : moved)
        {
            HE_EXPECT(!entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableMoveOnly>> v4;
        for (int i = 0; i < ExpectedSize; ++i)
            v4.Emplace(HashableMoveOnly{});
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        for (HashableMoveOnly& entry : v4)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableMoveOnly>> moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &v4.GetAllocator());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (const HashableMoveOnly& entry : moved)
        {
            HE_EXPECT(!entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, operator_assign_copy)
{
    constexpr uint32_t ExpectedSize = 10;

    HashTable<_SetTraits<int>> v;
    for (int i = 0; i < ExpectedSize; ++i)
        v.Emplace(i);
    HE_EXPECT_EQ(v.Size(), ExpectedSize);

    {
        HashTable<_SetTraits<int>> copy;
        copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        HashTable<_SetTraits<int>> copy(a2);
        copy = v;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_MEM(copy.Data(), v.Data(), v.Size() * sizeof(int));
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);
    }

    HashTable<_SetTraits<HashableCopyAndMove>> v2;
    for (int i = 0; i < ExpectedSize; ++i)
        v2.Emplace(HashableCopyAndMove{});
    HE_EXPECT_EQ(v2.Size(), ExpectedSize);

    {
        HashTable<_SetTraits<HashableCopyAndMove>> copy;
        copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (const HashableCopyAndMove& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyAndMove>> copy(a2);
        copy = v2;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (const HashableCopyAndMove& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    HashTable<_SetTraits<HashableCopyOnly>> v3;
    for (int i = 0; i < ExpectedSize; ++i)
    {
        const HashableCopyOnly c{};
        v3.Emplace(c);
    }
    HE_EXPECT_EQ(v3.Size(), ExpectedSize);

    {
        HashTable<_SetTraits<HashableCopyOnly>> copy;
        copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &Allocator::GetDefault());

        for (const HashableCopyOnly& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyOnly>> copy(a2);
        copy = v3;
        HE_EXPECT_EQ(copy.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&copy.GetAllocator(), &a2);

        for (const HashableCopyOnly& entry : copy)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, operator_assign_move)
{
    constexpr uint32_t ExpectedSize = 10;

    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < ExpectedSize; ++i)
            v.Emplace(i);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        HashTable<_SetTraits<int>> moved;
        moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < ExpectedSize; ++i)
            v.Emplace(i);
        HE_EXPECT_EQ(v.Size(), ExpectedSize);

        AnotherAllocator a2;
        HashTable<_SetTraits<int>> moved(a2);
        moved = Move(v);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v.Data());
        HE_EXPECT_EQ(v.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v2;
        for (int i = 0; i < ExpectedSize; ++i)
            v2.Emplace(HashableCopyAndMove{});
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        HashTable<_SetTraits<HashableCopyAndMove>> moved;
        moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyAndMove>> v2;
        for (int i = 0; i < ExpectedSize; ++i)
            v2.Emplace(HashableCopyAndMove{});
        HE_EXPECT_EQ(v2.Size(), ExpectedSize);

        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyAndMove>> moved(a2);
        moved = Move(v2);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT(moved.Data());
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v2.Data());
        HE_EXPECT_EQ(v2.Size(), 0);
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> v3;
        for (int i = 0; i < ExpectedSize; ++i)
        {
            const HashableCopyOnly c{};
            v3.Emplace(c);
        }
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        for (HashableCopyOnly& entry : v3)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableCopyOnly>> moved;
        moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (const HashableCopyOnly& entry : moved)
        {
            HE_EXPECT(!entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableCopyOnly>> v3;
        for (int i = 0; i < ExpectedSize; ++i)
        {
            const HashableCopyOnly c{};
            v3.Emplace(c);
        }
        HE_EXPECT_EQ(v3.Size(), ExpectedSize);

        for (HashableCopyOnly& entry : v3)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
            entry.Reset();
        }

        AnotherAllocator a2;
        HashTable<_SetTraits<HashableCopyOnly>> moved(a2);
        moved = Move(v3);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v3.Data());
        HE_EXPECT_EQ(v3.Size(), 0);

        for (const HashableCopyOnly& entry : moved)
        {
            HE_EXPECT(entry.copyConstructed);
            HE_EXPECT(!entry.copyAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableMoveOnly>> v4;
        for (int i = 0; i < ExpectedSize; ++i)
            v4.Emplace(HashableMoveOnly{});
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        for (HashableMoveOnly& entry : v4)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
            entry.Reset();
        }

        HashTable<_SetTraits<HashableMoveOnly>> moved;
        moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT(!v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (const HashableMoveOnly& entry : moved)
        {
            HE_EXPECT(!entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
        }
    }

    {
        HashTable<_SetTraits<HashableMoveOnly>> v4;
        for (int i = 0; i < ExpectedSize; ++i)
            v4.Emplace(HashableMoveOnly{});
        HE_EXPECT_EQ(v4.Size(), ExpectedSize);

        for (HashableMoveOnly& entry : v4)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
            entry.Reset();
        }

        AnotherAllocator a2;
        HashTable<_SetTraits<HashableMoveOnly>> moved(a2);
        moved = Move(v4);
        HE_EXPECT_EQ(moved.Size(), ExpectedSize);
        HE_EXPECT_EQ_PTR(&moved.GetAllocator(), &a2);
        HE_EXPECT(v4.Data());
        HE_EXPECT_EQ(v4.Size(), 0);

        for (const HashableMoveOnly& entry : moved)
        {
            HE_EXPECT(entry.moveConstructed);
            HE_EXPECT(!entry.moveAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, operator_eq)
{
    HashTable<_SetTraits<int>> v;
    for (int i = 0; i < 10; ++i)
        v.Emplace(i);

    HashTable<_SetTraits<int>> v2;
    for (int i = 0; i < 10; ++i)
        v2.Emplace(i);

    HE_EXPECT(v == v2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, operator_ne)
{
    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < 10; ++i)
            v.Emplace(i);

        HashTable<_SetTraits<int>> v2;
        for (int i = 0; i < 15; ++i)
            v2.Emplace(i);

        HE_EXPECT_NE(v.Size(), v2.Size());
        HE_EXPECT(v != v2);
    }

    {
        HashTable<_SetTraits<int>> v;
        for (int i = 0; i < 10; ++i)
            v.Emplace(i);

        HashTable<_SetTraits<int>> v2;
        for (int i = 10; i < 20; ++i)
            v2.Emplace(i);

        HE_EXPECT_EQ(v.Size(), v2.Size());
        HE_EXPECT(v != v2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, IsEmpty)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT(v.IsEmpty());

    v.Emplace(10);
    HE_EXPECT(!v.IsEmpty());

    v.Clear();
    HE_EXPECT(v.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Size)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.Emplace(10);
    HE_EXPECT_EQ(v.Size(), 1);

    v.Clear();
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, LoadFactor)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT_EQ(v.LoadFactor(), 0.0f);

    v.Emplace(10);
    HE_EXPECT_EQ(v.LoadFactor(), 0.0625f);

    for (int i = 0; i < 100; ++i)
        v.Emplace(i);
    HE_EXPECT_EQ(v.LoadFactor(), 0.78125f);

    v.Clear();
    HE_EXPECT_EQ(v.LoadFactor(), 0.0f);

    const float maxLoad = v.MaxLoadFactor();
    for (int i = 0; i < 1024; ++i)
    {
        HE_EXPECT_LT(v.LoadFactor(), maxLoad);
        v.Emplace(i);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Reserve)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT_EQ(v.LoadFactor(), 0.0f);

    v.Emplace(10);
    HE_EXPECT_EQ(v.LoadFactor(), 0.0625f);

    v.Reserve(2048);
    HE_EXPECT_EQ(v.LoadFactor(), 0.000244140625f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, ShrinkToFit)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT_EQ(v.LoadFactor(), 0.0f);

    v.Emplace(10);
    HE_EXPECT_EQ(v.LoadFactor(), 0.0625f);

    v.Reserve(2048);
    HE_EXPECT_EQ(v.LoadFactor(), 0.000244140625f);

    v.ShrinkToFit();
    HE_EXPECT_EQ(v.LoadFactor(), 0.125f);

    v.Clear();
    HE_EXPECT_EQ(v.LoadFactor(), 0.0f);

    v.ShrinkToFit();
    HE_EXPECT_EQ(v.LoadFactor(), 0.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Data)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT(v.Data() == nullptr);

    v.Emplace(16);
    HE_EXPECT(v.Data() != nullptr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Find)
{
    HashTable<_SetTraits<int>> v;

    int* p = v.Find(1);
    HE_EXPECT(p == nullptr);

    v.Emplace(1);
    p = v.Find(1);
    HE_EXPECT(p != nullptr);
    HE_EXPECT_EQ(*p, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Contains)
{
    HashTable<_SetTraits<int>> v;

    HE_EXPECT(!v.Contains(1));

    v.Emplace(1);
    HE_EXPECT(v.Contains(1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Get)
{
    HashTable<_SetTraits<int>> v;

    HE_EXPECT_ASSERT({
        // this will assert because it does not exist
        int& i = v.Get(1);
        HE_UNUSED(i);
    });

    // this should not assert
    v.Emplace(1);
    int& j = v.Get(1);
    HE_EXPECT_EQ(j, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Adopt_Release)
{
    static int s_destructed = 0;

    struct TestObj
    {
        TestObj() : h(s_randHashCode.Next()) {}
        ~TestObj() { ++s_destructed; }
        uint64_t HashCode() const { return h; }
        bool operator==(const TestObj& x) const { return h == x.h; }
        uint64_t h;
    };

    const TestObj* Null = nullptr;

    {
        HashTable<_SetTraits<TestObj>> v;
        v.Emplace(TestObj{});

        const TestObj* data = v.Data();
        const uint32_t size = v.Size();

        HE_EXPECT_NE_PTR(data, Null);
        HE_EXPECT_EQ(size, 1);
        HE_EXPECT_EQ(s_destructed, 1); // the one moved into the table

        Vector<TestObj> p = v.Release();

        HE_EXPECT_EQ_PTR(data, p.Data());
        HE_EXPECT_EQ_PTR(v.Data(), Null);
        HE_EXPECT_EQ(v.Size(), 0);
        HE_EXPECT_EQ(s_destructed, 1);

        v.Emplace(TestObj{});

        HE_EXPECT_NE_PTR(v.Data(), Null);
        HE_EXPECT_NE_PTR(v.Data(), p.Data());
        HE_EXPECT_EQ(v.Size(), 1);
        HE_EXPECT_EQ(s_destructed, 2); // another one moved into the table

        v.Adopt(Move(p));

        HE_EXPECT_NE_PTR(v.Data(), Null);
        HE_EXPECT_EQ_PTR(v.Data(), data);
        HE_EXPECT_EQ(v.Size(), size);
        HE_EXPECT_EQ(s_destructed, 3); // the one living in the map before Adopt
    }

    HE_EXPECT_EQ(s_destructed, 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Begin_End)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT(!v.Begin());
    HE_EXPECT(!v.End());
    HE_EXPECT(v.Begin() == v.End());

    v.Emplace(1);
    HE_EXPECT(v.Begin());
    HE_EXPECT(v.End());
    HE_EXPECT(v.Begin() != v.End());
    HE_EXPECT_EQ_PTR(v.Begin(), HashTableTestAttorney::GetVector(v).Begin());
    HE_EXPECT_EQ_PTR(v.End(), HashTableTestAttorney::GetVector(v).End());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, RangeBasedFor)
{
    uint8_t values[]{ 1, 2, 3, 4, 5, 6, 7, 8 };

    HashTable<_SetTraits<uint8_t>> v;
    for (uint32_t i = 0; i < HE_LENGTH_OF(values); ++i)
        v.Emplace(values[i]);

    HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(values));

    uint32_t i = 0;
    for (uint8_t b : v)
    {
        HE_EXPECT_EQ(b, values[i++]);
    }
    HE_EXPECT_EQ(i, HE_LENGTH_OF(values));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Clear)
{
    {
        HashTable<_SetTraits<int>> v;
        HE_EXPECT(v.IsEmpty());

        v.Clear();
        HE_EXPECT(v.IsEmpty());

        v.Reserve(16);
        HE_EXPECT(v.IsEmpty());

        v.Emplace(10);
        HE_EXPECT(!v.IsEmpty());

        v.Clear();
        HE_EXPECT(v.IsEmpty());
    }

    {
        static uint32_t s_destructed = 0;

        struct TestObj
        {
            TestObj() : h(s_randHashCode.Next()) {}
            ~TestObj() { ++s_destructed; }
            uint64_t HashCode() const { return h; }
            bool operator==(const TestObj& x) const { return h == x.h; }
            uint64_t h;
        };
        const TestObj value{};

        HashTable<_SetTraits<TestObj>> v;
        v.Emplace(value);
        HE_EXPECT_EQ(s_destructed, 0);
        HE_EXPECT_EQ(v.Size(), 1);

        v.Clear();
        HE_EXPECT_EQ(s_destructed, 1);
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Erase)
{
    {
        const int expected[]{ 1, 2, 60, 3 };

        HashTable<_SetTraits<int>> v;
        for (uint32_t i = 0; i < HE_LENGTH_OF(expected); ++i)
            v.Emplace(expected[i]);

        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected));
        for (int i : expected)
        {
            HE_EXPECT(v.Contains(i));
        }

        v.Erase(1);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected) - 1);
        for (int i : expected)
        {
            if (i == 1)
            {
                HE_EXPECT(!v.Contains(i));
            }
            else
            {
                HE_EXPECT(v.Contains(i));
            }
        }

        v.Erase(60);
        HE_EXPECT_EQ(v.Size(), HE_LENGTH_OF(expected) - 2);
        for (int i : expected)
        {
            if (i == 1 || i == 60)
            {
                HE_EXPECT(!v.Contains(i));
            }
            else
            {
                HE_EXPECT(v.Contains(i));
            }
        }
    }

    {
        static uint32_t s_destructed = 0;

        struct TestObj
        {
            TestObj() : h(s_randHashCode.Next()) {}
            ~TestObj() { ++s_destructed; }
            uint64_t HashCode() const { return h; }
            bool operator==(const TestObj& x) const { return h == x.h; }
            uint64_t h;
        };
        const TestObj value{};

        HashTable<_SetTraits<TestObj>> v;
        v.Emplace(value);
        HE_EXPECT_EQ(s_destructed, 0);
        HE_EXPECT_EQ(v.Size(), 1);

        v.Erase(value);
        HE_EXPECT_EQ(s_destructed, 1);
        HE_EXPECT_EQ(v.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, Emplace)
{
    HashTable<_SetTraits<int>> v;
    HE_EXPECT_EQ(v.Size(), 0);

    {
        auto result = v.Emplace(25);
        HE_EXPECT(result.inserted);
        HE_EXPECT_EQ(result.entry, 25);
        HE_EXPECT_EQ(v.Size(), 1);
        HE_EXPECT_EQ(*v.Data(), 25);
    }

    {
        auto result = v.Emplace(50);
        HE_EXPECT(result.inserted);
        HE_EXPECT_EQ(result.entry, 50);
        HE_EXPECT_EQ(v.Size(), 2);
    }

    {
        auto result = v.Emplace(25);
        HE_EXPECT(!result.inserted);
        HE_EXPECT_EQ(result.entry, 25);
        HE_EXPECT_EQ(v.Size(), 2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashMap_Construct)
{
    HashMap<int, uint32_t> v;
    HE_EXPECT(v.IsEmpty());
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashMap_operator_index)
{
    HashMap<int, uint32_t> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v[1] = 500;
    HE_EXPECT_EQ(v[1], 500);
    HE_EXPECT_EQ(v.Size(), 1);

    v[1] = 256;
    HE_EXPECT_EQ(v[1], 256);
    HE_EXPECT_EQ(v.Size(), 1);

    HE_EXPECT_EQ(v[10], 0);
    HE_EXPECT_EQ(v.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashMap_EmplaceOrAssign)
{
    HashMap<int, uint32_t> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.EmplaceOrAssign(1, 500);
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(v.Get(1), 500);

    v.EmplaceOrAssign(1, 100);
    HE_EXPECT_EQ(v.Size(), 1);
    HE_EXPECT_EQ(v.Get(1), 100);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashMap_Find)
{
    HashMap<int, uint32_t> v;

    uint32_t* p = v.Find(1);
    HE_EXPECT(p == nullptr);

    v.Emplace(1, 10);
    p = v.Find(1);
    HE_EXPECT(p != nullptr);
    HE_EXPECT_EQ(*p, 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashMap_Get)
{
    HashMap<int, uint32_t> v;

    HE_EXPECT_ASSERT({
        // this will assert because it does not exist
        uint32_t& i = v.Get(1);
        HE_UNUSED(i);
    });

    // this should not assert
    v.Emplace(1, 10);
    uint32_t& j = v.Get(1);
    HE_EXPECT_EQ(j, 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashSet_Construct)
{
    HashSet<int> v;
    HE_EXPECT(v.IsEmpty());
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash_table, HashSet_Insert)
{
    HashSet<int> v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.Insert(10);
    HE_EXPECT_EQ(v.Size(), 1);

    v.Insert(10);
    HE_EXPECT_EQ(v.Size(), 1);

    v.Insert(20);
    HE_EXPECT_EQ(v.Size(), 2);

    v.Insert(10);
    HE_EXPECT_EQ(v.Size(), 2);
}

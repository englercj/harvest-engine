// Copyright Chad Engler

#include "he/core/intrusive_list.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
struct FixtureEntry
{
    uint32_t data;
    IntrusiveListLink<FixtureEntry> link;
};
using FixtureList = IntrusiveList<FixtureEntry, &FixtureEntry::link>;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Contants)
{
    // Changing these are potentially breaking so checking them here so any changes are made with thoughtfulness.
    static_assert(IsSame<FixtureList::ElementType, FixtureEntry>);
    static_assert(IsSame<FixtureList::ListType, FixtureList>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Construct)
{
    FixtureList v;
    HE_EXPECT_EQ(v.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Construct_Move)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };

    FixtureList v;
    v.PushBack(&node1);
    v.PushBack(&node2);
    HE_EXPECT_EQ(v.Size(), 2);

    FixtureList v2{ Move(v) };
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_EQ(v2.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, operator_assign_move)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };

    FixtureList v;
    v.PushBack(&node1);
    v.PushBack(&node2);
    HE_EXPECT_EQ(v.Size(), 2);

    FixtureList v2;
    v2 = Move(v);
    HE_EXPECT_EQ(v.Size(), 0);
    HE_EXPECT_EQ(v2.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, IsEmpty)
{
    FixtureEntry node1{ 1 };

    FixtureList v;
    HE_EXPECT(v.IsEmpty());

    v.PushBack(&node1);
    HE_EXPECT(!v.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Size)
{
    FixtureEntry node1{ 1 };

    FixtureList v;
    HE_EXPECT_EQ(v.Size(), 0);

    v.PushBack(&node1);
    HE_EXPECT_EQ(v.Size(), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Clear)
{
    FixtureEntry node1{ 1 };
    HE_EXPECT_EQ_PTR(node1.link.next, nullptr);
    HE_EXPECT_EQ_PTR(node1.link.prev, nullptr);

    FixtureList v;
    HE_EXPECT(v.IsEmpty());

    v.PushBack(&node1);
    HE_EXPECT(!v.IsEmpty());
    HE_EXPECT_NE_PTR(v.Front(), nullptr);
    HE_EXPECT_NE_PTR(v.Back(), nullptr);

    v.Clear();
    HE_EXPECT(v.IsEmpty());
    HE_EXPECT_EQ_PTR(v.Front(), nullptr);
    HE_EXPECT_EQ_PTR(v.Back(), nullptr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Front_Back)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };

    FixtureList v;
    v.PushBack(&node1);
    v.PushBack(&node2);
    HE_EXPECT_EQ(v.Size(), 2);

    HE_EXPECT_EQ_PTR(v.Front(), &node1);
    HE_EXPECT_EQ_PTR(v.Back(), &node2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Next_Prev)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };
    FixtureEntry node3{ 3 };

    FixtureList v;
    v.PushBack(&node1);
    v.PushBack(&node2);
    v.PushBack(&node3);
    HE_EXPECT_EQ(v.Size(), 3);

    HE_EXPECT_EQ_PTR(v.Front(), &node1);
    HE_EXPECT_EQ_PTR(v.Next(&node1), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node3);
    HE_EXPECT_EQ_PTR(v.Next(&node3), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node1), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node2), &node1);
    HE_EXPECT_EQ_PTR(v.Prev(&node3), &node2);
    HE_EXPECT_EQ_PTR(v.Back(), &node3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, PushBack)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };
    FixtureEntry node3{ 3 };

    FixtureList v;
    v.PushBack(&node1);
    v.PushBack(&node2);
    v.PushBack(&node3);
    HE_EXPECT_EQ(v.Size(), 3);

    HE_EXPECT_EQ_PTR(v.Front(), &node1);
    HE_EXPECT_EQ_PTR(v.Next(&node1), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node3);
    HE_EXPECT_EQ_PTR(v.Next(&node3), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node1), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node2), &node1);
    HE_EXPECT_EQ_PTR(v.Prev(&node3), &node2);
    HE_EXPECT_EQ_PTR(v.Back(), &node3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, PushFront)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };
    FixtureEntry node3{ 3 };

    FixtureList v;
    v.PushFront(&node3);
    v.PushFront(&node2);
    v.PushFront(&node1);
    HE_EXPECT_EQ(v.Size(), 3);

    HE_EXPECT_EQ_PTR(v.Front(), &node1);
    HE_EXPECT_EQ_PTR(v.Next(&node1), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node3);
    HE_EXPECT_EQ_PTR(v.Next(&node3), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node1), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node2), &node1);
    HE_EXPECT_EQ_PTR(v.Prev(&node3), &node2);
    HE_EXPECT_EQ_PTR(v.Back(), &node3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, Remove)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };
    FixtureEntry node3{ 3 };
    FixtureEntry node4{ 4 };
    FixtureEntry node5{ 5 };

    FixtureList v;
    v.PushBack(&node1);
    v.PushBack(&node2);
    v.PushBack(&node3);
    v.PushBack(&node4);
    v.PushBack(&node5);
    HE_EXPECT_EQ(v.Size(), 5);

    HE_EXPECT_EQ_PTR(v.Front(), &node1);
    HE_EXPECT_EQ_PTR(v.Next(&node1), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node3);
    HE_EXPECT_EQ_PTR(v.Next(&node3), &node4);
    HE_EXPECT_EQ_PTR(v.Next(&node4), &node5);
    HE_EXPECT_EQ_PTR(v.Next(&node5), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node1), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node2), &node1);
    HE_EXPECT_EQ_PTR(v.Prev(&node3), &node2);
    HE_EXPECT_EQ_PTR(v.Prev(&node4), &node3);
    HE_EXPECT_EQ_PTR(v.Prev(&node5), &node4);
    HE_EXPECT_EQ_PTR(v.Back(), &node5);

    // Remove head
    v.Remove(&node1);
    HE_EXPECT_EQ_PTR(v.Front(), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node3);
    HE_EXPECT_EQ_PTR(v.Next(&node3), &node4);
    HE_EXPECT_EQ_PTR(v.Next(&node4), &node5);
    HE_EXPECT_EQ_PTR(v.Next(&node5), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node2), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node3), &node2);
    HE_EXPECT_EQ_PTR(v.Prev(&node4), &node3);
    HE_EXPECT_EQ_PTR(v.Prev(&node5), &node4);
    HE_EXPECT_EQ_PTR(v.Back(), &node5);

    // Remove tail
    v.Remove(&node5);
    HE_EXPECT_EQ_PTR(v.Front(), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node3);
    HE_EXPECT_EQ_PTR(v.Next(&node3), &node4);
    HE_EXPECT_EQ_PTR(v.Next(&node4), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node2), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node3), &node2);
    HE_EXPECT_EQ_PTR(v.Prev(&node4), &node3);
    HE_EXPECT_EQ_PTR(v.Back(), &node4);

    // Remove middle
    v.Remove(&node3);
    HE_EXPECT_EQ_PTR(v.Front(), &node2);
    HE_EXPECT_EQ_PTR(v.Next(&node2), &node4);
    HE_EXPECT_EQ_PTR(v.Next(&node4), nullptr);

    HE_EXPECT_EQ_PTR(v.Prev(&node2), nullptr);
    HE_EXPECT_EQ_PTR(v.Prev(&node4), &node2);
    HE_EXPECT_EQ_PTR(v.Back(), &node4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, intrusive_list, RangeBasedFor)
{
    FixtureEntry node1{ 1 };
    FixtureEntry node2{ 2 };
    FixtureEntry node3{ 3 };

    FixtureList v;
    v.PushFront(&node3);
    v.PushFront(&node2);
    v.PushFront(&node1);
    HE_EXPECT_EQ(v.Size(), 3);

    uint32_t value = 1;
    for (const FixtureEntry& node : v)
    {
        HE_EXPECT_EQ(node.data, value);
        ++value;
    }
}

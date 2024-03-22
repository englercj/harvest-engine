// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/rb_tree.h"

#include "he/core/test.h"

#include <algorithm>

using namespace he;

// ------------------------------------------------------------------------------------------------
struct RBTestNode
{
    uint32_t key;
    uint32_t value;
    RBTreeLink<RBTestNode> link;
};
using RBTestTree = RBTree<RBTestNode, &RBTestNode::link, uint32_t, &RBTestNode::key>;

struct RBTestTraits
{
    struct Node final
    {
        template <typename U>
        explicit Node(U&& k) : key(Forward<U>(k)), value() {}

        template <typename U, typename... Args>
        explicit Node(U&& k, Args&&... args) : key(Forward<U>(k)), value(Forward<Args>(args)...) {}

        uint32_t key;
        uint32_t value;

    private:
        friend RBTestTraits;
        RBTreeLink<Node> link;
    };

    using KeyType = uint32_t;
    using EntryType = Node;
    using TreeType = RBTree<Node, &Node::link, KeyType, &Node::key>;
};

using RBTestContainer = RBTreeContainerBase<RBTestTraits>;
using RBTestMap = RBTreeMap<uint32_t, uint32_t>;
using RBTestSet = RBTreeSet<uint32_t>;

static RBTestNode s_testNodes[]
{
    { 1, 1 },
    { 2, 2 },
    { 3, 3 },
    { 4, 4 },
    { 5, 5 },
    { 6, 6 },
    { 7, 7 },
    { 8, 8 },
    { 9, 9 },
    { 10, 10 },
};

static RBTestTraits::Node s_testContainerNodes[]
{
    RBTestTraits::Node{ 1, 1 },
    RBTestTraits::Node{ 2, 2 },
    RBTestTraits::Node{ 3, 3 },
    RBTestTraits::Node{ 4, 4 },
    RBTestTraits::Node{ 5, 5 },
    RBTestTraits::Node{ 6, 6 },
    RBTestTraits::Node{ 7, 7 },
    RBTestTraits::Node{ 8, 8 },
    RBTestTraits::Node{ 9, 9 },
    RBTestTraits::Node{ 10, 10 },
};

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, Construct)
{
    {
        RBTestTree tree;
        HE_EXPECT(tree.IsEmpty());
    }

    {
        RBTestTree tree;
        for (RBTestNode& node : s_testNodes)
            tree.Insert(&node);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT_EQ_PTR(tree.Find(1), &s_testNodes[0]);

        RBTestTree tree2(Move(tree));
        HE_EXPECT(tree.IsEmpty());
        HE_EXPECT(!tree2.IsEmpty());
        HE_EXPECT_EQ_PTR(tree2.Find(1), &s_testNodes[0]);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, operator_assign_move)
{
    RBTestTree tree;
    for (RBTestNode& node : s_testNodes)
        tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.Find(1), &s_testNodes[0]);

    RBTestTree tree2;
    tree2 = Move(tree);
    HE_EXPECT(tree.IsEmpty());
    HE_EXPECT(!tree2.IsEmpty());
    HE_EXPECT_EQ_PTR(tree2.Find(1), &s_testNodes[0]);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, IsEmpty)
{
    RBTestTree tree;
    HE_EXPECT(tree.IsEmpty());

    RBTestNode node{};
    tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());

    tree.Clear();
    HE_EXPECT(tree.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, First)
{
    RBTestTree tree;
    for (RBTestNode& node : s_testNodes)
        tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.First(), &s_testNodes[0]);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, Last)
{
    RBTestTree tree;
    for (RBTestNode& node : s_testNodes)
        tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.Last(), &s_testNodes[HE_LENGTH_OF(s_testNodes) - 1]);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, Clear)
{
    RBTestTree tree;
    HE_EXPECT(tree.IsEmpty());

    tree.Clear();
    HE_EXPECT(tree.IsEmpty());

    RBTestNode node{};
    tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());

    tree.Clear();
    HE_EXPECT(tree.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, Find)
{
    RBTestTree tree;

    RBTestNode* p = tree.Find(0);
    HE_EXPECT(p == nullptr);

    RBTestNode node{};
    tree.Insert(&node);
    p = tree.Find(0);
    HE_EXPECT(p == &node);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, Insert)
{
    RBTestTree tree;
    HE_EXPECT(tree.IsEmpty());

    RBTestNode node{};
    tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.First(), &node);
    HE_EXPECT_EQ_PTR(tree.Last(), &node);

    RBTestNode node2{ 1, 1 };
    tree.Insert(&node2);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.First(), &node);
    HE_EXPECT_EQ_PTR(tree.Last(), &node2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, Remove)
{
    constexpr uint32_t LastIndex = HE_LENGTH_OF(s_testNodes) - 1;

    RBTestTree tree;
    HE_EXPECT(tree.IsEmpty());

    for (RBTestNode& node : s_testNodes)
        tree.Insert(&node);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.First(), &s_testNodes[0]);
    HE_EXPECT_EQ_PTR(tree.Last(), &s_testNodes[LastIndex]);

    for (uint32_t i = 0; i < LastIndex; ++i)
    {
        tree.Remove(s_testNodes[i].key);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT_EQ_PTR(tree.First(), &s_testNodes[i + 1]);
        HE_EXPECT_EQ_PTR(tree.Last(), &s_testNodes[LastIndex]);
    }
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT_EQ_PTR(tree.First(), &s_testNodes[LastIndex]);
    HE_EXPECT_EQ_PTR(tree.Last(), &s_testNodes[LastIndex]);

    tree.Remove(s_testNodes[LastIndex].key);
    HE_EXPECT(tree.IsEmpty());
    HE_EXPECT(tree.First() == nullptr);
    HE_EXPECT(tree.Last() == nullptr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, CopyFrom)
{
    RBTestTree tree;
    for (RBTestNode& node : s_testNodes)
        tree.Insert(&node);

    struct Pool
    {
        RBTestNode nodes[HE_LENGTH_OF(s_testNodes)]{};
        uint32_t index{ 0 };
    } pool;
    RBTestTree copy;
    copy.CopyFrom(tree, [](const RBTestNode* node, void* userData) -> RBTestNode*
    {
        Pool* pool = static_cast<Pool*>(userData);
        HE_EXPECT_LT(pool->index, HE_LENGTH_OF(pool->nodes));
        RBTestNode& newNode = pool->nodes[pool->index++];
        newNode.key = node->key;
        newNode.value = node->value;
        return &newNode;
    }, &pool);

    RBTestNode* itA = tree.First();
    RBTestNode* itB = copy.First();
    for (; itA && itB; itA = tree.Next(itA), itB = copy.Next(itB))
    {
        HE_EXPECT_EQ(itA->key, itB->key);
        HE_EXPECT_EQ(itA->value, itB->value);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Construct)
{
    {
        RBTestContainer tree;
        HE_EXPECT(tree.IsEmpty());
        HE_EXPECT(&tree.GetAllocator() == &Allocator::GetDefault());
    }

    {
        AnotherAllocator a2;
        RBTestContainer tree(a2);
        HE_EXPECT(tree.IsEmpty());
        HE_EXPECT(&tree.GetAllocator() == &a2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Construct_Copy)
{
    RBTestContainer tree;
    for (RBTestTraits::Node& node : s_testContainerNodes)
        tree.Emplace(node.key, node.value);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT(tree.Find(1) != nullptr);

    {
        RBTestContainer copy(tree);
        HE_EXPECT(!copy.IsEmpty());
        HE_EXPECT(&copy.GetAllocator() == &tree.GetAllocator());
        HE_EXPECT(tree.Find(1) != nullptr);
    }

    {
        AnotherAllocator a2;
        RBTestContainer copy(tree, a2);
        HE_EXPECT(!copy.IsEmpty());
        HE_EXPECT(&copy.GetAllocator() == &a2);
        HE_EXPECT(tree.Find(1) != nullptr);
    }

    {
        RBTestContainer copy = tree;
        HE_EXPECT(!copy.IsEmpty());
        HE_EXPECT(&copy.GetAllocator() == &tree.GetAllocator());
        HE_EXPECT(tree.Find(1) != nullptr);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Construct_Move)
{
    {
        RBTestContainer tree;
        for (RBTestTraits::Node& node : s_testContainerNodes)
            tree.Emplace(node.key, node.value);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT(tree.Find(1) != nullptr);

        RBTestContainer moved(Move(tree));
        HE_EXPECT(!moved.IsEmpty());
        HE_EXPECT(&moved.GetAllocator() == &tree.GetAllocator());
        HE_EXPECT(moved.Find(1) != nullptr);
        HE_EXPECT(tree.IsEmpty());
    }

    {
        RBTestContainer tree;
        for (RBTestTraits::Node& node : s_testContainerNodes)
            tree.Emplace(node.key, node.value);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT(tree.Find(1) != nullptr);

        AnotherAllocator a2;
        RBTestContainer moved(Move(tree), a2);
        HE_EXPECT(!moved.IsEmpty());
        HE_EXPECT(&moved.GetAllocator() == &a2);
        HE_EXPECT(moved.Find(1) != nullptr);
        HE_EXPECT(tree.IsEmpty());
    }

    {
        RBTestContainer tree;
        for (RBTestTraits::Node& node : s_testContainerNodes)
            tree.Emplace(node.key, node.value);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT(tree.Find(1) != nullptr);

        RBTestContainer moved = Move(tree);
        HE_EXPECT(!moved.IsEmpty());
        HE_EXPECT(&moved.GetAllocator() == &tree.GetAllocator());
        HE_EXPECT(moved.Find(1) != nullptr);
        HE_EXPECT(tree.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_operator_assign_copy)
{
    RBTestContainer tree;
    for (RBTestTraits::Node& node : s_testContainerNodes)
        tree.Emplace(node.key, node.value);
    HE_EXPECT(!tree.IsEmpty());
    HE_EXPECT(tree.Find(1) != nullptr);

    {
        RBTestContainer copy;
        copy = tree;
        HE_EXPECT(!copy.IsEmpty());
        HE_EXPECT(&copy.GetAllocator() == &tree.GetAllocator());
        HE_EXPECT(copy.Find(1) != nullptr);
    }

    {
        AnotherAllocator a2;
        RBTestContainer copy(a2);
        copy = tree;
        HE_EXPECT(!copy.IsEmpty());
        HE_EXPECT(&copy.GetAllocator() == &a2);
        HE_EXPECT(copy.Find(1) != nullptr);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_operator_assign_move)
{
    {
        RBTestContainer tree;
        for (RBTestTraits::Node& node : s_testContainerNodes)
            tree.Emplace(node.key, node.value);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT(tree.Find(1) != nullptr);

        RBTestContainer moved;
        moved = Move(tree);
        HE_EXPECT(!moved.IsEmpty());
        HE_EXPECT(&moved.GetAllocator() == &tree.GetAllocator());
        HE_EXPECT(moved.Find(1) != nullptr);
        HE_EXPECT(tree.IsEmpty());
    }

    {
        RBTestContainer tree;
        for (RBTestTraits::Node& node : s_testContainerNodes)
            tree.Emplace(node.key, node.value);
        HE_EXPECT(!tree.IsEmpty());
        HE_EXPECT(tree.Find(1) != nullptr);

        AnotherAllocator a2;
        RBTestContainer moved(a2);
        moved = Move(tree);
        HE_EXPECT(!moved.IsEmpty());
        HE_EXPECT(&moved.GetAllocator() == &a2);
        HE_EXPECT(moved.Find(1) != nullptr);
        HE_EXPECT(tree.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_operator_eq)
{
    RBTestContainer tree;
    for (RBTestTraits::Node& node : s_testContainerNodes)
        tree.Emplace(node.key, node.value);

    RBTestContainer tree2;
    for (RBTestTraits::Node& node : s_testContainerNodes)
        tree2.Emplace(node.key, node.value);

    HE_EXPECT(tree == tree2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_operator_ne)
{
    {
        RBTestContainer tree;
        for (uint32_t i = 0; i < 10; ++i)
            tree.Emplace(i, i);

        RBTestContainer tree2;
        for (uint32_t i = 0; i < 15; ++i)
            tree.Emplace(i, i);

        HE_EXPECT_NE(tree.Size(), tree2.Size());
        HE_EXPECT(tree != tree2);
    }

    {
        RBTestContainer tree;
        for (uint32_t i = 0; i < 10; ++i)
            tree.Emplace(i, i);

        RBTestContainer tree2;
        for (uint32_t i = 10; i < 20; ++i)
            tree2.Emplace(i, i);

        HE_EXPECT_EQ(tree.Size(), tree2.Size());
        HE_EXPECT(tree != tree2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_IsEmpty)
{
    RBTestContainer tree;
    HE_EXPECT(tree.IsEmpty());

    tree.Emplace(10);
    HE_EXPECT(!tree.IsEmpty());

    tree.Clear();
    HE_EXPECT(tree.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Size)
{
    RBTestContainer tree;
    HE_EXPECT_EQ(tree.Size(), 0);

    tree.Emplace(10);
    HE_EXPECT_EQ(tree.Size(), 1);

    tree.Clear();
    HE_EXPECT_EQ(tree.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Contains)
{
    RBTestContainer tree;
    HE_EXPECT(!tree.Contains(1));

    tree.Emplace(1);
    HE_EXPECT(tree.Contains(1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Find)
{
    RBTestContainer tree;

    RBTestTraits::Node* p = tree.Find(1);
    HE_EXPECT(p == nullptr);

    tree.Emplace(1);
    p = tree.Find(1);
    HE_EXPECT(p != nullptr);
    HE_EXPECT_EQ(p->key, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Get)
{
    RBTestContainer tree;

    HE_EXPECT_ASSERT({
        // this will assert because it does not exist
        [[maybe_unused]] RBTestTraits::Node& node = tree.Get(1);
    });

    // this should not assert
    tree.Emplace(1);
    RBTestTraits::Node& i = tree.Get(1);
    HE_EXPECT_EQ(i.key, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Begin_End)
{
    RBTestContainer tree;
    HE_EXPECT(!tree.Begin());
    HE_EXPECT(!tree.End());
    HE_EXPECT(tree.Begin() == tree.End());

    tree.Emplace(1);
    HE_EXPECT(tree.Begin());
    HE_EXPECT(!tree.End());
    HE_EXPECT(tree.Begin() != tree.End());
    HE_EXPECT_EQ(tree.Begin()->key, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_RangeBasedFor)
{
    uint32_t values[]{ 5, 2, 7, 3, 1, 4, 6, 8 };

    RBTestContainer tree;
    for (uint32_t i = 0; i < HE_LENGTH_OF(values); ++i)
        tree.Emplace(values[i], values[i]);

    HE_EXPECT_EQ(tree.Size(), HE_LENGTH_OF(values));

    uint32_t last = 0;
    for (const RBTestContainer::EntryType& it : tree)
    {
        const uint32_t value = last + 1;
        HE_EXPECT_EQ(it.key, value);
        HE_EXPECT_EQ(it.value, value);
        HE_EXPECT_GT(it.key, last);
        last = it.key;
    }
    HE_EXPECT_EQ(last, 8);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Clear)
{
    {
        RBTestContainer tree;
        HE_EXPECT(tree.IsEmpty());

        tree.Clear();
        HE_EXPECT(tree.IsEmpty());

        tree.Emplace(10);
        HE_EXPECT(!tree.IsEmpty());

        tree.Clear();
        HE_EXPECT(tree.IsEmpty());
    }

    {
        static uint32_t s_counter = 0;
        static uint32_t s_destructed = 0;

        struct TestObj
        {
            TestObj() : h(s_counter++) {}
            ~TestObj() { ++s_destructed; }
            bool operator==(const TestObj& x) const { return h == x.h; }
            bool operator<(const TestObj& x) const { return h < x.h; }
            uint64_t h;
        };
        const TestObj value{};

        RBTreeContainerBase<RBTreeSetTraits<TestObj>> tree;
        tree.Emplace(value);
        HE_EXPECT_EQ(s_destructed, 0);
        HE_EXPECT_EQ(tree.Size(), 1);

        tree.Clear();
        HE_EXPECT_EQ(s_destructed, 1);
        HE_EXPECT_EQ(tree.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Erase)
{
    {
        const uint32_t expected[]{ 1, 2, 60, 3 };

        RBTestContainer tree;
        for (uint32_t i = 0; i < HE_LENGTH_OF(expected); ++i)
            tree.Emplace(expected[i]);

        HE_EXPECT_EQ(tree.Size(), HE_LENGTH_OF(expected));
        for (uint32_t i : expected)
        {
            HE_EXPECT(tree.Contains(i));
        }

        tree.Erase(1);
        HE_EXPECT_EQ(tree.Size(), HE_LENGTH_OF(expected) - 1);
        for (uint32_t i : expected)
        {
            if (i == 1)
            {
                HE_EXPECT(!tree.Contains(i));
            }
            else
            {
                HE_EXPECT(tree.Contains(i));
            }
        }

        tree.Erase(60);
        HE_EXPECT_EQ(tree.Size(), HE_LENGTH_OF(expected) - 2);
        for (uint32_t i : expected)
        {
            if (i == 1 || i == 60)
            {
                HE_EXPECT(!tree.Contains(i));
            }
            else
            {
                HE_EXPECT(tree.Contains(i));
            }
        }
    }

    {
        static uint32_t s_counter = 0;
        static uint32_t s_destructed = 0;

        struct TestObj
        {
            TestObj() : h(s_counter++) {}
            ~TestObj() { ++s_destructed; }
            bool operator==(const TestObj& x) const { return h == x.h; }
            bool operator<(const TestObj& x) const { return h < x.h; }
            uint64_t h;
        };
        const TestObj value{};

        RBTreeContainerBase<RBTreeSetTraits<TestObj>> tree;
        tree.Emplace(value);
        HE_EXPECT_EQ(s_destructed, 0);
        HE_EXPECT_EQ(tree.Size(), 1);

        tree.Erase(value);
        HE_EXPECT_EQ(s_destructed, 1);
        HE_EXPECT_EQ(tree.Size(), 0);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeContainerBase_Emplace)
{
    RBTestContainer tree;
    HE_EXPECT_EQ(tree.Size(), 0);

    {
        auto result = tree.Emplace(25, 50);
        HE_EXPECT(result.inserted);
        HE_EXPECT_EQ(result.entry.key, 25);
        HE_EXPECT_EQ(result.entry.value, 50);
        HE_EXPECT_EQ(tree.Size(), 1);
        HE_EXPECT(tree.Find(25) != nullptr);
    }

    {
        auto result = tree.Emplace(50, 100);
        HE_EXPECT(result.inserted);
        HE_EXPECT_EQ(result.entry.key, 50);
        HE_EXPECT_EQ(result.entry.value, 100);
        HE_EXPECT_EQ(tree.Size(), 2);
        HE_EXPECT(tree.Find(50) != nullptr);
    }

    {
        auto result = tree.Emplace(25, 100);
        HE_EXPECT(!result.inserted);
        HE_EXPECT_EQ(result.entry.key, 25);
        HE_EXPECT_EQ(result.entry.value, 50);
        HE_EXPECT_EQ(tree.Size(), 2);
        HE_EXPECT(tree.Find(25) != nullptr);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeMap_Construct)
{
    RBTestMap tree;
    HE_EXPECT(tree.IsEmpty());
    HE_EXPECT_EQ(tree.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeMap_operator_index)
{
    RBTestMap tree;
    HE_EXPECT_EQ(tree.Size(), 0);

    tree[1] = 500;
    HE_EXPECT_EQ(tree[1], 500);
    HE_EXPECT_EQ(tree.Size(), 1);

    tree[1] = 256;
    HE_EXPECT_EQ(tree[1], 256);
    HE_EXPECT_EQ(tree.Size(), 1);

    HE_EXPECT_EQ(tree[10], 0);
    HE_EXPECT_EQ(tree.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeMap_EmplaceOrAssign)
{
    RBTestMap tree;
    HE_EXPECT_EQ(tree.Size(), 0);

    tree.EmplaceOrAssign(1, 500);
    HE_EXPECT_EQ(tree.Size(), 1);
    HE_EXPECT_EQ(tree.Get(1), 500);

    tree.EmplaceOrAssign(1, 100);
    HE_EXPECT_EQ(tree.Size(), 1);
    HE_EXPECT_EQ(tree.Get(1), 100);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeMap_Find)
{
    RBTestMap tree;

    uint32_t* p = tree.Find(1);
    HE_EXPECT(p == nullptr);

    tree.Emplace(1, 10);
    p = tree.Find(1);
    HE_EXPECT(p != nullptr);
    HE_EXPECT_EQ(*p, 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeMap_Get)
{
    RBTestMap tree;

    HE_EXPECT_ASSERT({
        // this will assert because it does not exist
        [[maybe_unused]] uint32_t& i = tree.Get(1);
    });

    // this should not assert
    tree.Emplace(1, 10);
    uint32_t& j = tree.Get(1);
    HE_EXPECT_EQ(j, 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeSet_Construct)
{
    RBTestSet tree;
    HE_EXPECT(tree.IsEmpty());
    HE_EXPECT_EQ(tree.Size(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, rb_tree, RBTreeSet_Insert)
{
    RBTestSet tree;
    HE_EXPECT_EQ(tree.Size(), 0);

    tree.Insert(10);
    HE_EXPECT_EQ(tree.Size(), 1);

    tree.Insert(10);
    HE_EXPECT_EQ(tree.Size(), 1);

    tree.Insert(20);
    HE_EXPECT_EQ(tree.Size(), 2);

    tree.Insert(10);
    HE_EXPECT_EQ(tree.Size(), 2);
}

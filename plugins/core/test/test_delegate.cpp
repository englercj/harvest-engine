// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/delegate.h"

#include "he/core/test.h"
#include "he/core/type_traits.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
// Fixtures

static int _TestConstRef(const int& i) { return i * i; }
static int _TestConstPtr(const int* i) { return (*i) * (*i); }
static int _TestConstRefPayload(const int& i, int j) { return i + j; }
static int _TestConstPtrPayload(const int* i, int j) { return (*i) + j; }

static int _TestRef(int& i) { return i * i; }
static int _TestPtr(int* i) { return (*i) * (*i); }
static int _TestRefPayload(int& i, int j) { return i + j; }
static int _TestPtrPayload(int* i, int j) { return (*i) + j; }

static bool _TestMoveOnly(MoveOnly x) { return x.moveConstructed; }

static double _TestEmpty() { return 10.0; }

namespace
{
    struct _TestFunctor
    {
        int operator()(int i) { return i + i; }
        int MemberFunc(int i) const { return i; }

        const int m_value = 20;
    };
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, delegate, Static_Make)
{
    using D = Delegate<int(int)>;

    // Free function, const ref
    {
        D d = D::Make<&_TestConstRef>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 9);
    }

    // Free function, const ptr
    {
        int v = 3;
        Delegate<int(int*)> d = Delegate<int(int*)>::Make<&_TestConstPtr>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&v), 9);
    }

    // Free function, const ref payload
    {
        int v = 5;
        D d = D::Make<&_TestConstRefPayload>(v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Free function, const ptr payload
    {
        int v = 5;
        D d = D::Make<&_TestConstPtrPayload>(&v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Free function, non-const ref
    {
        int v = 3;
        Delegate<int(int&)> d = Delegate<int(int&)>::Make<&_TestRef>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(v), 9);
    }

    // Free function, non-const ptr
    {
        int v = 3;
        Delegate<int(int*)> d = Delegate<int(int*)>::Make<&_TestPtr>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&v), 9);
    }

    // Free function, non-const ref payload
    {
        int v = 5;
        D d = D::Make<&_TestRefPayload>(v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Free function, non-const ptr payload
    {
        int v = 5;
        D d = D::Make<&_TestPtrPayload>(&v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Functor, non-const
    {
        _TestFunctor f;
        D d = D::Make<&_TestFunctor::operator()>(f);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 6);
    }

    // Functor, non-const, unbound
    {
        _TestFunctor f;
        Delegate<int(_TestFunctor*,int)> d = Delegate<int(_TestFunctor*,int)>::Make<&_TestFunctor::operator()>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&f, 3), 6);
    }

    // Less arguments than delegate
    {
        MoveOnly m;
        D d = D::Make<&_TestEmpty>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 10.0);
    }

    // Member function
    {
        const _TestFunctor f;
        D d = D::Make<&_TestFunctor::MemberFunc>(&f);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 3);
    }

    // Member function, unbound
    {
        const _TestFunctor f;
        Delegate<int(const _TestFunctor*, int)> d = Delegate<int(const _TestFunctor*, int)>::Make<&_TestFunctor::MemberFunc>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&f, 3), 3);
    }

    // Member data
    {
        _TestFunctor f;
        Delegate<int()> d = Delegate<int()>::Make<&_TestFunctor::m_value>(f);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(), 20);
    }

    // Convertable return type
    {
        MoveOnly m;
        Delegate<int(MoveOnly)> d = Delegate<int(MoveOnly)>::Make<&_TestMoveOnly>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(Move(m)), 1);
    }

    // Void return type
    {
        _TestFunctor f;

        Delegate<void(int)> d1 = Delegate<void(int)>::Make<&_TestConstRef>();
        HE_EXPECT(d1);

        Delegate<void(int)> d2 = Delegate<void(int)>::Make<&_TestFunctor::operator()>(f);
        HE_EXPECT(d2);

        Delegate<void(int)> d3 = Delegate<void(int)>::Make<&_TestFunctor::MemberFunc>(AsConst(f));
        HE_EXPECT(d3);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, delegate, Construct)
{
    // Default construct
    {
        Delegate<int(int)> d;
        HE_EXPECT(!d);
    }

    // Function type construct
    {
        Delegate<int(int)> d([](const void*, int c) { return c; });
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 3);
    }

    // Deduction guide, function type
    {
        Delegate d(+[](const void*, int c) { return c; });
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 3);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, delegate, Set)
{
    using D = Delegate<int(int)>;

    // Free function, const ref
    {
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestConstRef>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 9);
    }

    // Free function, const ptr
    {
        int v = 3;
        Delegate<int(int*)> d;
        d.Set<&_TestConstPtr>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&v), 9);
    }

    // Free function, const ref payload
    {
        int v = 5;
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestConstRefPayload>(v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Free function, const ptr payload
    {
        int v = 5;
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestConstPtrPayload>(&v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Free function, non-const ref
    {
        int v = 3;
        Delegate<int(int&)> d;
        HE_EXPECT(!d);
        d.Set<&_TestRef>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(v), 9);
    }

    // Free function, non-const ptr
    {
        int v = 3;
        Delegate<int(int*)> d;
        d.Set<&_TestPtr>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&v), 9);
    }

    // Free function, non-const ref payload
    {
        int v = 5;
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestRefPayload>(v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Free function, non-const ptr payload
    {
        int v = 5;
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestPtrPayload>(&v);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 8);
    }

    // Functor, non-const
    {
        _TestFunctor f;
        D d;
        d.Set<&_TestFunctor::operator()>(f);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 6);
    }

    // Functor, non-const, unbound
    {
        _TestFunctor f;
        Delegate<int(_TestFunctor*,int)> d;
        d.Set<&_TestFunctor::operator()>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&f, 3), 6);
    }

    // Less arguments than delegate
    {
        MoveOnly m;
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestEmpty>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 10.0);
    }

    // Member function
    {
        const _TestFunctor f;
        D d;
        HE_EXPECT(!d);
        d.Set<&_TestFunctor::MemberFunc>(f);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(3), 3);
    }

    // Member function, unbound
    {
        const _TestFunctor f;
        Delegate<int(const _TestFunctor*, int)> d;
        HE_EXPECT(!d);
        d.Set<&_TestFunctor::MemberFunc>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(&f, 3), 3);
    }

    // Member data
    {
        _TestFunctor f;
        Delegate<int()> d;
        d.Set<&_TestFunctor::m_value>(f);
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(), 20);
    }

    // Convertable return type
    {
        MoveOnly m;
        Delegate<int(MoveOnly)> d;
        d.Set<&_TestMoveOnly>();
        HE_EXPECT(d);
        HE_EXPECT_EQ(d(Move(m)), 1);
    }

    // Void return type
    {
        _TestFunctor f;

        Delegate<void(int)> d1;
        d1.Set<&_TestConstRef>();
        HE_EXPECT(d1);

        Delegate<void(int)> d2;
        d2.Set<&_TestFunctor::operator()>(f);
        HE_EXPECT(d2);

        Delegate<void(int)> d3;
        d3.Set<&_TestFunctor::MemberFunc>(AsConst(f));
        HE_EXPECT(d3);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, delegate, Clear)
{
    Delegate d(+[](const void*, int c) { return c; });
    HE_EXPECT(d);
    d.Clear();
    HE_EXPECT(!d);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, delegate, Payload)
{
    auto f1 = +[](const void*, int c) { return c; };
    Delegate d1(f1);
    HE_EXPECT_EQ(d1.Payload(), nullptr);

    Delegate d2(+[](const void*, int c) { return c; }, &d1);
    HE_EXPECT_EQ_PTR(d2.Payload(), &d1);

    Delegate d3(+[](const void*, int c) { return c; });
    HE_EXPECT(d3.Payload() == nullptr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, delegate, comparison_operators)
{
    using D = Delegate<int(int)>;

    D a;
    D b;
    _TestFunctor f1;
    _TestFunctor f2;
    const int v = 0;

    HE_EXPECT(a == (D{}));
    HE_EXPECT(a == b);
    HE_EXPECT(!(a != b));

    a.Set<&_TestConstRef>();

    HE_EXPECT(a == (D::Make<&_TestConstRef>()));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));

    b.Set<&_TestConstRef>();

    HE_EXPECT(b == (D::Make<&_TestConstRef>()));
    HE_EXPECT(a == b);
    HE_EXPECT(!(a != b));

    a.Set<&_TestConstRefPayload>(v);

    HE_EXPECT(a == (D::Make<&_TestConstRefPayload>(v)));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    b.Set<&_TestConstRefPayload>(v);

    HE_EXPECT(b == (D::Make<&_TestConstRefPayload>(v)));
    HE_EXPECT(!(a != b));
    HE_EXPECT(a == b);
    HE_EXPECT(a == b);

    a.Set<&_TestConstPtrPayload>(&v);

    HE_EXPECT(a == (D::Make<&_TestConstPtrPayload>(&v)));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    b.Set<&_TestConstPtrPayload>(&v);

    HE_EXPECT(b == (D::Make<&_TestConstPtrPayload>(&v)));
    HE_EXPECT(!(a != b));
    HE_EXPECT(a == b);
    HE_EXPECT(a == b);

    a.Set<&_TestFunctor::operator()>(f1);

    HE_EXPECT(a == (D::Make<&_TestFunctor::operator()>(f1)));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    b.Set<&_TestFunctor::operator()>(f1);

    HE_EXPECT(b == (D::Make<&_TestFunctor::operator()>(f1)));
    HE_EXPECT(!(a != b));
    HE_EXPECT(a == b);
    HE_EXPECT(a == b);

    a.Set<&_TestFunctor::operator()>(f2);

    HE_EXPECT(a == (D::Make<&_TestFunctor::operator()>(f2)));
    HE_EXPECT_NE_PTR(a.Payload(), b.Payload());
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    a.Set([](const void* ptr, int val) { return static_cast<const _TestFunctor*>(ptr)->MemberFunc(val) * val; }, &f1);

    HE_EXPECT(a != (D{ [](const void*, int val) { return val + val; }, &f1 }));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    b.Set([](const void* ptr, int val) { return static_cast<const _TestFunctor*>(ptr)->MemberFunc(val) + val; }, &f1);

    HE_EXPECT(b != (D{ [](const void*, int val) { return val * val; }, &f1 }));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    a.Clear();

    HE_EXPECT(a == (D{}));
    HE_EXPECT(a != b);
    HE_EXPECT(!(a == b));
    HE_EXPECT(a != b);

    b.Clear();

    HE_EXPECT(b == (D{}));
    HE_EXPECT(!(a != b));
    HE_EXPECT(a == b);
    HE_EXPECT(a == b);
}

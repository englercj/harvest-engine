// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/result_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, GetId)
{
    uint32_t self = Thread::GetId();
    HE_EXPECT_NE(self, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, SetName)
{
    Thread::SetName("[HE] Test Thread");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, Sleep)
{
    Thread::Sleep(FromPeriod<Milliseconds>(100));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, Yield)
{
    Thread::Yield();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, Run)
{
    int32_t value = 0;

    Thread::Run([&]()
    {
        value = 42;
    }).Join();

    HE_EXPECT_EQ(value, 42);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, GetNativeHandle)
{
    Thread t;
    HE_EXPECT_EQ_PTR(t.GetNativeHandle(), static_cast<void*>(nullptr));
    const Result rc = t.Start({ [](void*) {} });
    HE_EXPECT(rc, rc);
    HE_EXPECT_NE_PTR(t.GetNativeHandle(), static_cast<void*>(nullptr));
    HE_EXPECT(t.IsJoinable());
    t.Join();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, IsJoinable)
{
    Thread t;
    HE_EXPECT(!t.IsJoinable());
    const Result rc = t.Start({ [](void*) {} });
    HE_EXPECT(rc, rc);
    HE_EXPECT(t.IsJoinable());
    t.Join();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, Start)
{
    Thread t;
    const Result rc = t.Start({ [](void*) {} });
    HE_EXPECT(rc, rc);
    HE_EXPECT(t.IsJoinable());
    t.Join();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, Join)
{
    Thread t;
    HE_EXPECT(!t.IsJoinable());
    const Result rc = t.Start({ [](void*) {} });
    HE_EXPECT(rc, rc);
    HE_EXPECT(t.IsJoinable());
    t.Join();
    HE_EXPECT(!t.IsJoinable());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, Detach)
{
    Thread t;
    HE_EXPECT(!t.IsJoinable());
    const Result rc = t.Start({ [](void*) {} });
    HE_EXPECT(rc, rc);
    HE_EXPECT(t.IsJoinable());
    t.Detach();
    HE_EXPECT(!t.IsJoinable());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, thread, SetAffinity)
{
    Thread t;
    const Result rc = t.Start({ [](void*) {} });
    HE_EXPECT(rc, rc);
    HE_EXPECT(t.IsJoinable());
    const Result r = t.SetAffinity(0xffffffffffffffff);
    HE_EXPECT(r, r);
    t.Join();
}

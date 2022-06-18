// Copyright Chad Engler

#pragma once

#include "he/core/signal.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, signal, Constructor)
{
    Signal<void()> s;
    HE_EXPECT(s.IsEmpty());
    HE_EXPECT_EQ(s.Size(), 0);
    s.Dispatch();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, signal, Size_IsEmpty)
{
    Signal<void()> s;
    HE_EXPECT(s.IsEmpty());
    HE_EXPECT_EQ(s.Size(), 0);

    s.Attach([](const void*) {});
    HE_EXPECT(!s.IsEmpty());
    HE_EXPECT_EQ(s.Size(), 1);

    s.Attach([](const void*) {});
    HE_EXPECT(!s.IsEmpty());
    HE_EXPECT_EQ(s.Size(), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, signal, Attach_Dispatch)
{
    static uint32_t s_callCount = 0;

    Signal<void()> s;

    HE_EXPECT_EQ(s_callCount, 0);

    s.Attach([](const void*) { ++s_callCount; });
    s.Dispatch();
    HE_EXPECT_EQ(s_callCount, 1);

    s.Attach([](const void*) { ++s_callCount; });
    s.Dispatch();
    HE_EXPECT_EQ(s_callCount, 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, signal, AttachOnce_Dispatch)
{
    static uint32_t s_callCount = 0;

    Signal<void()> s;

    HE_EXPECT_EQ(s_callCount, 0);

    s.AttachOnce([](const void*) { ++s_callCount; });
    HE_EXPECT(!s.IsEmpty());
    s.Dispatch();
    HE_EXPECT(s.IsEmpty());
    HE_EXPECT_EQ(s_callCount, 1);

    s.AttachOnce([](const void*) { ++s_callCount; });
    HE_EXPECT(!s.IsEmpty());
    s.Dispatch();
    HE_EXPECT(s.IsEmpty());
    HE_EXPECT_EQ(s_callCount, 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, signal, Detach)
{
    Signal<void()> s;
    HE_EXPECT(s.IsEmpty());

    {
        Signal<void()>::Binding bind = s.Attach([](const void*) {});
        HE_EXPECT(!s.IsEmpty());

        s.Detach(bind);
        HE_EXPECT(s.IsEmpty());
    }

    {
        Signal<void()>::Binding bind = s.Attach([](const void*) {});
        HE_EXPECT(!s.IsEmpty());

        bind.Detach();
        HE_EXPECT(s.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, signal, DetachAll)
{
    static uint32_t s_callCount = 0;

    Signal<void()> s;

    HE_EXPECT_EQ(s_callCount, 0);

    s.Attach([](const void*) { ++s_callCount; });
    s.Dispatch();
    HE_EXPECT_EQ(s_callCount, 1);

    s.Attach([](const void*) { ++s_callCount; });
    s.DetachAll();
    s.Dispatch();
    HE_EXPECT_EQ(s_callCount, 1);
}

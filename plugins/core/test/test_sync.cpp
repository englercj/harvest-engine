// Copyright Chad Engler

// Note: Because the test functions in here are run so many times here we use HE_ASSERT instead of
// HE_EXPECT to avoid a falsely inflated expectations count at the test framework summary output.
// If we use HE_EXPECT then these tests will account for ~120,000 of the expectations making it
// seem like there is a lot more testing happening than really is.

#include "he/core/sync.h"

#include "he/core/thread.h"
#include "he/core/test.h"

#include <atomic>
#include <thread>

using namespace he;

// ------------------------------------------------------------------------------------------------
constexpr uint32_t RoundsCount = 5000;
constexpr uint32_t ThreadsCount = 8;

template <typename T, typename F>
static void TestLock(F&& func)
{
    T lock;
    bool value = false;

    auto TestFunc = [](F& func, T& lock, bool& value)
    {
        for (uint32_t i = 0; i < RoundsCount; ++i)
        {
            func(lock, value);
        }
    };

    std::thread threads[ThreadsCount];
    for (uint32_t i = 0; i < ThreadsCount; ++i)
        threads[i] = std::thread(TestFunc, std::ref(func), std::ref(lock), std::ref(value));

    for (uint32_t i = 0; i < ThreadsCount; ++i)
        threads[i].join();

    if constexpr (!IsSpecialization<T, LockGuard> && !IsSame<T, ReadLockGuard>)
    {
        HE_EXPECT(lock.TryAcquire());
        lock.Release();
    }
}

struct TestCVPred
{
    uint32_t& counter;
    bool operator()()
    {
        switch (counter)
        {
            case 0: return false;
            case 1: counter = 2; return true;
            default: HE_ASSERT(false); return false;
        }
    }
};

static void TestCV(bool useTimeout, uint32_t sleeperIndex)
{
    constexpr Duration Delay = FromPeriod<Milliseconds>(100);

    ConditionVariable cv;
    Mutex lock;
    uint32_t counter = 0;

    std::thread t0([&]()
    {
        if (sleeperIndex == 0)
            SleepCurrentThread(Delay);

        constexpr Duration Timeout = FromPeriod<Seconds>(5);
        {
            LockGuard guard(lock);

            if (useTimeout)
            {
                HE_EXPECT(cv.Wait(lock, TestCVPred{ counter }, Timeout));
            }
            else
            {
                cv.Wait(lock, TestCVPred{ counter });
            }

            HE_EXPECT_EQ(counter, 2);
        }
    });

    std::thread t1([&]()
    {
        if (sleeperIndex == 1)
            SleepCurrentThread(Delay);

        {
            LockGuard guard(lock);
            HE_EXPECT_EQ(counter, 0);
            counter = 1;
        }
        cv.WakeOne();
    });

    t0.join();
    t1.join();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, RWLock)
{
    TestLock<RWLock>([](RWLock& lock, bool& value)
    {
        lock.AcquireRead();
        HE_ASSERT(!value);
        lock.ReleaseRead();
        lock.AcquireWrite();
        value = true;
        value = false;
        lock.ReleaseWrite();
    });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, Mutex)
{
    TestLock<Mutex>([](Mutex& lock, bool& value)
    {
        lock.Acquire();
        HE_ASSERT(!value);
        value = true;
        value = false;
        lock.Release();
    });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, RecursiveMutex)
{
    TestLock<RecursiveMutex>([](RecursiveMutex& lock, bool& value)
    {
        lock.Acquire();
        HE_ASSERT(!value);
        lock.Acquire();
        value = true;
        lock.Acquire();
        value = false;
        lock.Release();
        lock.Release();
        lock.Release();
    });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, LockGuard)
{
    TestLock<Mutex>([](Mutex& lock, bool& value)
    {
        LockGuard l(lock);
        HE_ASSERT(!value);
        value = true;
        value = false;
    });
}


// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, ReadLockGuard)
{
    TestLock<RWLock>([](RWLock& lock, bool& value)
    {
        {
            ReadLockGuard l(lock);
            HE_ASSERT(!value);
        }
        {
            LockGuard l(lock);
            value = true;
            value = false;
        }
    });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, ConditionVariable)
{
    // Test that condition variables properly wake and wait
    for (uint32_t i = 0; i < 3; ++i)
    {
        TestCV(true, i);
        TestCV(false, i);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, Semaphore)
{
    constexpr Duration DelayShort = FromPeriod<Milliseconds>(100);
    constexpr Duration DelayMedium = FromPeriod<Milliseconds>(100 * 3);
    constexpr Duration DelayLong = FromPeriod<Milliseconds>(100 * 6);

    std::atomic<uint32_t> counter{ 2 };

    Semaphore sem;
    std::thread t([&]()
    {
        --counter;
        while (counter > 0) { HE_SPIN_WAIT_PAUSE(); }

        HE_EXPECT(sem.Wait(DelayShort));
        HE_EXPECT(!sem.Wait(DelayLong));
    });

    --counter;
    while (counter > 0) { HE_SPIN_WAIT_PAUSE(); }

    sem.Notify();
    SleepCurrentThread(DelayMedium);
    t.join();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, SyncEvent)
{
    // Works on one thread
    {
        SyncEvent evt;
        HE_EXPECT(!evt.Wait(Duration_Zero));
        evt.Signal();
        HE_EXPECT(evt.Wait(Duration_Zero));
        evt.Reset();
        HE_EXPECT(!evt.Wait(Duration_Zero));
    }

    // Works on multiple threads
    {
        constexpr Duration DelayShort = FromPeriod<Milliseconds>(100);
        constexpr Duration DelayMedium = FromPeriod<Milliseconds>(100 * 3);
        constexpr Duration DelayLong = FromPeriod<Milliseconds>(100 * 6);

        std::atomic<uint32_t> counter{ 2 };

        SyncEvent evt;
        std::thread t([&]()
        {
            --counter;
            while (counter > 0) { HE_SPIN_WAIT_PAUSE(); }

            HE_EXPECT(evt.Wait(DelayShort));
            HE_EXPECT(evt.Wait(DelayLong));
        });

        --counter;
        while (counter > 0) { HE_SPIN_WAIT_PAUSE(); }

        evt.Signal();
        SleepCurrentThread(DelayMedium);
        t.join();
    }
}

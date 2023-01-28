// Copyright Chad Engler

// Note: Because the test functions in here are run so many times here we use HE_ASSERT instead of
// HE_EXPECT to avoid a falsely inflated expectations count at the test framework summary output.
// If we use HE_EXPECT then these tests will account for ~120,000 of the expectations making it
// seem like there is a lot more testing happening than really is.

#include "he/core/sync.h"

#include "he/core/test.h"

#include <thread>

using namespace he;

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
    // TODO
    ConditionVariable cv;
    cv.WakeAll();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, Semaphore)
{
    // TODO
    Semaphore sem;
    sem.Notify();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, sync, SyncEvent)
{
    // TODO
    SyncEvent evt;
    HE_EXPECT(!evt.Wait(Duration_Zero));
    evt.Signal();
    HE_EXPECT(evt.Wait(Duration_Zero));
    evt.Reset();
    HE_EXPECT(!evt.Wait(Duration_Zero));
}

// Copyright Chad Engler

#pragma once

#include "he/core/delegate.h"
#include "he/core/result.h"
#include "he/core/sync.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/core/vector.h"

#include <deque>

namespace he
{
    // --------------------------------------------------------------------------------------------
    using TaskDelegate = Delegate<void()>;

    // --------------------------------------------------------------------------------------------
    class TaskExecutor
    {
    public:
        virtual ~TaskExecutor() = default;

        virtual void Add(const char* name, TaskDelegate func) = 0;
        virtual void Add(TaskDelegate func) { Add("", func); };
    };

    // --------------------------------------------------------------------------------------------
    class ThreadPoolExecutor : public TaskExecutor
    {
    public:
        struct Config
        {
            uint32_t count{ 0 }; ///< Number of threads to spawn.
            uint64_t affinity{ 0 }; ///< Affinity to use for spawned threads. Only considered when non-zero.
            const char* name{ nullptr }; ///< Name to assign to spawned thread. Only considered when non-null.
        };

    public:
        ~ThreadPoolExecutor() noexcept { Shutdown(); }

        Result Startup(const Config& config);
        void Shutdown();

        void Add(const char* name, TaskDelegate func) override;

    private:
        bool Pump();

        static void PumpThread(void* data);

    protected:
        Vector<Thread> m_threads{};
        String m_threadName{};

        bool m_running{ false };
        std::deque<TaskDelegate> m_tasks{};

        Mutex m_mutex{};
        ConditionVariable m_cv{};
    };
}

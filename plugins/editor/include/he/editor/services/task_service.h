// Copyright Chad Engler

#pragma once

#include "he/core/atomic.h"
#include "he/core/delegate.h"
#include "he/core/intrusive_list.h"
#include "he/core/string_view.h"
#include "he/core/sync.h"
#include "he/core/task_executor.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"

#include <deque>

namespace he::editor
{
    class Task
    {
    public:
        virtual ~Task() = default;

        virtual const char* Name() const = 0;
        virtual float Progress() const { return -1.0f; }
        bool IsRunning() const { return m_taskIsRunning; }

        virtual void Run() = 0;

    public:
        IntrusiveListLink<Task> m_link{};

    protected:
        friend class TaskService;

        Atomic<bool> m_taskIsRunning{ false };
    };

    class TaskService : public TaskExecutor
    {
    public:
        bool Initialize();
        void Terminate();

        void Add(const char* name, TaskDelegate func) override;
        void Add(UniquePtr<Task> task);

        uint32_t PendingSize() const { return m_pendingTasks.Size(); }
        uint32_t RunningSize() const { return m_runningTasks.Size(); }

        bool IsEmpty() { return m_pendingTasks.IsEmpty() && m_runningTasks.IsEmpty(); }

        template <typename F>
        void ForEachRunning(F&& itr) const
        {
            LockGuard lock(m_mutex);

            for (const UniquePtr<Task>& ptr : m_runningTasks)
            {
                const Task& task = *ptr;
                if (!itr(task))
                    return;
            }
        }

        template <typename F>
        void ForEachPending(F&& itr) const
        {
            LockGuard lock(m_mutex);

            for (const UniquePtr<Task>& ptr : m_pendingTasks)
            {
                const Task& task = *ptr;
                if (!itr(task))
                    return;
            }
        }

    private:
        bool Pump();

        static void PumpThread(void* data);

    private:
        Vector<Thread> m_threads{};

        Mutex m_mutex{};
        ConditionVariable m_cv{};

        bool m_running HE_TSA_GUARDED_BY(m_mutex){ false };

        IntrusiveList<Task, &Task::m_link> m_runningTasks HE_TSA_GUARDED_BY(m_mutex){};
        IntrusiveList<Task, &Task::m_link> m_pendingTasks HE_TSA_GUARDED_BY(m_mutex){};
    };
}

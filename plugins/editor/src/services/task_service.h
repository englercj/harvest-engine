// Copyright Chad Engler

#pragma once

#include "he/core/delegate.h"
#include "he/core/string_view.h"
#include "he/core/sync.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"

#include <deque>
#include <functional>

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

    protected:
        friend class TaskService;

        std::atomic<bool> m_taskIsRunning{ false };
    };

    class TaskService : public TaskExecutor
    {
    public:
        bool Initialize();
        void Terminate();

        void Add(const char* name, TaskDelegate func) override;
        void Add(UniquePtr<Task> task);

        uint32_t PendingSize() const { return static_cast<uint32_t>(m_pendingTasks.size()); }
        uint32_t RunningSize() const { return static_cast<uint32_t>(m_runningTasks.size()); }

        bool IsEmpty() { return m_pendingTasks.empty() && m_runningTasks.empty(); }

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

        static void PumpThread(TaskService* service);

    private:
        Vector<std::thread> m_threads{};

        std::deque<UniquePtr<Task>> m_runningTasks{};
        std::deque<UniquePtr<Task>> m_pendingTasks{};

        bool m_running{ false };
        Mutex m_mutex{};
        ConditionVariable m_cv{};
    };
}

// Copyright Chad Engler

#pragma once

#include "he/core/delegate.h"
#include "he/core/string_view.h"
#include "he/core/sync.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"
#include "he/core/vector.h"

#include <deque>
#include <functional>

namespace he::editor
{
    using TaskDelegate = Delegate<void(float&)>;

    class TaskService : public TaskExecutor
    {
    public:
        struct TaskData
        {
            String name;
            TaskDelegate func;
            float progress;
        };

    public:
        TaskService() = default;

        bool Initialize();
        void Terminate();

        void Add(StringView name, TaskDelegate func);

        bool IsEmpty() const { return m_tasks.empty(); }

        template <typename F>
        void ForEach(F&& itr) const
        {
            LockGuard lock(m_mutex);
            for (const TaskData& data : m_tasks)
            {
                if (!itr(data))
                    break;
            }
        }

    private:
        void Add(he::TaskDelegate func) override;

        bool Pump();

        static void PumpThread(TaskService* service);

    private:
        Vector<std::thread> m_threads;
        std::deque<TaskData> m_tasks;

        bool m_running;
        Mutex m_mutex;
        ConditionVariable m_cv;
    };
}

// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/sync.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"
#include "he/core/vector.h"

#include <deque>
#include <functional>

namespace he::editor
{
    class TaskService : public TaskExecutor
    {
    public:
        using TaskFunc = std::function<void(float&)>;

        struct TaskData
        {
            String name;
            TaskFunc func;
            float progress;
        };

    public:
        TaskService() = default;

        bool Initialize();
        void Terminate();

        void Add(StringView name, TaskFunc func);

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
        void Add(TaskExecutor::TaskFunc func, void* taskData) override;

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

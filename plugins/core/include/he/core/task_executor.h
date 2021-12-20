// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/types.h"
#include "he/core/vector.h"

#include <deque>
#include <thread>
#include <condition_variable>

namespace he
{
    // --------------------------------------------------------------------------------------------
    class TaskExecutor
    {
    public:
        using TaskFunc = void(*)(void*);

    public:
        virtual ~TaskExecutor() {}

        virtual void Add(TaskFunc func, void* taskData) = 0;
    };

    // --------------------------------------------------------------------------------------------
    class ThreadPoolExecutor : public TaskExecutor
    {
    public:
        ~ThreadPoolExecutor() { Shutdown(); }

        Result Startup(uint32_t threadCount = 0, const char* name = nullptr);
        void Shutdown();

        void Add(TaskFunc func, void* taskData) override;

    private:
        bool Pump();

        static void PumpThread(ThreadPoolExecutor* executor);

        struct ThreadTask
        {
            TaskFunc func;
            void* taskData;
        };

    private:
        Vector<std::thread> m_threads;
        String m_threadName;

        bool m_running;
        std::deque<ThreadTask> m_tasks;
        std::mutex m_mutex;
        std::condition_variable m_cv;
    };
}

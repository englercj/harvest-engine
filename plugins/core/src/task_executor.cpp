// Copyright Chad Engler

#include "he/core/task_executor.h"

#include "he/core/thread.h"
#include "he/core/utils.h"

namespace he
{
    Result ThreadPoolExecutor::Startup(uint32_t threadCount, const char* name)
    {
        m_running = true;
        m_threadName = name ? name : "Thread Pool Executor";

        if (threadCount == 0)
        {
            const uint32_t totalThreads = std::thread::hardware_concurrency();
            threadCount = Clamp<uint32_t>(totalThreads, 2, 16) - 1;
        }

        m_threads.Reserve(threadCount);
        for (uint32_t i = 0; i < threadCount; ++i)
        {
            m_threads.PushBack(std::thread(PumpThread, this));
        }

        return Result::Success;
    }

    void ThreadPoolExecutor::Shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_running)
                return;

            m_running = false;
        }

        m_cv.notify_all();

        for (std::thread& t : m_threads)
            t.join();

        m_threads.Clear();
    }

    void ThreadPoolExecutor::Add(TaskFunc func, void* taskData)
    {
        if (!func)
            return;

        m_mutex.lock();
        m_tasks.push_back({ func, taskData });
        m_mutex.unlock();

        m_cv.notify_one();
    }

    bool ThreadPoolExecutor::Pump()
    {
        ThreadTask task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_cv.wait(lock, [this]()
            {
                return !m_tasks.empty() || !m_running;
            });

            if (m_tasks.empty())
                return false;

            task = m_tasks.front();
            m_tasks.pop_front();
        }

        task.func(task.taskData);
        return true;
    }

    void ThreadPoolExecutor::PumpThread(ThreadPoolExecutor* executor)
    {
        SetCurrentThreadName(executor->m_threadName.Data());

        while (executor->Pump()) {}
    }
}

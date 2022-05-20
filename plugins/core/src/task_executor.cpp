// Copyright Chad Engler

#include "he/core/task_executor.h"

#include "he/core/cpu.h"
#include "he/core/thread.h"
#include "he/core/utils.h"

namespace he
{
    Result ThreadPoolExecutor::Startup(const Config& config)
    {
        m_running = true;
        m_threadName = config.name ? config.name : "Thread Pool Executor";
        uint32_t count = config.count;

        if (count == 0)
            count = GetCpuInfo().threadCount;

        m_threads.Reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            m_threads.PushBack(std::thread(PumpThread, this));
            ThreadHandle handle = reinterpret_cast<ThreadHandle>(m_threads.Back().native_handle());
            Result r = SetThreadAffinity(handle, config.affinity);
            if (!r)
            {
                Shutdown();
                return r;
            }
        }

        return Result::Success;
    }

    void ThreadPoolExecutor::Shutdown()
    {
        {
            LockGuard lock(m_mutex);
            if (!m_running)
                return;

            m_running = false;
        }

        m_cv.WakeAll();

        for (std::thread& t : m_threads)
            t.join();

        m_threads.Clear();
    }

    void ThreadPoolExecutor::Add(TaskFunc func, void* taskData)
    {
        if (!func)
            return;

        m_mutex.Acquire();
        m_tasks.push_back({ func, taskData });
        m_mutex.Release();

        m_cv.WakeOne();
    }

    bool ThreadPoolExecutor::Pump()
    {
        ThreadTask task;

        {
            LockGuard lock(m_mutex);

            m_cv.Wait(lock, [this]()
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

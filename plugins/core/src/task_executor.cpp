// Copyright Chad Engler

#include "he/core/task_executor.h"

#include "he/core/cpu_info.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/thread.h"
#include "he/core/utils.h"

namespace he
{
    Result ThreadPoolExecutor::Startup(const Config& config)
    {
        m_running = true;
        m_threadName = config.name ? config.name : "[HE] Thread Pool Executor";
        uint32_t count = config.count;

        if (count == 0)
            count = GetCpuInfo().threadCount;

        m_threads.Reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            ThreadDesc desc;
            desc.proc = &PumpThread;
            desc.data = this;
            desc.affinity = config.affinity;

            Thread t;
            const Result rc = t.Start(desc);
            if (!HE_VERIFY(rc,
                HE_MSG("Failed to create thread pool thread."),
                HE_KV(result, rc)))
            {
                Shutdown();
                return rc;
            }

            m_threads.PushBack(Move(t));
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

        for (Thread& t : m_threads)
        {
            t.Join();
        }

        m_threads.Clear();
    }

    void ThreadPoolExecutor::Add(const char* name, TaskDelegate func)
    {
        HE_UNUSED(name);

        if (!func)
            return;

        m_mutex.Acquire();
        m_tasks.push_back(func);
        m_mutex.Release();

        m_cv.WakeOne();
    }

    bool ThreadPoolExecutor::Pump()
    {
        TaskDelegate task;

        {
            LockGuard lock(m_mutex);

            m_cv.Wait(lock, [this]()
            {
                return !m_tasks.empty() || !m_running;
            });

            if (m_tasks.empty())
                return m_running;

            task = m_tasks.front();
            m_tasks.pop_front();
        }

        task();
        return true;
    }

    void ThreadPoolExecutor::PumpThread(void* instance)
    {
        ThreadPoolExecutor* executor = static_cast<ThreadPoolExecutor*>(instance);
        Thread::SetName(executor->m_threadName.Data());

        while (executor->Pump()) {}
    }
}

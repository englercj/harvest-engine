// Copyright Chad Engler

#include "task_service.h"

#include "he/core/thread.h"
#include "he/core/utils.h"

namespace he::editor
{
    bool TaskService::Initialize()
    {
        m_running = true;

        const uint32_t cpuCount = GetCpuInfo().threadCount;
        const uint32_t threadCount = cpuCount <= 2 ? cpuCount : Max(cpuCount - 2, 2u);

        m_threads.Reserve(threadCount);
        for (uint32_t i = 0; i < threadCount; ++i)
        {
            m_threads.PushBack(std::thread(PumpThread, this));
        }

        return true;
    }

    void TaskService::Terminate()
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

    void TaskService::Add(StringView name, TaskDelegate func)
    {
        if (!func)
            return;

        TaskData data;
        data.name = name;
        data.progress = 0.0f;
        data.func = func;

        m_mutex.Acquire();
        m_tasks.push_back(Move(data));
        m_mutex.Release();

        m_cv.WakeOne();
    }

    static void _heTaskThunk(he::TaskDelegate cont, float&)
    {
        cont();
    }

    void TaskService::Add(he::TaskDelegate func)
    {
        TaskDelegate delegate = TaskDelegate::Make<&_heTaskThunk>(func);
        Add("Engine Task", delegate);
    }

    bool TaskService::Pump()
    {
        TaskData data;

        {
            LockGuard lock(m_mutex);

            m_cv.Wait(lock, [this]()
            {
                return !m_tasks.empty() || !m_running;
            });

            if (m_tasks.empty())
                return m_running;

            data = Move(m_tasks.front());
            m_tasks.pop_front();
        }

        data.func(data.progress);
        data.progress = 1.0f;
        return true;
    }

    void TaskService::PumpThread(TaskService* service)
    {
        SetCurrentThreadName("[HE] Editor Task Service Thread");

        while (service->Pump()) {}
    }
}

// Copyright Chad Engler

#include "he/editor/services/task_service.h"

#include "he/core/cpu_info.h"
#include "he/core/thread.h"
#include "he/core/unique_ptr.h"
#include "he/core/utils.h"

namespace he::editor
{
    class TaskWrapper : public Task
    {
    public:
        TaskWrapper(const char* name, TaskDelegate func)
            : m_name(name)
            , m_func(func)
        {}

        const char* Name() const override { return m_name.Data(); }
        void Run() override { m_func(); }

    private:
        String m_name;
        TaskDelegate m_func;
    };

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

    void TaskService::Add(const char* name, TaskDelegate func)
    {
        if (!func)
            return;

        Add(MakeUnique<TaskWrapper>(name, func));
    }

    void TaskService::Add(UniquePtr<Task> task)
    {
        m_mutex.Acquire();
        m_pendingTasks.push_back(Move(task));
        m_mutex.Release();

        m_cv.WakeOne();
    }

    bool TaskService::Pump()
    {
        // TODO: Use some lock-free algos in here, we're currently thrashing this lock both here
        // and in the ForEach method.

        Task* task = nullptr;

        {
            LockGuard lock(m_mutex);

            m_cv.Wait(lock, [this]()
            {
                return !m_pendingTasks.empty() || !m_running;
            });

            if (m_pendingTasks.empty())
                return m_running;

            m_runningTasks.push_back(Move(m_pendingTasks.front()));
            m_pendingTasks.pop_front();

            task = m_runningTasks.back().Get();
        }

        task->m_taskIsRunning = true;
        task->Run();

        {
            LockGuard lock(m_mutex);

            m_runningTasks.erase(std::remove_if(m_runningTasks.begin(), m_runningTasks.end(), [&](const UniquePtr<Task>& x)
            {
                return task == x.Get();
            }));
        }

        return true;
    }

    void TaskService::PumpThread(TaskService* service)
    {
        SetCurrentThreadName("[HE] Editor Task Service Thread");

        while (service->Pump()) {}
    }
}

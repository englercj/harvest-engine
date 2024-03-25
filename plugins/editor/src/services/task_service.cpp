// Copyright Chad Engler

#include "he/editor/services/task_service.h"

#include "he/core/cpu_info.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
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
            ThreadDesc desc;
            desc.proc = &PumpThread;
            desc.data = this;

            Thread t;
            const Result rc = t.Start(desc);
            if (!HE_VERIFY(rc,
                HE_MSG("Failed to create task service thread."),
                HE_KV(result, rc)))
            {
                Terminate();
                return false;
            }

            m_threads.PushBack(Move(t));
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

        for (Thread& t : m_threads)
        {
            t.Join();
        }

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
        m_pendingTasks.PushBack(task.Release());
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
                return !m_pendingTasks.IsEmpty() || !m_running;
            });

            if (m_pendingTasks.IsEmpty())
                return m_running;

            task = m_pendingTasks.Front();
            m_pendingTasks.Remove(task);
            m_runningTasks.PushBack(task);
        }

        task->m_taskIsRunning = true;
        task->Run();

        {
            LockGuard lock(m_mutex);
            m_runningTasks.Remove(task);
            Allocator::GetDefault().Delete(task);
        }

        return true;
    }

    void TaskService::PumpThread(void* data)
    {
        TaskService* service = static_cast<TaskService*>(data);
        Thread::SetName("[HE] Editor Task Service Thread");

        while (service->Pump()) {}
    }
}

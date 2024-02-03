// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/assert.h"

namespace he
{
    Thread::~Thread()
    {
        HE_ASSERT(m_handle == nullptr, HE_MSG("Thread object destroyed without joining or detaching."));
    }

    Thread::Thread(Thread&& x) noexcept
        : m_handle(Exchange(x.m_handle, nullptr))
    {
    }

    Thread& Thread::operator=(Thread&& x) noexcept
    {
        if (this != &x)
        {
            m_handle = Exchange(x.m_handle, nullptr);
        }

        return *this;
    }
}

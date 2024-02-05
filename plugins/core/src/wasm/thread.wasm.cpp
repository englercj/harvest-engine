// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/assert.h"
#include "he/core/atomic.h"
#include "he/core/clock.h"
#include "he/core/compiler.h"
#include "he/core/log.h"
#include "he/core/sync.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_WASM)

#include "thread_internal.wasm.h"
#include "he/core/wasm/lib_core.wasm.h"

namespace he
{
    static Atomic<uint32_t> s_nextThreadId{ 1 };
    static _WasmThreadState s_mainThreadState{};
    static IntrusiveList<_WasmThreadState, &_WasmThreadState::link> s_threadStates{};
    static Mutex s_threadStatesMutex{};

    static thread_local _WasmThreadState* s_localThreadState = nullptr;

    void _InitializeMainThread()
    {
        uint8_t* tlsMem = nullptr;
        const size_t tlsSize = __builtin_wasm_tls_size();
        if (tlsSize)
        {
            tlsMem = Allocator::GetDefault().Malloc(tlsSize, __builtin_wasm_tls_align());
            MemZero(mem, tlsSize);
            __wasm_init_tls(tlsMem);
        }

        s_mainThreadState.id = s_nextThreadId.FetchAdd(1);
        s_mainThreadState.isMain = true;
        s_mainThreadState.canWait = !heWASM_IsWeb(); // Main thread cannot wait on the web, but on Node it can.

        s_mainThreadState.memBase = tlsMem;
        s_mainThreadState.tlsBase = tlsMem;
        s_mainThreadState.stackBase = nullptr;
        s_mainThreadState.memSize = __builtin_wasm_tls_size();

        s_threadStatesMutex.Lock();
        s_threadStates.PushBack(&s_mainThreadState);
        s_threadStatesMutex.Unlock();

        s_localThreadState = &s_mainThreadState;
    }

    void _TerminateMainThread()
    {
        s_threadStatesMutex.Lock();
        s_threadStates.Remove(&s_mainThreadState);
        s_threadStatesMutex.Unlock();

        Allocator::GetDefault().Free(s_mainThreadState.memBase);

        s_mainThreadState.memBase = nullptr;
        s_mainThreadState.tlsBase = nullptr;
        s_mainThreadState.stackBase = nullptr;
        s_mainThreadState.memSize = 0;
    }

    bool _CanCurentThreadWait()
    {
        return s_localThreadState->canWait;
    }

    int32_t _AtomicWaitCurrentThread(int32_t* value, int32_t expected, int64_t timeoutNs)
    {
        // memory.atomic.wait32 returns:
        //   0 => "ok", woken by another agent.
        //   1 => "not-equal", loaded value != expected value
        //   2 => "timed-out", the timeout expired
        if (_CanCurentThreadWait())
        {
            return __builtin_wasm_atomic_wait_i32(value, expected, timeoutNs);
        }

        return *value == expected ? 0 : 1;
    }

    uint32_t Thread::GetId()
    {
        return s_localThreadState->id;
    }

    void Thread::SetName(const char* name)
    {
        s_localThreadState->name = name;
    }

    void Thread::Sleep(Duration amount)
    {
        static int32_t s_dummy = 0;
        _AtomicWaitCurrentThread(&s_dummy, 0, amount.val);
    }

    void Thread::Yield()
    {
        static int32_t s_dummy = 0;
        _AtomicWaitCurrentThread(&s_dummy, 0, 100);
    }

    Result Thread::Start(const ThreadDesc& desc)
    {
        if (!HE_VERIFY(desc.proc != nullptr, HE_MSG("Thread procedure must be non-null.")))
            return Result::InvalidParameter;

        if (!HE_VERIFY(!m_handle, HE_MSG("Thread is already running.")))
            return Result::InvalidParameter;

        // Calculate the size of the memory block we need to allocate for the thread.
        // This is the size of the thread state, TLS memory, stack, and alignment padding.
        size_t size = sizeof(_WasmThreadState);
        if (__builtin_wasm_tls_size())
        {
            size += __builtin_wasm_tls_size() + __builtin_wasm_tls_align();
        }
        const size_t stackSize = desc.stackSize ? desc.stackSize : 64 * 1024;
        size += stackSize + alignof(MaxAlign);

        uint8_t* mem = static_cast<uint8_t*>(Allocator::GetDefault().Malloc(size, alignof(_WasmThreadState)));

        _WasmThreadState* state = ::new(mem) _WasmThreadState();

        state->id = s_nextThreadId.FetchAdd(1);
        state->isMain = false;
        state->canWait = true;

        // Store the memory block so we can free it later.
        state->memBase = mem;
        state->memSize = size;
        mem += sizeof(_WasmThreadState);

        // Store the TLS memory pointer, and zero initialize the TLS memory.
        const size_t tlsSize = __builtin_wasm_tls_size();
        if (tlsSize))
        {
            mem = AlignUp(mem, __builtin_wasm_tls_align());
            MemZero(mem, tlsSize);
            state->tlsBase = mem;
            mem += tlsSize;
        }

        // Store the stack pointer for the thread.
        // Wasm stack grows 'down', so we start the stack pointer at the highest address.
        mem = AlignUp(mem + stackSize, alignof(MaxAlign));
        state->stackBase = mem;

        // Ensure we didn't use more memory than we allocated.
        HE_ASSERT(mem < (static_cast<uint8_t*>(state->memBase) + size));

        s_threadStatesMutex.Lock();
        s_threadStates.PushBack(state);
        s_threadStatesMutex.Unlock();

        // Spawn the actual Worker instance in JS-land.
        const int rc = heWASM_CreateThread(state, desc.proc, desc.data);
        if (rc != 0)
        {
            s_threadStatesMutex.Lock();
            s_threadStates.Remove(state);
            s_threadStatesMutex.Unlock();

            void* memBase = state->memBase;
            state->~_WasmThreadState();
            Allocator::GetDefault().Free(memBase);
            return PosixResult(rc);
        }

        m_handle = state;
        return Result::Success;
    }

    Result Thread::Join()
    {
        // TODO
    }

    Result Thread::Detach()
    {
        // TODO
    }

    Result Thread::SetAffinity(uint64_t mask)
    {
        // Wasm doesn't have thread affinity support.
        return Result::NotSupported;
    }
}

extern "C"
{
    // Loads the TLS memory for the current thread. This function is generated by the linker.
    extern void __wasm_init_tls(void *memory);

    HE_EXPORT void _heRunThread(_WasmThreadState* state, Pfn_ThreadProc proc, void* arg)
    {
        __wasm_init_tls(state->tlsBase);

        he::s_localThreadState = state;

        proc(arg);

        he::s_threadStatesMutex.Lock();
        he::s_threadStates.Remove(state);
        he::s_threadStatesMutex.Unlock();

        he::s_localThreadState = nullptr;

        state->~_WasmThreadState();
        he::Allocator::GetDefault().Free(memBase);
    }
}

#endif

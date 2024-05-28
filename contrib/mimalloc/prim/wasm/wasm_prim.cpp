// Copyright Chad Engler

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/memory_ops.h"
#include "he/core/process.h"
#include "he/core/random.h"
#include "he/core/string.h"
#include "he/core/thread.h"
#include "he/core/wasm/lib_core.wasm.h"

#include "mimalloc.h"
#include "mimalloc/internal.h"

extern "C"
{
    uint64_t _he_prim_clock_now()
    {
        return he::MonotonicClock::Now().val / 1000000;
    }

    void _he_prim_out_stderr(const char* msg)
    {
        heWASM_ConsoleLog(heWASM_ConsoleLogLevel::Error, msg);
    }

    bool _he_prim_getenv(const char* name, char* result, size_t resultLen)
    {
        he::String value;
        he::Result rc = he::GetEnv(name, value);
        if (rc && value.Size() < resultLen)
        {
            he::MemCopy(result, value.Data(), value.Size() + 1);
            return true;
        }
        return false;
    }

    bool _he_prim_random_buf(void* buf, size_t len)
    {
        return he::GetSecureRandomBytes(static_cast<uint8_t*>(buf), len);
    }

    static he::TlsValue _he_heap_thread_value;

    static void _he_wasm_thread_done(void* value)
    {
        if (value != NULL)
        {
            _mi_thread_done((mi_heap_t*)value);
        }
    }

    void _he_prim_thread_init_auto_done()
    {
        HE_ASSERT(!_he_heap_thread_value.IsValid());
        _he_heap_thread_value.Create(&_he_wasm_thread_done);
    }

    void _he_prim_thread_done_auto_done()
    {
        _he_heap_thread_value.Destroy();
    }

    void _he_prim_thread_associate_default_heap(mi_heap_t* heap)
    {
        _he_heap_thread_value.Set(heap);
    }
}

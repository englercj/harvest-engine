// Copyright Chad Engler

#include "stdlib.h"

#include "errno.h"
#include "signal.h"

#include "wasm/libc.wasm.h"

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/hash_table.h"
#include "he/core/memory_ops.h"
#include "he/core/random.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/wasm/lib_core.wasm.h"

extern "C"
{
    static he::Random64 s_rand;
    static he::Vector<void(*)(void)> s_atExitFuncs;
    static he::Vector<void(*)(void)> s_atQuickExitFuncs;
    static he::HashMap<he::String, he::String> s_environ;

    // --------------------------------------------------------------------------------------------
    int atoi(const char* s)
    {
        return he::StrToInt<int>(s, nullptr, 10);
    }

    long atol(const char* s)
    {
        return he::StrToInt<long>(s, nullptr, 10);
    }

    long long atoll(const char* s)
    {
        return he::StrToInt<long long>(s, nullptr, 10);
    }

    double atof(const char* s)
    {
        return he::StrToFloat<double>(s);
    }

    // --------------------------------------------------------------------------------------------
    int rand(void)
    {
        return static_cast<int>(s_rand.Next(0, RAND_MAX));
    }

    void srand(unsigned seed)
    {
        s_rand.m_state = static_cast<uint64_t>(seed);
    }

    // --------------------------------------------------------------------------------------------
    float strtof(const char* __restrict s, char** __restrict end)
    {
        return he::StrToFloat<float>(static_cast<const char*>(s), const_cast<const char**>(end));
    }

    double strtod(const char* __restrict s, char** __restrict end)
    {
        return he::StrToFloat<double>(static_cast<const char*>(s), const_cast<const char**>(end));
    }

    long double strtold(const char* __restrict s, char** __restrict end)
    {
        return he::StrToFloat<long double>(static_cast<const char*>(s), const_cast<const char**>(end));
    }

    // --------------------------------------------------------------------------------------------
    long strtol(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<long>(static_cast<const char*>(s), const_cast<const char**>(end), base);
    }

    unsigned long strtoul(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<unsigned long>(static_cast<const char*>(s), const_cast<const char**>(end), base);
    }

    long long strtoll(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<long long>(static_cast<const char*>(s), const_cast<const char**>(end), base);
    }

    unsigned long long strtoull(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<unsigned long long>(static_cast<const char*>(s), const_cast<const char**>(end), base);
    }

    // --------------------------------------------------------------------------------------------
    void* malloc(size_t size)
    {
        he::Allocator& allocator = he::Allocator::GetDefault();
        return allocator.Malloc(size);
    }

    void* calloc(size_t num, size_t size)
    {
        he::Allocator& allocator = he::Allocator::GetDefault();
        void* mem = allocator.Malloc(num * size);
        he::MemZero(mem, num * size);
        return mem;
    }

    void* realloc(void* ptr, size_t size)
    {
        he::Allocator& allocator = he::Allocator::GetDefault();
        return allocator.Realloc(ptr, size);
    }

    void free(void* ptr)
    {
        he::Allocator& allocator = he::Allocator::GetDefault();
        allocator.Free(ptr);
    }

    void* aligned_alloc(size_t size, size_t alignment)
    {
        he::Allocator& allocator = he::Allocator::GetDefault();
        return allocator.Malloc(size, alignment);
    }

    // --------------------------------------------------------------------------------------------
    __attribute__((__noreturn__)) void abort()
    {
        raise(SIGABRT);

        // Cause an unconditional trap to terminate the process.
        __builtin_trap();
    }

    int atexit(void(*proc)(void))
    {
        if (proc)
        {
            s_atExitFuncs.PushBack(proc);
        }
        return 0;
    }

    __attribute__((__noreturn__)) void exit(int rc)
    {
        for (uint32_t i = s_atExitFuncs.Size() - 1; i != static_cast<uint32_t>(-1); --i)
        {
            s_atExitFuncs[i]();
        }
        _Exit(rc);
    }

    __attribute__((__noreturn__)) void _Exit(int rc)
    {
        heWASM_SetExitCode(rc);

        // Cause an unconditional trap to terminate the process.
        // Not quite right, but since we registered our exist code beforehand, should be OK.
        __builtin_trap();
    }

    int at_quick_exit(void(*proc)(void))
    {
        if (proc)
        {
            s_atQuickExitFuncs.PushBack(proc);
        }
        return 0;
    }

    __attribute__((__noreturn__)) void quick_exit(int rc)
    {
        for (uint32_t i = s_atQuickExitFuncs.Size() - 1; i != static_cast<uint32_t>(-1); --i)
        {
            s_atQuickExitFuncs[i]();
        }
        _Exit(rc);
    }

    // --------------------------------------------------------------------------------------------
    char* getenv([[maybe_unused]] const char* name)
    {
        if (he::StrEmpty(name))
            return nullptr;

        const he::String* value = s_environ.Find(name);
        return value ? const_cast<char*>(value->Data()) : nullptr;
    }

    int setenv(const char *name, const char *value, int overwrite)
    {
        if (he::StrEmpty(name))
        {
            errno = EINVAL;
            return -1;
        }

        if (overwrite)
            s_environ.EmplaceOrAssign(name, value);
        else
            s_environ.Emplace(name, value);

        return 0;
    }

    int unsetenv(const char* name)
    {
        if (he::StrEmpty(name))
        {
            errno = EINVAL;
            return -1;
        }

        s_environ.Erase(name);
        return 0;
    }

    // --------------------------------------------------------------------------------------------
    int abs(int x)
    {
        return x < 0 ? -x : x;
    }

    long labs(long x)
    {
        return x < 0 ? -x : x;
    }

    long long llabs(long long x)
    {
        return x < 0 ? -x : x;
    }
}

// Copyright Chad Engler

#include "stdlib.h"

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"

extern "C"
{
    // --------------------------------------------------------------------------------------------
    float strtof(const char* __restrict s, char** __restrict end)
    {
        return he::StrToFloat<float>(static_cast<const char*>(s), static_cast<const char**>(end));
    }

    double strtod(const char* __restrict s, char** __restrict end)
    {
        return he::StrToFloat<double>(static_cast<const char*>(s), static_cast<const char**>(end));
    }

    long double strtold(const char* __restrict s, char** __restrict end)
    {
        return he::StrToFloat<long double>(static_cast<const char*>(s), static_cast<const char**>(end));
    }

    long strtol(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<long>(static_cast<const char*>(s), static_cast<const char**>(end), base);
    }

    unsigned long strtoul(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<unsigned long>(static_cast<const char*>(s), static_cast<const char**>(end), base);
    }

    long long strtoll(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<long long>(static_cast<const char*>(s), static_cast<const char**>(end), base);
    }

    unsigned long long strtoull(const char* __restrict s, char** __restrict end, int base)
    {
        return he::StrToInt<unsigned long long>(static_cast<const char*>(s), static_cast<const char**>(end), base);
    }

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

    __attribute__((__noreturn__)) void abort()
    {
        // Cause an unconditional trap to terminate the process.
        HE_UNREACHABLE();
    }
}

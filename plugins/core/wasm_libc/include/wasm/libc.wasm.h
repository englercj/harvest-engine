// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/wasm.h"

#define HE_WASM_IMPORT_LIBC(name) HE_WASM_IMPORT(libc, name)

extern "C"
{
    struct tm;

    extern void HE_WASM_IMPORT_LIBC(heWASM_TzSet)(int* tz, int* dst, char* stdName, uint32_t stdNameLen, char* dstName, uint32_t dstNameLen);
    extern int HE_WASM_IMPORT_LIBC(heWASM_MkTime)(tm* t);
    extern int HE_WASM_IMPORT_LIBC(heWASM_TimeGm)(tm* t);
    extern void HE_WASM_IMPORT_LIBC(heWASM_GmTime)(int time, tm* t);
    extern void HE_WASM_IMPORT_LIBC(heWASM_LocalTime)(int time, tm* t);
    extern void HE_WASM_IMPORT_LIBC(heWASM_StdIoWrite)(int fd, const char* src, uint32_t len);
}

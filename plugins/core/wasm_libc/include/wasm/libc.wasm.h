// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/wasm.h"

#define HE_WASM_IMPORT_LIBC(name) HE_WASM_IMPORT(libc, name)

extern "C"
{
    struct tm;

    extern void HE_WASM_IMPORT_LIBC(heWASM_TzSet)(int* tz, int* dst, char* stdName, char* dstName);
    extern int HE_WASM_IMPORT_LIBC(heWASM_MkTime)(tm* t);
    extern int HE_WASM_IMPORT_LIBC(heWASM_TimeGm)(tm* t);
    extern void HE_WASM_IMPORT_LIBC(heWASM_GmTime)(int time, tm* t);
    extern void HE_WASM_IMPORT_LIBC(heWASM_LocalTime)(int time, tm* t);

    extern double HE_WASM_IMPORT_LIBC(heWASM_Acos)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Asin)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Atan)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Atan2)(double y, double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Cos)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Exp)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Mod)(double x, double y);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Log)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Log10)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Log1p)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Log2)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Pow)(double x, double y);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Round)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Sin)(double x);
    extern double HE_WASM_IMPORT_LIBC(heWASM_Tan)(double x);

    extern void HE_WASM_IMPORT_LIBC(heWASM_StdIoWrite)(int fd, const char* src, uint32_t len);
}

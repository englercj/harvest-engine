// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/wasm.h"

#define HE_WASM_IMPORT_CORE(name) HE_WASM_IMPORT(core, name)

extern "C"
{
    enum class heWASM_ConsoleLogLevel : uint8_t
    {
        Log     = 0,
        Debug   = 1,
        Info    = 2,
        Warn    = 3,
        Error   = 4,
    };

    extern void HE_WASM_IMPORT_CORE(heWASM_Alert)(const char* msg);
    extern void HE_WASM_IMPORT_CORE(heWASM_ConsoleLog)(heWASM_ConsoleLogLevel level, const char* msg);
    extern void HE_WASM_IMPORT_CORE(heWASM_Debugger)();
    extern void HE_WASM_IMPORT_CORE(heWASM_Eval)(const char* code);
    extern void HE_WASM_IMPORT_CORE(heWASM_SetExitCode)(int rc);
    extern uint32_t HE_WASM_IMPORT_CORE(heWASM_GetDateNow)();
    extern uint32_t HE_WASM_IMPORT_CORE(heWASM_GetDateTzOffset)();
    extern double HE_WASM_IMPORT_CORE(heWASM_GetPerformanceNow)();
    extern uint32_t HE_WASM_IMPORT_CORE(heWASM_GetHardwareConcurrency)();
    extern void HE_WASM_IMPORT_CORE(heWASM_GetRandomBytes)(uint8_t* dst, uint32_t dstLen);
    extern bool HE_WASM_IMPORT_CORE(heWASM_IsDaylightSavingTimeActive)();
    extern bool HE_WASM_IMPORT_CORE(heWASM_IsWeb)();
    extern uint32_t HE_WASM_IMPORT_CORE(heWASM_GetUserAgentLength)();
    extern void HE_WASM_IMPORT_CORE(heWASM_GetUserAgent)(char* dst, uint32_t dstLen);
    extern bool HE_WASM_IMPORT_CORE(heWASM_GetBatteryStatus)(double* chargingTime, double* dischargingTime, double* level, bool* charging);
    extern bool HE_WASM_IMPORT_CORE(heWASM_CreateThread)(void* state, void(*proc)(void*), void* data);

    extern double HE_WASM_IMPORT_CORE(heWASM_Sin)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Asin)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Cos)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Acos)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Tan)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Atan)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Atan2)(double y, double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Exp)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Mod)(double x, double y);
    extern double HE_WASM_IMPORT_CORE(heWASM_Log)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Log10)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Log1p)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Log2)(double x);
    extern double HE_WASM_IMPORT_CORE(heWASM_Pow)(double x, double y);
}

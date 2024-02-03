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
    extern bool HE_WASM_IMPORT_CORE(heWASM_CreateThread)(void* proc, void* data);
}

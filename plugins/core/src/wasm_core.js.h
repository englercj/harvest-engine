// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/wasm.h"

extern "C"
{
    enum class heWASM_ConsoleLogLevel : uint8_t
    {
        Log,
        Debug,
        Info,
        Warn,
        Error,
    };

    extern void HE_WASM_IMPORT(heWASM_Alert)(const char* msg);
    extern void HE_WASM_IMPORT(heWASM_ConsoleLog)(heWASM_ConsoleLogLevel level, const char* msg);
    extern void HE_WASM_IMPORT(heWASM_Debugger)();
    extern void HE_WASM_IMPORT(heWASM_Eval)(const char* code);
    extern uint32_t HE_WASM_IMPORT(heWASM_GetDateNow)();
    extern uint32_t HE_WASM_IMPORT(heWASM_GetDateTzOffset)();
    extern double HE_WASM_IMPORT(heWASM_GetPerformanceNow)();
    extern uint32_t HE_WASM_IMPORT(heWASM_GetHardwareConcurrency)();
    extern void HE_WASM_IMPORT(heWASM_GetRandomBytes)(uint8_t* dst, uint32_t dstLen);
    extern bool HE_WASM_IMPORT(heWASM_IsDaylightSavingTimeActive)();
    extern bool HE_WASM_IMPORT(heWASM_IsWeb)();

    extern uint32_t HE_WASM_IMPORT(heWASM_GetUserAgentLength)();
    extern void HE_WASM_IMPORT(heWASM_GetUserAgent)(char* dst, uint32_t dstLen);
    extern bool HE_WASM_IMPORT(heWASM_GetBatteryStatus)(double* chargingTime, double* dischargingTime, double* level, bool* charging);
}

// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/wasm.h"

#define HE_WASM_IMPORT_WINDOW(name) HE_WASM_IMPORT(window, name)

extern "C"
{
    extern void HE_WASM_IMPORT_WINDOW(heWASM_FunctionName)();
}

// Copyright Chad Engler

#pragma once

#include "he/core/config.h"
#include "he/core/types.h"

#if defined(HE_PLATFORM_API_WASM)
    #define HE_WASM_IMPORT(module, name) __attribute__((import_module(#module), import_name(#name))) name
#else
    #define HE_WASM_IMPORT(module, name) name
#endif

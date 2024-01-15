// Copyright Chad Engler

#pragma once

#include "he/core/config.h"
#include "he/core/types.h"

#if defined(HE_PLATFORM_WASM)
    #define HE_WASM_IMPORT(name) __attribute__((import_module("env"), import_name(#name)))) name
#else
    #define HE_WASM_IMPORT(name) name
#endif

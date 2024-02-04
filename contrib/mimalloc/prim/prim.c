/* ----------------------------------------------------------------------------
Copyright (c) 2018-2023, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

// Select the implementation of the primitives
// depending on the OS.

#if defined(__WASM__)
#include "prim/wasm/prim.c"      // memory-grow (Wasm)

#elif defined(_WIN32)
#include "prim/windows/prim.c"  // VirtualAlloc (Windows)

#elif defined(__APPLE__)
#include "prim/osx/prim.c"      // macOSX (actually defers to mmap in unix/prim.c)

#elif defined(__wasi__)
#define MI_USE_SBRK
#include "prim/wasi/prim.c"     // memory-grow or sbrk (Wasm)

#else
#include "prim/unix/prim.c"     // mmap() (Linux, macOSX, BSD, Illumnos, Haiku, DragonFly, etc.)

#endif

// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"

#include <type_traits>

/// \def HE_ALLOCA_RAW(len)
/// Allocate `len` bytes on the stack of the current function scope.
/// Do not use this macro directly. Use the \ref HE_ALLOCA macro instead.
///
/// \param len The number of bytes to allocate.
#if HE_COMPILER_MSVC
    extern "C" void* __cdecl _alloca(size_t);
    #pragma intrinsic(_alloca)

    #define HE_ALLOCA_RAW(len) _alloca(len)
#else
    #define HE_ALLOCA_RAW(len) __builtin_alloca(len)
#endif

/// Allocate `len` number of trivially destructable type `T` on the stack.
///
/// \note Constructors and destructors are not called.
///
/// \param T The type to allocate. Must be trivially destructable.
/// \param len The number of `T` objects to allocate.
#define HE_ALLOCA(T, len) static_cast<std::enable_if_t<std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>, T*>>(HE_ALLOCA_RAW((len) * sizeof(T)))

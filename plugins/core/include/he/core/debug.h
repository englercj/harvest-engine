// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/appender.h"
#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/cpu.h"
#include "he/core/macros.h"
#include "he/core/string.h"
#include "he/core/utils.h"

#include "fmt/format.h"

/// \def HE_DEBUG_BREAK()
/// Causes a breakpoint in the code, where the user will be prompted to run the debugger.

/// \def HE_FILE
/// The current file name. This value is always empty for non-internal builds.

/// \def HE_LINE
/// The current line number. This value is always zero for non-internal builds.

#if HE_INTERNAL_BUILD
    #if defined(__INTELLISENSE__)
        #define HE_DEBUG_BREAK() (false)
    #elif defined(HE_PLATFORM_EMSCRIPTEN)
        extern "C" void emscripten_debugger(void);
        #define HE_DEBUG_BREAK() (emscripten_debugger(), false)
    #elif HE_COMPILER_CLANG
        #define HE_DEBUG_BREAK() (__builtin_debugtrap(), false)
    #elif HE_COMPILER_MSVC
        #define HE_DEBUG_BREAK() (__debugbreak(), false)
    #elif HE_COMPILER_GCC
        __attribute__((always_inline, artificial)) inline void _he_debugtrap()
        {
        #if HE_CPU_X86
            __asm__ volatile("int $3" ::: "memory");
        #elif HE_CPU_ARM_32
            __asm__ volatile(".inst 0xe7f001f0" ::: "memory");
        #elif HE_CPU_ARM_64
            __asm__ volatile(".inst 0xd4200000" ::: "memory");
        #endif
        }
        #define HE_DEBUG_BREAK() (_he_debugtrap(), false)
    #else
        #define HE_DEBUG_BREAK() (false)
    #endif

    #define HE_FILE __FILE__

    #if HE_COMPILER_MSVC
        // MSVC has a "feature" that treats __LINE__ like a variable for Edit-and-continue.
        // Since we definitely want a literal value we append 'U' to it to force it to be a
        // numeric literal.
        #define HE_LINE HE_PP_JOIN(__LINE__, U)
    #else
        #define HE_LINE __LINE__
    #endif
#else
    #define HE_DEBUG_BREAK()
    #define HE_FILE ""
    #define HE_LINE 0
#endif

namespace he
{
    /// Outputs `s` to the debugger output. On Windows this will output to the debug channel
    /// even when the debugger is not attached. You can see such messages using DebugView.
    ///
    /// \param s The string to print.
    void PrintToDebugger(const char* s);

    /// Outputs the formatted string to the debugger output.
    ///
    /// \param fmt The format string.
    /// \param args The arguments for the format string.
    template <typename... Args>
    void PrintToDebugger(fmt::format_string<Args...> fmt, Args&&... args)
    {
        String buf(Allocator::GetTemp());
        fmt::format_to(Appender(buf), fmt, Forward<Args>(args)...);
        return PrintToDebugger(buf.Data());
    }

    /// Checks if the debugger is currently attached to this program and returns the result.
    ///
    /// \note Not all platforms support checking this value, in which case false is returned.
    ///
    /// \return True if the debugger is currently attached.
    bool IsDebuggerAttached();
}

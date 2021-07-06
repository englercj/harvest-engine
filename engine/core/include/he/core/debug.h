#pragma once

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/cpu.h"
#include "he/core/platform.h"
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
    #elif HE_PLATFORM_EMSCRIPTEN
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
    #define HE_LINE __LINE__
#else
    #define HE_DEBUG_BREAK()
    #define HE_FILE ""
    #define HE_LINE 0
#endif

namespace he
{
    /// Outputs `s` to the debugger output. On MSVC this will output to the debug channel
    /// even when the debugger is not attached. You can see such messages using DebugView.
    ///
    /// \param s The string to output
    void OutputToDebugger(const char* s);

    /// Outputs the formatted string to the debugger output.
    ///
    /// \param fmt The format string
    /// \param args The arguments for the format string
    template <typename... Args>
    void OutputToDebugger(const char* fmt, Args&&... args)
    {
        fmt::memory_buffer buf;
        fmt::format_to(buf, fmt, Forward<Args>(args)...);
        buf.push_back('\0');
        return OutputToDebugger(buf.data());
    }

    // Returns `true` if the debugger is currently attached
    bool IsDebuggerAttached();
}

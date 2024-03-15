// Copyright Chad Engler

#include "he/core/stack_trace.h"

#if defined(HE_PLATFORM_API_WASM)

#include "he/core/wasm/lib_core.wasm.h"

namespace he
{
    Result CaptureStackTrace([[maybe_unused]] uintptr_t* frames, [[maybe_unused]] uint32_t& count, [[maybe_unused]] uint32_t skipCount)
    {
        // TODO: I think we can do this by reading `new Error().stack` in the JS.
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/stack
        return Result::NotSupported;
    }

    Result GetSymbolInfo([[maybe_unused]] uintptr_t frame, [[maybe_unused]] SymbolInfo& info)
    {
        // TODO: Stacks generated with `new Error().stack` might always be symbolized by the
        // browser if the debug symbols are available. Will have to do some tests.
        return Result::NotSupported;
    }
}

#endif

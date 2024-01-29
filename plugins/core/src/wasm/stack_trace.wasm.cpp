// Copyright Chad Engler

#include "he/core/stack_trace.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm/lib_core.wasm.h"

namespace he
{
    Result CaptureStackTrace(uintptr_t* frames, uint32_t& count, uint32_t skipCount)
    {
        // TODO: I think we can do this by reading `new Error().stack` in the JS.
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Error/stack
        return Result::NotSupported;
    }

    Result GetSymbolInfo(uintptr_t frame, SymbolInfo& info)
    {
        // TODO: Stacks generated with `new Error().stack` might always be symbolized by the
        // browser if the debug symbols are available. Will have to do some tests.
        return Result::NotSupported;
    }
}

#endif

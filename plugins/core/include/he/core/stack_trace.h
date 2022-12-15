// Copyright Chad Engler

#pragma once

#include "he/core/delegate.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    /// Structure containing debug information about a symbol
    struct SymbolInfo
    {
        String name;        ///< The name of the symbol.
        String file;        ///< The source file the symbol is defined in.
        uint32_t line;      ///< The line number within the source file the symbol is defined on.
        uint32_t column;    ///< The column number within the line the symbol is defined on.
    };

    /// Walks up at most `count` frames of the stack and writes pointers to each into `frames`.
    /// Optionally skips `skipCount` frames at the top of the stack.
    ///
    /// \note The resulting frame array will start at the caller of this function. That is, this
    /// function will not be included in the array of stack frames.
    ///
    /// \param[out] frames The array of stack frame pointers to fill in.
    /// \param[in,out] count The length of the stack frame pointers array. When success is
    ///     returned the number of actual frames read is written to this parameter.
    /// \param[in] skipCount Optional. The number of frames to skip at the top of the stack. The
    ///     default is zero, however this function's frame is always skipped.
    /// \return The result of the operation.
    HE_NO_INLINE Result CaptureStackTrace(uintptr_t* frames, uint32_t& count, uint32_t skipCount = 0);

    /// Given a frame pointer address resolve the symbol information for that function.
    ///
    /// \param[in] frame The frame pointer address, usually from \ref GatherFramePointers
    /// \param[out] info The symbol info structure to fill with this frame's data.
    /// \return The result of the operation.
    Result GetSymbolInfo(uintptr_t frame, SymbolInfo& info);
}

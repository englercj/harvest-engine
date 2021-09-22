// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/span.h"
#include "he/core/result.h"
#include "he/core/vector.h"

namespace he::editor
{
    Result LoadFile(Vector<uint8_t>& dst, const char* path);
    inline Result LoadFile(Vector<char>& dst, const char* path) { return LoadFile(reinterpret_cast<Vector<uint8_t>&>(dst), path); }

    Result SaveFile(const char* path, const Span<uint8_t>& src, bool append = false);
    inline Result SaveFile(const char* path, const Span<char>& src, bool append = false) { return SaveFile(path, reinterpret_cast<const Span<uint8_t>&>(src), append); }
}

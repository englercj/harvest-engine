// Copyright Chad Engler

#include "he/core/string_view.h"

#include "he/core/hash.h"

namespace he
{
    uint64_t StringView::HashCode() const noexcept
    {
        return WyHash::HashData(Data(), Size());
    }
}

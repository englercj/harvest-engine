// Copyright Chad Engler

#include "he/core/string_view.h"

#include "he/core/assert.h"
#include "he/core/hash.h"

namespace he
{
    uint64_t StringView::HashCode() const noexcept
    {
        return WyHash::Mem(Data(), Size());
    }

    StringView StringView::Substring(uint32_t offset) const
    {
        HE_ASSERT(offset < m_size);
        return StringView{ m_data + offset, m_size - offset };
    }

    StringView StringView::Substring(uint32_t offset, uint32_t count) const
    {
        HE_ASSERT(offset < m_size);
        HE_ASSERT(count <= (m_size - offset));
        return StringView{ m_data + offset, count };
    }
}

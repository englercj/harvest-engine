// Copyright Chad Engler

#include "he/core/name.h"

#include "he/core/hash.h"

namespace he
{
    uint64_t Name::HashCode() const noexcept
    {
        return Mix64(m_id.val);
    }
}

// Copyright Chad Engler

#pragma once

#include "he/rhi/types.h"

namespace he::rhi
{
    /// Returns true if a format is a depth format type.
    ///
    /// \param[in] x Then format to check.
    /// \return True if the format is a depth format.
    constexpr bool IsDepthFormat(Format x) noexcept
    {
        return x == Format::Depth16Unorm
            || x == Format::Depth32Float
            || x == Format::Depth24Unorm_Stencil8
            || x == Format::Depth32Float_Stencil8;
    }
}

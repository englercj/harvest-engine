// Copyright Chad Engler

#pragma once

#include "he/rhi/types.h"

namespace he::scribe
{
    struct GlyphAtlas
    {
        rhi::Texture* curveTexture{ nullptr };
        rhi::TextureView* curveView{ nullptr };
        rhi::Texture* bandTexture{ nullptr };
        rhi::TextureView* bandView{ nullptr };
        rhi::DescriptorTable* descriptorTable{ nullptr };
        uint32_t refCount{ 0 };
    };
}

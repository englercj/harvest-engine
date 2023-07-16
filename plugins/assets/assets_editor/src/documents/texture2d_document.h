// Copyright Chad Engler

#pragma once

#include "he/editor/documents/asset_document.h"

#include "he/core/types.h"

namespace he::assets
{
    class Texture2DDocument : public editor::AssetDocument
    {
    public:
        void Show() override;
    };
}

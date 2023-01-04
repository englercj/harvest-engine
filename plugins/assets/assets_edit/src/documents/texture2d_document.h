// Copyright Chad Engler

#pragma once

#include "documents/asset_document.h"

#include "he/core/types.h"

namespace he::editor
{
    class Texture2DDocument : public AssetDocument
    {
    public:
        void Show() override;
    };
}

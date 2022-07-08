// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"

namespace he::assets
{
    class TextureImporter : public AssetImporter
    {
        HE_ASSETS_DECL_IMPORTER("he.importer.texture", "{869BBC35-8DD3-453B-B4FF-CC45F279FB5C}");

    public:
        void Import(const ImportContext& ctx, ImportResult& result) override;
    };
}

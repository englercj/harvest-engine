// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"

namespace he::assets
{
    class ImageImporter : public AssetImporter
    {
        HE_ASSETS_DECL_IMPORTER("he.assets.image.importer", "25ac0b13-d7f4-4950-a359-c35ce5904d29");

    public:
        bool CanImport(const char* file) override;
        bool Import(const ImportContext& ctx, ImportResult& result) override;
    };
}

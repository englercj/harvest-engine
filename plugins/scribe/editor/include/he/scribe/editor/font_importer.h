// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"

namespace he::scribe::editor
{
    using assets::ImporterId;
    using assets::ImporterVersion;

    class FontImporter : public assets::AssetImporter
    {
        HE_ASSETS_DECL_IMPORTER("he.scribe.font.importer", "f22d56f6-76b0-4568-914f-12cff40ae6d1");

    public:
        bool CanImport(const char* file) override;
        assets::ImportError Import(const assets::ImportContext& ctx, assets::ImportResult& result) override;
    };
}

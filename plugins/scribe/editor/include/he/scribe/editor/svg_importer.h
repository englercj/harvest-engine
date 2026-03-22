// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"

namespace he::scribe::editor
{
    using assets::ImporterId;
    using assets::ImporterVersion;

    class SvgImporter : public assets::AssetImporter
    {
        HE_ASSETS_DECL_IMPORTER("he.scribe.svg.importer", "4d3a6787-69c0-49a7-a679-3987456f7652");

    public:
        bool CanImport(const char* file) override;
        assets::ImportError Import(const assets::ImportContext& ctx, assets::ImportResult& result) override;
    };
}

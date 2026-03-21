// Copyright Chad Engler

#pragma once

#include "he/assets/asset_compiler.h"

namespace he::scribe::editor
{
    using assets::CompilerId;
    using assets::CompilerVersion;

    class FontFamilyCompiler : public assets::AssetCompiler
    {
        HE_ASSETS_DECL_COMPILER("he.scribe.font_family.compiler", "52050719-ddae-4b3d-afad-13691a3ccca4");

    public:
        bool Compile(const assets::CompileContext& ctx, assets::CompileResult& result) override;
    };
}

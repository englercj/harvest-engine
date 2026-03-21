// Copyright Chad Engler

#pragma once

#include "he/assets/asset_compiler.h"

namespace he::scribe::editor
{
    using assets::CompilerId;
    using assets::CompilerVersion;

    class ImageCompiler : public assets::AssetCompiler
    {
        HE_ASSETS_DECL_COMPILER("he.scribe.image.compiler", "8fcfcf8d-38b0-4d29-a44d-ee7b6648a6fd");

    public:
        bool Compile(const assets::CompileContext& ctx, assets::CompileResult& result) override;
    };
}

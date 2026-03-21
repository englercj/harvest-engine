// Copyright Chad Engler

#pragma once

#include "he/assets/asset_compiler.h"

namespace he::scribe::editor
{
    using assets::CompilerId;
    using assets::CompilerVersion;

    class FontFaceCompiler : public assets::AssetCompiler
    {
        HE_ASSETS_DECL_COMPILER("he.scribe.font_face.compiler", "7f5d78c0-df93-4f74-b4a6-7a51f8872ebc");

    public:
        bool Compile(const assets::CompileContext& ctx, assets::CompileResult& result) override;
    };
}

// Copyright Chad Engler

#pragma once

#include "he/assets/asset_compiler.h"

namespace he::assets
{
    class Texture2DCompiler : public AssetCompiler
    {
        HE_ASSETS_DECL_COMPILER("he.assets.texture2d.compiler", "45768cf1-efca-40d4-ad92-a1b47d8b819b");

    public:
        bool Compile(const CompileContext& ctx, CompileResult& result) override;
    };
}

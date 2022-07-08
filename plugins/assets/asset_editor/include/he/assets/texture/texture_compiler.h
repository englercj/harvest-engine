// Copyright Chad Engler

#pragma once

#include "he/assets/asset_compiler.h"

namespace he::assets
{
    class TextureCompiler : public AssetCompiler
    {
        HE_ASSETS_DECL_COMPILER("he.compiler.texture", "{869BBC35-8DD3-453B-B4FF-CC45F279FB5C}");

    public:
        void Compile(const CompileContext& ctx, CompileResult& result) override;
    };
}

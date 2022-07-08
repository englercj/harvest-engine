// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/types.h"

namespace he::assets
{
    struct CompileContext
    {
        schema::Asset::Reader asset;
    };

    struct CompileResult
    {
    };

    class AssetCompiler
    {
    public:
        virtual CompilerId Id() const = 0;
        virtual CompilerVersion Version() const = 0;

        /// Function to compile an asset, may be called from any thread.
        virtual void Compile(const CompileContext& ctx, CompileResult& result) = 0;
    };
}

#define HE_ASSETS_DECL_COMPILER(id, version) \
    public: \
        static constexpr CompilerId IdValue{ id }; \
        static constexpr CompilerVersion VersionValue{ version }; \
        CompilerId Id() const override { return IdValue; } \
        CompilerVersion Version() const override { return VersionValue; } \

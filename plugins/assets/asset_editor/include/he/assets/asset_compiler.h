// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/types.h"

namespace he::assets
{
    struct CompileContext
    {
        schema::AssetFile::Reader assetFile;
        schema::Asset::Reader asset;

        template <typename T>
        bool GetResource(const AssetUuid& assetId, ResourceId resourceId, Vector<T>& data) const
        {
            // TODO
            HE_UNUSED(assetId, resourceId, data);
            return true;
        }
    };

    struct CompileResult
    {
        bool AddResource(const AssetUuid& assetId, ResourceId resourceId, Span<const uint8_t> data)
        {
            // TODO
            HE_UNUSED(assetId, resourceId, data);
            return true;
        }
    };

    class AssetCompiler
    {
    public:
        virtual CompilerId Id() const = 0;
        virtual CompilerVersion Version() const = 0;

        /// Function to compile an asset, may be called from any thread.
        virtual bool Compile(const CompileContext& ctx, CompileResult& result) = 0;
    };
}

#define HE_ASSETS_DECL_COMPILER(id, version) \
    public: \
        static constexpr CompilerId IdValue{ id }; \
        static constexpr CompilerVersion VersionValue{ version }; \
        CompilerId Id() const override { return IdValue; } \
        CompilerVersion Version() const override { return VersionValue; } \

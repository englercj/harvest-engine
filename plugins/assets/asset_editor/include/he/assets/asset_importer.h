// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/types.h"

namespace he::assets
{
    struct ImportContext
    {
        /// Absolute path to the file that should be imported.
        const char* file{ nullptr };

        /// Asset file to be created from this import. If a file already exists then this will
        /// contain the existing asset file.
        schema::AssetFile::Builder assetFile{};

        /// The builder to hold the asset file allocations.
        he::schema::Builder assetFileBuilder{};

        /// A builder for the additional information that was requested by the importer on a
        /// previous run.
        he::schema::Builder moreInfoBuilder{};
    };

    struct ImportResult
    {
        /// Declaration reflection info for the additional information structure the importer is
        /// requesting. When null it is assumed no additional information is required.
        he::schema::DeclInfo* moreInfoDecl{ nullptr };

        /// Builder for a structure of additional data the importer requires to perform the import
        /// operation.
        he::schema::Builder moreInfoBuilder{};
    };

    class AssetImporter
    {
    public:
        virtual ImporterId Id() const = 0;
        virtual ImporterVersion Version() const = 0;

        /// Function to import a source, may be called from any thread.
        virtual void Import(const ImportContext& ctx, ImportResult& result) = 0;
    };
}

#define HE_ASSETS_DECL_IMPORTER(id, version) \
    public: \
        static constexpr ImporterId IdValue{ id }; \
        static constexpr ImporterVersion VersionValue{ version }; \
        ImporterId Id() const override { return IdValue; } \
        ImporterVersion Version() const override { return VersionValue; } \

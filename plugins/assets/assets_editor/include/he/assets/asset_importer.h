// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"
#include "he/assets/types.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/dynamic.h"

namespace he::assets
{
    // --------------------------------------------------------------------------------------------
    /// Input context for an import process.
    struct ImportContext
    {
        explicit ImportContext(AssetDatabase& db) : db(db) {}

        /// The asset database that can be used to find additional information for this import.
        AssetDatabase& db;

        /// Absolute path to the file that should be imported.
        const char* file{ nullptr };

        /// The existing asset file that was created when this file was previously imported, or
        /// a newly created asset file if one did not previously exist.
        AssetFile::Reader assetFile{};

        /// Import settings requested by the importer on a previous run.
        /// If no such settings have been provided then this reader will be invalid.
        schema::StructReader settings{};
    };

    // --------------------------------------------------------------------------------------------
    /// Output result of an import process.
    class ImportResult
    {
    public:
        explicit ImportResult(ImportContext& ctx, schema::Builder& builder)
            : m_ctx(ctx)
            , m_builder(builder)
        {}

        /// Add an asset resulting from the import process.
        ///
        /// \param[in] assetTypeName The type name of the asset.
        /// \param[in] name The user friendly name of the asset.
        /// \return A builder for the created asset object. If an error occurs an invalid builder
        /// is returned, so be sure to check `.IsValid()` before using it.
        Asset::Builder CreateAsset(StringView assetTypeName, StringView name);

        /// Update an existing asset in the asset file. If the asset uuid doesn't exist in the file
        /// an invalid builder is returned.
        ///
        /// \param[in] assetUuid The UUID of the asset to update.
        /// \return A builder for the asset that can be modified.
        Asset::Builder UpdateAsset(const AssetUuid& assetUuid);

        /// Update an existing asset in the asset file. If the asset uuid doesn't exist in the file
        /// an invalid builder is returned.
        ///
        /// \param[in] asset The asset to update.
        /// \return A builder for the asset that can be modified.
        Asset::Builder UpdateAsset(Asset::Reader asset);

        /// Add a resource resulting from the import process. These resources are generally data
        /// that is extracted from the source so that parsing the source file isn't necessary for
        /// the compiler. For example, the raw pixels of an image imported from a PSD will allow
        /// the compiler to use the pixels without touching the PSD file.
        ///
        /// \param[in] assetUuid The UUID of the asset that is generating this resource.
        /// \param[in] resourceId A user-defined ID for this resource. This is used to access the
        ///     resource later.
        /// \param[in] data The bytes of the resource data to be stored.
        /// \return True if the data was stored successfully, or false otherwise.
        //bool AddResource(const AssetUuid& assetUuid, ResourceId resourceId, Span<const uint8_t> data);

        /// Requests an import settings object. This is useful when the import cannot be completed
        /// with the information given, and additional information is needed. For example, an
        /// import of a PSD file may require the user to select which layers to import, or how to
        /// treat various layers upon import.
        ///
        /// The import process will happen again once this object has been filled out and is
        /// passed in the \ref ImportContext::challenge builder.
        ///
        /// \tparam T The type of schema object that is required for the import to proceed.
        /// \return Returns the newly created challenge object so it can be filled with information.
        template <typename T>
        typename T::Builder RequestImportSettings()
        {
            typename T::Builder data = m_builder.AddStruct<T>();
            m_reqSettings = data;
            return data;
        }

    private:
        friend class AssetServer;

        ImportContext& m_ctx;
        schema::Builder& m_builder;

        schema::DynamicStruct::Builder m_reqSettings{};
        Vector<Asset::Builder> m_new{};
        Vector<Asset::Builder> m_updated{};
    };

    // --------------------------------------------------------------------------------------------
    /// Base class for a processor that imports source files into Harvest assets.
    class AssetImporter
    {
    public:
        /// Provides a stable globally unique identifier for this importer. Usually this is the
        /// hash of a unique qualified name string.
        ///
        /// \note Normally you don't want to implement this function directly. Instead use the
        /// \ref HE_ASSETS_DECL_IMPORTER macro to generate this function for you based on a string.
        virtual ImporterId Id() const = 0;

        /// Provides a version for this instance of the importer. Anytime the importer changes
        /// in a *backwards incompatible* way this value should also change. When this value does
        /// change it will cause all assets that were imported by a previous version to
        /// automatically be reimported.
        ///
        /// \note It is not recommended to use monotonically increasing values here because of
        /// merge issues. For example, two branches increasing the version when merged should
        /// actually create a third new version number. However, there may be no conflict since
        /// the values are the same. Instead, the recommendation is to use a unique value to ensure
        /// there is a conflict on merge and a third value can be generated. Using a UUID is the
        /// recommended strategy.
        ///
        /// \note Normally you don't want to implement this function directly. Instead use the
        /// \ref HE_ASSETS_DECL_IMPORTER macro to generate this function for you based on a string.
        virtual ImporterVersion Version() const = 0;

        /// Checks if the proposed file can be handled by this importer.
        ///
        /// \param[in] file The path to the file to be imported.
        /// \return True if this importer can handle the file, false otherwise.
        virtual bool CanImport(const char* file) = 0;

        /// Function to import a source, may be called from any thread.
        virtual bool Import(const ImportContext& ctx, ImportResult& result) = 0;
    };
}

// ------------------------------------------------------------------------------------------------
/// Declares the necessary data for a class that inherits from AssetImporter.
///
/// For additional details about the `id` and `version` parameters see \ref AssetImporter::Id()
/// and \ref AssetImporter::Version()
///
/// Example:
/// ```cpp
/// class MyImporter : public AssetImporter
/// {
///     HE_ASSETS_DECL_IMPORTER("my.plugin.type.importer", "b3742456-c417-41ac-9785-6da013426bf9");
///
/// public:
///     bool CanImport(const char* file) override;
///     bool Import(const ImportContext& ctx, ImportResult& result) override;
/// };
/// ```
///
/// \param[in] id The stable unique identifier for this importer. Usually a qualified name string.
/// \param[in] version The version for this importer. Usually a string UUID.
#define HE_ASSETS_DECL_IMPORTER(id, version) \
    public: \
        static constexpr ImporterId IdValue{ id }; \
        static constexpr ImporterVersion VersionValue{ version }; \
        ImporterId Id() const override { return IdValue; } \
        ImporterVersion Version() const override { return VersionValue; }

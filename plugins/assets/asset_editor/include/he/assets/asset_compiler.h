// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"
#include "he/assets/types.h"
#include "he/core/types.h"

namespace he::assets
{
    // --------------------------------------------------------------------------------------------
    /// Input context for a compile process.
    struct CompileContext
    {
        explicit CompileContext(AssetDatabase& db) : db(db) {}

        /// The asset database that can be used to find additional information for this compile.
        AssetDatabase& db;

        /// The asset file which contains the asset being compiled.
        schema::AssetFile::Reader assetFile;

        /// The asset which is being compiled.
        schema::Asset::Reader asset;
    };

    // --------------------------------------------------------------------------------------------
    /// Output result of a compile process.
    class CompileResult
    {
    public:
        explicit CompileResult(CompileContext& ctx) : m_ctx(ctx) {}

        /// Add a resource resulting from the compile process. These resources are generally data
        /// that is loaded at runtime by the game.
        ///
        /// \param[in] assetUuid The UUID of the asset that is generating this resource.
        /// \param[in] resourceId A user-defined ID for this resource. This is used to access the
        ///     resource later.
        /// \param[in] data The bytes of the resource data to be stored.
        /// \return True if the data was stored successfully, or false otherwise.
        //bool AddResource(const AssetUuid& assetUuid, ResourceId resourceId, Span<const uint8_t> data);

    private:
        CompileContext& m_ctx;
    };

    // --------------------------------------------------------------------------------------------
    /// Base class for a processor that compiles Harvest assets into game-ready resources.
    class AssetCompiler
    {
    public:
        /// Provides a stable globally unique identifier for this compiler. Usually this is the
        /// hash of a unique qualified name string.
        ///
        /// \note Normally you don't want to implement this function directly. Instead use the
        /// \ref HE_ASSETS_DECL_COMPILER macro to generate this function for you based on a string.
        virtual CompilerId Id() const = 0;

        /// Provides a version for this instance of the compiler. Anytime the compiler changes
        /// in a *backwards incompatible* way this value should also change. When this value does
        /// change it will cause all assets that were compiled by a previous version to
        /// automatically be recompiled.
        ///
        /// \note It is not recommended to use monotonically increasing values here because of
        /// merge issues. For example, two branches increasing the version when merged should
        /// actually create a third new version number. However, there may be no conflict since
        /// the values are the same. Instead, the recommendation is to use a unique value to ensure
        /// there is a conflict on merge and a third value can be generated. Using a UUID is the
        /// recommended strategy.
        ///
        /// \note Normally you don't want to implement this function directly. Instead use the
        /// \ref HE_ASSETS_DECL_COMPILER macro to generate this function for you based on a string.
        virtual CompilerVersion Version() const = 0;

        /// Function to compile an asset, may be called from any thread.
        virtual bool Compile(const CompileContext& ctx, CompileResult& result) = 0;
    };
}

// ------------------------------------------------------------------------------------------------
/// Declares the necessary data for a class that inherits from AssetCompiler.
///
/// For additional details about the `id` and `version` parameters see \ref AssetCompiler::Id()
/// and \ref AssetCompiler::Version()
///
/// Example:
/// ```cpp
/// class MyCompiler : public AssetCompiler
/// {
///     HE_ASSETS_DECL_COMPILER("my.plugin.type.compiler", "3e4b648f-938e-4e35-8ec4-cc1da2d55bb9");
///
/// public:
///     bool Compile(const CompileContext& ctx, CompileResult& result) override;
/// };
/// ```
///
/// \param[in] id The stable unique identifier for this compiler. Usually a qualified name string.
/// \param[in] version The version for this compiler. Usually a string UUID.
#define HE_ASSETS_DECL_COMPILER(id, version) \
    public: \
        static constexpr CompilerId IdValue{ id }; \
        static constexpr CompilerVersion VersionValue{ version }; \
        CompilerId Id() const override { return IdValue; } \
        CompilerVersion Version() const override { return VersionValue; }

// Copyright Chad Engler

#pragma once

#include "he/assets/asset_type_registry.h"
#include "he/assets/types.h"
#include "he/assets/texture/texture_compiler.h"
#include "he/assets/texture/texture_importer.h"
#include "he/core/module_registry.h"
#include "he/core/types.h"

namespace he::assets
{
    class AssetEditorModule : public Module
    {
        bool Startup() override
        {
            // ----------
            // Register Asset Types
            AssetTypeRegistry& types = AssetTypeRegistry::Get();

            // Texture
            types.RegisterAssetType<schema::Texture, TextureCompiler>();
            types.RegisterImporter<TextureImporter>();

            // ----------
            // Register Editor Types
            //EditorRegistry& editor = EditorDocumentRegistry::Get();

            //// Texture
            //editor.RegisterAssetDocument<schema::Texture, TextureDocument>();

            return true;
        }
    };
}

HE_EXPORT_MODULE(he::assets::AssetEditorModule, AssetEditor);

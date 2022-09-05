// Copyright Chad Engler

#include "he/assets/asset_type_registry.h"
#include "he/assets/types.h"
#include "he/assets/compilers/texture2d_compiler.h"
#include "he/assets/importers/image_importer.h"
#include "he/core/module_registry.h"
#include "he/core/types.h"

#include "encoder/basisu_enc.h"

namespace he::assets
{
    class AssetEditorModule : public Module
    {
        void Register() override
        {
            ModuleRegistry& registry = Registry();

            registry.RegisterApi<AssetTypeRegistry>();
        }

        void Unregister() override
        {
            ModuleRegistry& registry = Registry();

            registry.UnregisterApi<AssetTypeRegistry>();
        }

        bool Startup() override
        {
            ModuleRegistry& registry = Registry();

            AssetTypeRegistry& types = registry.GetApi<AssetTypeRegistry>();
            types.RegisterAssetType<schema::Texture2D, Texture2DCompiler>();
            types.RegisterImporter<ImageImporter>();

            //EditorDocumentRegistry* editor = registry.FindApi<EditorDocumentRegistry>();
            //if (editor)
            //{
            //    editor->RegisterAssetDocument<schema::Texture, TextureDocument>();
            //}

            basisu::basisu_encoder_init();

            return true;
        }

        void Shutdown() override
        {
            basisu::basisu_encoder_deinit();

            ModuleRegistry& registry = Registry();

            //EditorDocumentRegistry* editor = registry.FindApi<EditorDocumentRegistry>();
            //if (editor)
            //{
            //    editor->UnregisterAssetDocument<schema::Texture, TextureDocument>();
            //}

            AssetTypeRegistry& types = registry.GetApi<AssetTypeRegistry>();
            types.UnregisterAssetType<schema::Texture2D>();
            types.UnregisterImporter<ImageImporter>();
        }
    };
}

HE_EXPORT_MODULE(he::assets::AssetEditorModule);

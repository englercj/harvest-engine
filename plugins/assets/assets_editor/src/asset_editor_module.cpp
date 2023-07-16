// Copyright Chad Engler

#include "documents/texture2d_document.h"

#include "he/assets/asset_editors.h"
#include "he/assets/asset_type_registry.h"
#include "he/assets/types.h"
#include "he/assets/compilers/texture2d_compiler.h"
#include "he/assets/importers/image_importer.h"
#include "he/core/module_registry.h"
#include "he/core/types.h"
#include "he/editor/services/asset_document_service.h"
#include "he/editor/services/type_edit_ui_service.h"

#include "encoder/basisu_enc.h"

namespace he::assets
{
    Module* g_assetEditorModule = nullptr;

    class AssetEditorModule : public Module
    {
        HE_DECL_MODULE(AssetEditorModule);

    private:
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
            HE_ASSERT(!g_assetEditorModule);
            g_assetEditorModule = this;

            ModuleRegistry& registry = Registry();

            AssetTypeRegistry& types = registry.GetApi<AssetTypeRegistry>();
            types.RegisterAssetType<Texture2D, Texture2DCompiler>();
            types.RegisterImporter<ImageImporter>();

            editor::AssetDocumentService& docs = registry.GetApi<editor::AssetDocumentService>();
            docs.RegisterAssetDocument<Texture2D, Texture2DDocument>();

            editor::TypeEditUIService& editors = registry.GetApi<editor::TypeEditUIService>();
            editors.RegisterFieldEditor<Asset>("uuid", { &AssetUuidFieldEditor, editor::TypeEditUIService::EditorFlag::Inline });
            editors.RegisterFieldEditor<Asset>("data", { &AssetDataFieldEditor });

            basisu::basisu_encoder_init();

            return true;
        }

        void Shutdown() override
        {
            HE_ASSERT(g_assetEditorModule);
            g_assetEditorModule = nullptr;

            basisu::basisu_encoder_deinit();

            ModuleRegistry& registry = Registry();

            editor::TypeEditUIService& editors = registry.GetApi<editor::TypeEditUIService>();
            editors.UnregisterFieldEditor<Asset>("uuid");
            editors.UnregisterFieldEditor<Asset>("data");

            editor::AssetDocumentService& docs = registry.GetApi<editor::AssetDocumentService>();
            docs.UnregisterAssetDocument<Texture2D>();

            AssetTypeRegistry& types = registry.GetApi<AssetTypeRegistry>();
            types.UnregisterAssetType<Texture2D>();
            types.UnregisterImporter<ImageImporter>();
        }
    };
}

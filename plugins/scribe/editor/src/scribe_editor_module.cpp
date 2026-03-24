// Copyright Chad Engler

#include "he/scribe/editor/compiler_frontend.h"
#include "he/scribe/editor/font_face_compiler.h"
#include "he/scribe/editor/font_family_compiler.h"
#include "he/scribe/editor/font_importer.h"
#include "he/scribe/editor/image_compiler.h"
#include "he/scribe/editor/svg_importer.h"
#include "he/scribe/schema_types.h"

#include "he/assets/asset_type_registry.h"
#include "he/core/module_registry.h"

namespace he::scribe
{
    class ScribeEditorModule : public Module
    {
        HE_DECL_MODULE(ScribeEditorModule);

    private:
        bool Startup() override
        {
            assets::AssetTypeRegistry& types = Registry().GetApi<assets::AssetTypeRegistry>();
            types.RegisterAssetType<ScribeFontFace, editor::FontFaceCompiler>();
            types.RegisterAssetType<ScribeFontFamily, editor::FontFamilyCompiler>();
            types.RegisterAssetType<ScribeImage, editor::ImageCompiler>();
            types.RegisterImporter<editor::FontImporter>();
            types.RegisterImporter<editor::SvgImporter>();
            return true;
        }

        void Shutdown() override
        {
            assets::AssetTypeRegistry& types = Registry().GetApi<assets::AssetTypeRegistry>();
            types.UnregisterImporter<editor::SvgImporter>();
            types.UnregisterImporter<editor::FontImporter>();
            types.UnregisterAssetType<ScribeImage>();
            types.UnregisterAssetType<ScribeFontFamily>();
            types.UnregisterAssetType<ScribeFontFace>();
        }
    };
}

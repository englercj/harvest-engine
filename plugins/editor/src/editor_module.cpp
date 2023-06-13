// Copyright Chad Engler

#include "he/editor/di.h"

#include "he/core/module_registry.h"
#include "he/core/types.h"
#include "he/editor/services/type_edit_ui_service.h"
#include "he/editor/widgets/schema_type_editors.h"
#include "he/schema/types.h"

namespace he::editor
{
    class EditorModule : public Module
    {
        HE_DECL_MODULE(EditorModule);

    private:
        void Register() override
        {
            ModuleRegistry& registry = Registry();

            registry.RegisterApi(DICreate<TypeEditUIService&>());
        }

        void Unregister() override
        {
            ModuleRegistry& registry = Registry();

            registry.UnregisterApi<TypeEditUIService>();
        }

        bool Startup() override
        {
            ModuleRegistry& registry = Registry();

            editor::TypeEditUIService& editors = registry.GetApi<editor::TypeEditUIService>();
            editors.RegisterTypeEditor<schema::Vec2f>({ &Vec2fEditor, editor::TypeEditUIService::EditorFlag::Inline });
            editors.RegisterTypeEditor<schema::Vec3f>({ &Vec3fEditor, editor::TypeEditUIService::EditorFlag::Inline });
            editors.RegisterTypeEditor<schema::Vec4f>({ &Vec4fEditor, editor::TypeEditUIService::EditorFlag::Inline });

            return true;
        }
    };
}

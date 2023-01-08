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
            editors.RegisterTypeEditor<schema::Vec2f>({ true, &Vec2fEditor });
            editors.RegisterTypeEditor<schema::Vec3f>({ true, &Vec3fEditor });
            editors.RegisterTypeEditor<schema::Vec4f>({ true, &Vec4fEditor });

            return true;
        }
    };
}

HE_EXPORT_MODULE(he::editor::EditorModule);

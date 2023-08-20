// Copyright Chad Engler

#include "he/editor/di.h"

#include "he/core/module_registry.h"
#include "he/core/types.h"
#include "he/editor/services/app_args_service.h"
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

            registry.RegisterApi(DICreate<AppArgsService&>());
            registry.RegisterApi(DICreate<TypeEditUIService&>());
        }

        void Unregister() override
        {
            ModuleRegistry& registry = Registry();

            registry.UnregisterApi<TypeEditUIService>();
            registry.UnregisterApi<AppArgsService>();
        }

        bool Startup() override
        {
            ModuleRegistry& registry = Registry();

            TypeEditUIService& editors = registry.GetApi<TypeEditUIService>();
            editors.RegisterTypeEditor<schema::Vec2f>({ &Vec2fEditor, TypeEditUIService::EditorFlag::Inline });
            editors.RegisterTypeEditor<schema::Vec3f>({ &Vec3fEditor, TypeEditUIService::EditorFlag::Inline });
            editors.RegisterTypeEditor<schema::Vec4f>({ &Vec4fEditor, TypeEditUIService::EditorFlag::Inline });
            editors.RegisterTypeEditor<schema::Uuid>({ &UuidEditor, TypeEditUIService::EditorFlag::Inline });

            AppArgsService& appArgs = registry.GetApi<AppArgsService>();
            appArgs.RegisterArg(ArgDesc{ "help", "Prints this help message", ArgType::Flag, 'h' });

            return true;
        }
    };
}

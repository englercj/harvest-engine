// Copyright Chad Engler

#include "di.h"
#include "services/type_edit_ui_service.h"
#include "widgets/property_grid.h"

#include "he/core/module_registry.h"
#include "he/core/types.h"

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

            TypeEditUIService& editors = registry.GetApi<TypeEditUIService>();
            editors.RegisterTypeEditor<bool>(&PropertyGridEditor_Bool);

            return true;
        }
    };
}

HE_EXPORT_MODULE(he::editor::EditorModule);

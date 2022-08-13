// Copyright Chad Engler

#pragma once

#include "di.h"
#include "services/property_grid_service.h"

#include "he/core/module_registry.h"
#include "he/core/types.h"

namespace he::editor
{
    class EditorModule : public Module
    {
        void Register() override
        {
            ModuleRegistry& registry = Registry();

            registry.RegisterApi(DICreate<PropertyGridService&>());
        }

        void Unregister() override
        {
            ModuleRegistry& registry = Registry();

            registry.UnregisterApi<PropertyGridService>();
        }
    };
}

HE_EXPORT_MODULE(he::editor::EditorModule);

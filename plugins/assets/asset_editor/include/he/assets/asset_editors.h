// Copyright Chad Engler

#pragma once

#include "he/schema/dynamic.h"

#include "services/type_edit_ui_service.h"

namespace he::assets
{
    void Vec2fEditor(const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
    void Vec3fEditor(const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);

    void AssetUuidFieldEditor(const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
    void AssetDataFieldEditor(const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
}

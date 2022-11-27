// Copyright Chad Engler

#pragma once

#include "he/schema/dynamic.h"

#include "services/type_edit_ui_service.h"

namespace he::assets
{
    void Vec2fEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
    void Vec3fEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);

    void AssetUuidFieldEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
    void AssetDataFieldEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
}

// Copyright Chad Engler

#pragma once

#include "he/schema/dynamic.h"

#include "he/editor/services/type_edit_ui_service.h"

namespace he::editor
{
    void Vec2fEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx);
    void Vec3fEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx);
    void Vec4fEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx);
}

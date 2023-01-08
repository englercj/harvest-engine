// Copyright Chad Engler

#pragma once

#include "he/schema/dynamic.h"

#include "he/editor/services/type_edit_ui_service.h"

namespace he::assets
{
    void AssetUuidFieldEditor(const void*, const schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
    void AssetDataFieldEditor(const void*, const schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx);
}

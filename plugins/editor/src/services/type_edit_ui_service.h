// Copyright Chad Engler

#pragma once

#include "asset_edit_service.h"

#include "he/core/delegate.h"
#include "he/core/span.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/schema/dynamic.h"
#include "he/schema/layout.h"
#include "he/schema/schema.h"

#include <unordered_map>

namespace he::editor
{
    class TypeEditUIService
    {
    public:
        using EditorDelegate = Delegate<bool(schema::DynamicValue::Builder value, AssetEdit& edit)>;

    public:
        void ShowTypeEditor(const TypeInfo& info, const void* value, schema::Field::Reader path);

        template <typename T>
        void ShowTypeEditor(const void* value, schema::Field::Reader path)
        {
            ShowTypeEditor(TypeInfo::Get<T>(), value, path);
        }

    public:
        template <schema::DataType T>
        void RegisterTypeEditor(EditorDelegate func) { RegisterTypeEditor(TypeInfo::Get<T>().Hash(), func); }

        template <typename T>
        void RegisterTypeEditor()

    private:
        void RegisterTypeEditor(uint64_t key, EditorDelegate func);

    private:
        std::unordered_map<uint64_t, EditorDelegate> m_editors{};
    };
}

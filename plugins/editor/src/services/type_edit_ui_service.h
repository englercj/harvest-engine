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
        struct Context
        {
            const he::schema::DynamicValue::Reader data;
            he::schema::Field::Reader field;
            uint32_t listIndex;

            AssetEdit& edit;
            he::schema::DynamicStructVisitor::Reader& visitor;
        };

        using EditorDelegate = Delegate<void(const he::schema::DynamicValue::Reader& value, Context& ctx)>;

        struct Editor
        {
            bool isInline{ false };
            EditorDelegate func{};
        };

    public:
        void RegisterTypeEditor(he::schema::TypeId typeId, Editor&& editor);

        template <typename T>
        void RegisterTypeEditor(Editor&& editor) { RegisterTypeEditor(T::Id, Move(editor)); }

        void RegisterFieldEditor(he::schema::Field::Reader field, Editor&& editor);

        template <typename T>
        void RegisterFieldEditor(StringView name, Editor&& editor) { RegisterFieldEditor(he::schema::FindFieldByName<T>(name), Move(editor)); }

        const Editor* FindEditor(he::schema::TypeId typeId) const;
        const Editor* FindEditor(he::schema::Field::Reader field) const;

    private:
        std::unordered_map<he::schema::TypeId, Editor> m_typeEditors{};
        std::unordered_map<const he::schema::Word*, Editor> m_fieldEditors{};
    };
}

// Copyright Chad Engler

#pragma once

#include "he/core/delegate.h"
#include "he/core/hash_table.h"
#include "he/core/span.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/editor/services/asset_edit_service.h"
#include "he/schema/dynamic.h"
#include "he/schema/layout.h"
#include "he/schema/schema.h"

namespace he::editor
{
    class TypeEditUIService
    {
    public:
        struct Context
        {
            const schema::DynamicValue::Reader data;
            schema::Field::Reader field;
            uint32_t listIndex;

            SchemaEdit& edit;
            schema::DynamicStructVisitor::Reader& visitor;
        };

        using EditorDelegate = Delegate<void(const schema::DynamicValue::Reader& value, Context& ctx)>;

        enum class EditorFlag
        {
            None        = 0,
            Inline      = 1 << 0,
        };

        struct Editor
        {
            EditorDelegate func{};
            EditorFlag flags{ EditorFlag::None };
        };

    public:
        const Editor* FindEditor(schema::TypeId typeId) const;
        const Editor* FindEditor(schema::Field::Reader field) const;

    public:
        void RegisterTypeEditor(schema::TypeId typeId, Editor&& editor);
        void UnregisterTypeEditor(schema::TypeId typeId);

        template <typename T>
        void RegisterTypeEditor(Editor&& editor) { RegisterTypeEditor(T::Id, Move(editor)); }

        template <typename T>
        void UnregisterTypeEditor() { UnregisterTypeEditor(T::Id); }

    public:
        void RegisterFieldEditor(schema::Field::Reader field, Editor&& editor);
        void UnregisterFieldEditor(schema::Field::Reader field);

        template <typename T>
        void RegisterFieldEditor(StringView name, Editor&& editor) { RegisterFieldEditor(schema::FindFieldByName<T>(name), Move(editor)); }

        template <typename T>
        void UnregisterFieldEditor(StringView name) { UnregisterFieldEditor(schema::FindFieldByName<T>(name)); }

    private:
        HashMap<schema::TypeId, Editor> m_typeEditors{};
        HashMap<const schema::Word*, Editor> m_fieldEditors{};
    };

    HE_ENUM_FLAGS(TypeEditUIService::EditorFlag);
}

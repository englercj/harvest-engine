// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/delegate.h"
#include "he/core/sync.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/schema/dynamic.h"
#include "he/schema/layout.h"
#include "he/schema/schema.h"

namespace he::editor
{
    // --------------------------------------------------------------------------------------------
    struct SchemaEditPathEntry
    {
        schema::Field::Reader field{};              ///< The field being edited.
        uint32_t index{ 0 };                        ///< Index for an element of the field when it is a List or Array.
        const schema::DeclInfo* info{ nullptr };    ///< Type of the field when it is AnyPointer or AnyStruct.
    };

    // --------------------------------------------------------------------------------------------
    struct SchemaEditAction
    {
        enum class Kind
        {
            AddListItem,
            EraseListItem,
            SetValue,
            InitValue,
            ClearValue,
        };

        Kind kind{ Kind::SetValue };
        Vector<SchemaEditPathEntry> path{};
        schema::DynamicValue::Builder value{};

    private:
        friend class SchemaEditContext;
        Vector<SchemaEditAction> undoActions{};
    };

    // --------------------------------------------------------------------------------------------
    struct SchemaEdit
    {
        explicit SchemaEdit(SchemaEditContext& ctx) : ctx(&ctx) {}

        SchemaEditAction& EmplaceAction(SchemaEditAction::Kind kind)
        {
            SchemaEditAction& action = actions.EmplaceBack();
            action.kind = kind;
            action.path = path;
            return action;
        }

        SchemaEditContext* ctx;
        String name{};
        Vector<SchemaEditPathEntry> path{};
        Vector<SchemaEditAction> actions{};
    };

    // --------------------------------------------------------------------------------------------
    class SchemaEditContext
    {
    public:
        SchemaEditContext() = default;
        SchemaEditContext(const schema::DynamicStruct::Reader& data) noexcept;

        void SetData(const schema::DynamicStruct::Reader& data);

        schema::DynamicStruct::Builder& Data() { return m_data; }
        const schema::DynamicStruct::Builder& Data() const { return m_data; }

        schema::Builder& Builder() { return m_builder; }
        const schema::Builder& Builder() const { return m_builder; }

        Span<const SchemaEdit> Edits() const { return m_edits; }

        void PushEdit(SchemaEdit&& edit);

        void Redo();
        void Undo();
        void Clear();

    private:
        void RedoEdit(SchemaEdit& edit);
        void RedoAction(SchemaEditAction& action);

        void UndoEdit(const SchemaEdit& edit);
        void UndoAction(const SchemaEditAction& action);

        void MakeUndoActions(schema::DynamicValue::Builder data, SchemaEditAction& action, uint32_t index);
        void ApplyAction(schema::DynamicValue::Builder& data, const SchemaEditAction& action);

    private:
        schema::Builder m_builder{};
        schema::DynamicStruct::Builder m_data{};

        Vector<SchemaEdit> m_edits{};
        uint32_t m_activeEditCount{ 0 };
    };
}

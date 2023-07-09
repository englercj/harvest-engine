// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/delegate.h"
#include "he/core/sync.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/variant.h"
#include "he/core/vector.h"
#include "he/schema/buffer_dynamic.h"
#include "he/schema/buffer_layout.h"
#include "he/schema/schema.h"

namespace he::editor
{
    // TODO: Rewrite this as a ledger atop a default (or loaded) object.
    // 1. Reads are `GetBool(path)` and walk the ledger to get the 'latest' value.
    // 2. Writes are `SetBool(path, value)` and add a new entry to the ledger.
    // 3. Every X entries in the ledger we make a 'snapshot' of the current state to accelerate reads.
    // 4. Saving an asset creates a snapshot and writes that to disk.
    // 5. Undo/Redo just moves the ledger index, snapshots change the 'end' pointer for searching.
    //
    // This method is fast for writes, but slower for reads than just having the buffer. Which sucks
    // because reading happens every frame for ImGui, but writes only happen at user interaction speed.
    //
    // Maybe instead this should just bite the bullet and realloc when adding list entries? That makes
    // AddListItem/RemoveListItem/Save slower, but reads remain fast because the resolved buffer is always
    // available.
    //
    // If it matters, every write (SchemaEdit equivalent) in The Machinery made a copy of the object
    // and kept it around in the undo buffer. So this would be much better than that?


    // --------------------------------------------------------------------------------------------
    struct SchemaEditPathEntry
    {
        const schema::FieldInfo* field{ nullptr };  ///< The field being edited.
        const schema::DeclInfo* info{ nullptr };    ///< Type of the field when it is AnyPointer/AnyStruct
        uint32_t index{ 0 };                        ///< Index for an array or list field.
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
        Variant<bool, int64_t, uint64_t, double, he::String> value{};

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

        String name{};
        Vector<SchemaEditPathEntry> path{};
        Vector<SchemaEditAction> actions{};
    };

    // --------------------------------------------------------------------------------------------
    class SchemaEditContext
    {
    public:
        SchemaEditContext() = default;
        SchemaEditContext(const schema::DeclInfo& info, void* data) noexcept;

        void SetData(const schema::DeclInfo& info, void* data);

        Span<const SchemaEdit> Edits() const { return m_edits; }
        uint32_t ActiveEditCount() const { return m_activeEditCount; }

        void PushEdit(SchemaEdit&& edit);

        void Redo();
        void Undo();
        void Clear();
        void ClearData();
        void ClearEdits();

    private:
        void RedoEdit(SchemaEdit& edit);
        void RedoAction(SchemaEditAction& action);

        void UndoEdit(const SchemaEdit& edit);
        void UndoAction(const SchemaEditAction& action);

        void ApplyAction(void* data, const SchemaEditAction& action);

    private:
        schema::DeclInfo* m_info{ nullptr };
        void* m_data{ nullptr };

        Vector<SchemaEdit> m_edits{};
        uint32_t m_activeEditCount{ 0 };
    };
}

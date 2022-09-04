// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/delegate.h"
#include "he/core/sync.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"
#include "he/schema/layout.h"

namespace he::editor
{
    // --------------------------------------------------------------------------------------------
    struct AssetEditPathEntry
    {
        schema::Field::Reader field{};  ///< The field being edited.
        uint32_t listIndex{ 0 };        ///< Index for an array or list field.
    };

    // --------------------------------------------------------------------------------------------
    struct AssetEditValue
    {
        AssetEditValue() : builder(), value(builder.AddStruct<schema::Value>()) {}

        schema::Builder builder;
        schema::Value::Builder value;
    };

    // --------------------------------------------------------------------------------------------
    struct AssetEditAction
    {
        enum class Kind
        {
            AddListItem,
            RemoveListItem,
            SetUnionTag,
            SetValue,
            ResetToDefault,
        };

        Kind kind{ Kind::SetValue };
        Vector<AssetEditPathEntry> path{};
        AssetEditValue value{};
        AssetEditValue previousValue{};
    };

    // --------------------------------------------------------------------------------------------
    struct AssetEdit
    {
        AssetEditAction& EmplaceAction(AssetEditAction::Kind kind)
        {
            AssetEditAction& action = actions.EmplaceBack();
            action.kind = kind;
            action.path = path;
            return action;
        }

        String name{};
        Vector<AssetEditPathEntry> path{};
        Vector<AssetEditAction> actions{};
    };

    // --------------------------------------------------------------------------------------------
    class AssetEditContext
    {
    public:
        schema::StructBuilder& Data() { return m_data; }
        const schema::DeclInfo& DeclInfo() { return *m_declInfo; }
        Span<const AssetEdit> Edits() const { return m_edits; }

        void PushEdit(AssetEdit&& edit);

        void Redo();
        void Undo();

    private:
        void RedoEdit(AssetEdit& edit);
        void RedoAction(AssetEditAction& action);

        void UndoEdit(AssetEdit& edit);
        void UndoAction(AssetEditAction& action);

    private:
        schema::Builder m_builder{};
        schema::StructBuilder m_data{};
        const schema::DeclInfo* m_declInfo{ nullptr };

        Vector<AssetEdit> m_edits{};
        uint32_t m_activeEditCount{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    class AssetEditService
    {
    public:
        AssetEditContext& OpenAsset(const assets::AssetUuid& assetUuid);

        void Close(AssetEditContext& ctx);

    private:
        struct ContextEntry
        {
            UniquePtr<AssetEditContext> ctx{};
            uint32_t refCount{ 0 };
        };

        Vector<ContextEntry> m_contexts{};
    };
}

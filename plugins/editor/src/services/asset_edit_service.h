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
    struct AssetEditPathEntry
    {
        he::schema::Field::Reader field{};              ///< The field being edited.
        uint32_t listIndex{ 0 };                        ///< Index for an array or list field.
    };

    // --------------------------------------------------------------------------------------------
    struct AssetEditAction
    {
        enum class Kind
        {
            AddListItem,
            RemoveListItem,
            SetValue,
            InitValue,
            ClearValue,
        };

        Kind kind{ Kind::SetValue };
        Vector<AssetEditPathEntry> path{};
        he::schema::DynamicValue::Builder value{};
        he::schema::DynamicValue::Builder previousValue{};
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

        he::schema::Builder m_builder{};
        String name{};
        Vector<AssetEditPathEntry> path{};
        Vector<AssetEditAction> actions{};
    };

    // --------------------------------------------------------------------------------------------
    class AssetEditContext
    {
    public:
        he::schema::Builder& Builder() { return m_builder; }
        const he::schema::Builder& Builder() const { return m_builder; }

        he::schema::DynamicStruct::Builder& Data() { return m_data; }
        const he::schema::DynamicStruct::Builder& Data() const { return m_data; }

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
        he::schema::Builder m_builder{};
        he::schema::DynamicStruct::Builder m_data{};

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

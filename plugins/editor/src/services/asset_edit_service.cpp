// Copyright Chad Engler

#include "asset_edit_service.h"

#include "he/core/appender.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/enum_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/core/types.h"

#include "fmt/format.h"

namespace he::editor
{
    static schema::DynamicValue::Builder GetByPath(const schema::DynamicValue::Builder& data, const AssetEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Get(static_cast<uint16_t>(entry.listIndex));
            case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Get(entry.listIndex);
            case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Get(entry.field);
            //case schema::DynamicValue::Kind::AnyPointer:
            //{
            //    HE_ASSERT(entry.info, HE_MSG("Editing an AnyPointer field requires DeclInfo to be set."));
            //    const he::schema::AnyPointer::Reader dataPtr = data.As<he::schema::AnyPointer>();
            //    entry.
            //    // TODO: Read declInfo, check if list or struct, create a DynamicList or DynamicStruct and return it

            //    const he::schema::StructReader assetDataStruct = assetDataPtr.TryGetStruct();
            //    const he::schema::DynamicStruct::Reader assetData(*assetType->declInfo, assetDataStruct);
            //}
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return schema::DynamicValue::Builder{};
        }
    }

    static void SetByPath(schema::DynamicValue::Builder& data, const AssetEditPathEntry& entry, const schema::DynamicValue::Reader& value)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: data.As<schema::DynamicArray>().Set(static_cast<uint16_t>(entry.listIndex), value); break;
            case schema::DynamicValue::Kind::List: data.As<schema::DynamicList>().Set(entry.listIndex, value); break;
            case schema::DynamicValue::Kind::Struct: data.As<schema::DynamicStruct>().Set(entry.field, value); break;
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
        }
    }

    static schema::DynamicValue::Builder InitByPath(schema::DynamicValue::Builder& data, const AssetEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Init(static_cast<uint16_t>(entry.listIndex));
            case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Get(entry.listIndex); // No init necessary for lists of structs
            case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Init(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return schema::DynamicValue::Builder{};
        }
    }

    //static schema::DynamicValue::Builder InitByPath(schema::DynamicValue::Builder& data, const AssetEditPathEntry& entry, uint32_t size)
    //{
    //    switch (data.GetKind())
    //    {
    //        case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Init(static_cast<uint16_t>(entry.listIndex), size);
    //        case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Init(entry.listIndex, size);
    //        case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Init(entry.field, size);
    //        default:
    //            HE_VERIFY(false, HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."));
    //            return schema::DynamicValue::Builder{};
    //    }
    //}

    static void ClearByPath(schema::DynamicValue::Builder& data, const AssetEditPathEntry& entry)
    {
        if (HE_VERIFY(data.GetKind() == schema::DynamicValue::Kind::Struct,
            HE_MSG("Path is invalid, only struct fields can be cleared.")))
        {
            data.As<schema::DynamicStruct>().Clear(entry.field);
        }
    }

    static schema::DynamicValue::Builder WalkToLastPathEntry(schema::DynamicValue::Builder data, Span<const AssetEditPathEntry> path)
    {
        for (uint32_t i = 0; i < (path.Size() - 1); ++i)
        {
            data = GetByPath(data, path[i]);
        }

        return data;
    }

    void AssetEditContext::PushEdit(AssetEdit&& edit)
    {
        if (edit.actions.IsEmpty())
            return;

        for (const AssetEditAction& action : edit.actions)
        {
            if (!HE_VERIFY(!action.path.IsEmpty(),
                HE_MSG("Edit action has an empty path."),
                HE_KV(edit_name, edit.name)))
            {
                return;
            }
            // TODO: Verify paths have a declInfo for anypointer fields
        }

        if (edit.name.IsEmpty() && edit.actions.Size() == 1)
        {
            const AssetEditAction& action = edit.actions[0];
            const schema::Field::Reader field = action.path.Back().field;
            const StringView fieldName = field.GetName().AsView();

            fmt::format_to(Appender(edit.name), "{:s}: {}", action.kind, fieldName);
        }

        if (m_activeEditCount < m_edits.Size())
        {
            const uint32_t count = m_edits.Size() - m_activeEditCount;
            m_edits.Erase(m_activeEditCount, count);
        }

        m_edits.PushBack(Move(edit));
        Redo();
    }

    void AssetEditContext::Redo()
    {
        if (m_edits.IsEmpty())
            return;

        HE_ASSERT(m_activeEditCount <= m_edits.Size());

        if (m_activeEditCount == m_edits.Size())
            return;

        RedoEdit(m_edits[m_activeEditCount]);
        ++m_activeEditCount;
    }

    void AssetEditContext::Undo()
    {
        if (m_edits.IsEmpty())
            return;

        if (m_activeEditCount == 0)
            return;

        --m_activeEditCount;
        UndoEdit(m_edits[m_activeEditCount]);
    }

    void AssetEditContext::RedoEdit(AssetEdit& edit)
    {
        for (AssetEditAction& action : edit.actions)
        {
            RedoAction(action);
        }
    }

    void AssetEditContext::RedoAction(AssetEditAction& action)
    {
        schema::DynamicValue::Builder data = WalkToLastPathEntry(m_data, action.path);
        // TODO: previous value needs to live in AssetEdit::m_builder
        //action.previousValue = GetPathEntry(data, action.path.Back());

        switch (action.kind)
        {
            case AssetEditAction::Kind::AddListItem:
            case AssetEditAction::Kind::RemoveListItem:
                // TODO: Add/Remove list item
                break;
            case AssetEditAction::Kind::SetValue:
                SetByPath(data, action.path.Back(), action.value.AsReader());
                break;
            case AssetEditAction::Kind::InitValue:
                InitByPath(data, action.path.Back());
                break;
            case AssetEditAction::Kind::ClearValue:
                ClearByPath(data, action.path.Back());
                break;
        }
    }

    void AssetEditContext::UndoEdit(AssetEdit& edit)
    {
        for (AssetEditAction& action : edit.actions)
        {
            UndoAction(action);
        }
    }

    void AssetEditContext::UndoAction(AssetEditAction& action)
    {
        schema::DynamicValue::Builder data = WalkToLastPathEntry(m_data, action.path);

        switch (action.kind)
        {
            case AssetEditAction::Kind::AddListItem:
            case AssetEditAction::Kind::RemoveListItem:
                // TODO: Add/remove list item
                break;
            case AssetEditAction::Kind::SetValue:
            case AssetEditAction::Kind::InitValue:
            case AssetEditAction::Kind::ClearValue:
            {
                if (action.previousValue.GetKind() == schema::DynamicValue::Kind::Unknown)
                    ClearByPath(data, action.path.Back());
                else
                    SetByPath(data, action.path.Back(), action.previousValue.AsReader());
                break;
            }
        }
    }
}

namespace he
{
    template <>
    const char* AsString(editor::AssetEditAction::Kind x)
    {
        switch (x)
        {
            case editor::AssetEditAction::Kind::AddListItem: return "Add List Item";
            case editor::AssetEditAction::Kind::RemoveListItem: return "Remove List Item";
            case editor::AssetEditAction::Kind::SetValue: return "Set Value";
            case editor::AssetEditAction::Kind::InitValue: return "Init Value";
            case editor::AssetEditAction::Kind::ClearValue: return "Clear Value";
        }

        return "<unknown>";
    }
}

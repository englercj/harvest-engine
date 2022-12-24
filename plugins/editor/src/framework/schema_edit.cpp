// Copyright Chad Engler

#include "schema_edit.h"

#include "he/core/appender.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/enum_fmt.h"
#include "he/core/string_view_fmt.h"
#include "he/core/types.h"

#include "fmt/format.h"

namespace he::editor
{
    // --------------------------------------------------------------------------------------------
    static he::schema::DynamicValue::Builder ResolveAnyPointer(const he::schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        if (data.GetKind() != he::schema::DynamicValue::Kind::AnyPointer)
            return data;

        HE_ASSERT(entry.info, HE_MSG("Editing an AnyPointer field requires DeclInfo to be set."));
        const he::schema::AnyPointer::Builder dataPtr = data.As<he::schema::AnyPointer>();
        const he::schema::StructBuilder builder = dataPtr.TryGetStruct();
        return he::schema::DynamicStruct::Builder(*entry.info, builder);
    }

    static bool HasByPath(const he::schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case he::schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Has(static_cast<uint16_t>(entry.index));
            case he::schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Has(entry.index);
            case he::schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Has(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return false;
        }
    }

    static he::schema::DynamicValue::Builder GetByPath(const he::schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case he::schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Get(static_cast<uint16_t>(entry.index));
            case he::schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Get(entry.index);
            case he::schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Get(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return he::schema::DynamicValue::Builder{};
        }
    }

    static void SetByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry, const he::schema::DynamicValue::Reader& value)
    {
        switch (data.GetKind())
        {
            case he::schema::DynamicValue::Kind::Array: data.As<schema::DynamicArray>().Set(static_cast<uint16_t>(entry.index), value); break;
            case he::schema::DynamicValue::Kind::List: data.As<schema::DynamicList>().Set(entry.index, value); break;
            case he::schema::DynamicValue::Kind::Struct: data.As<schema::DynamicStruct>().Set(entry.field, value); break;
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
        }
    }

    static he::schema::DynamicValue::Builder InitByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case he::schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Init(static_cast<uint16_t>(entry.index));
            case he::schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Get(entry.index); // No init necessary for lists of structs
            case he::schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Init(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return he::schema::DynamicValue::Builder{};
        }
    }

    //static he::schema::DynamicValue::Builder InitByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry, uint32_t size)
    //{
    //    switch (data.GetKind())
    //    {
    //        case he::schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Init(static_cast<uint16_t>(entry.index), size);
    //        case he::schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Init(entry.index, size);
    //        case he::schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Init(entry.field, size);
    //        default:
    //            HE_VERIFY(false,
    //                HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
    //                HE_KV(kind, data.GetKind()));
    //            return he::schema::DynamicValue::Builder{};
    //    }
    //}

    static void ClearByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case he::schema::DynamicValue::Kind::Array: data.As<schema::DynamicArray>().Clear(static_cast<uint16_t>(entry.index)); break;
            case he::schema::DynamicValue::Kind::List: data.As<schema::DynamicList>().Clear(entry.index); break;
            case he::schema::DynamicValue::Kind::Struct: data.As<schema::DynamicStruct>().Clear(entry.field); break;
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
        }
    }

    static he::schema::DynamicValue::Builder CopyByPath(schema::Builder& builder, const he::schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        const he::schema::DynamicValue::Builder& value = GetByPath(data, entry);

        switch (value.GetKind())
        {
            case he::schema::DynamicValue::Kind::Unknown: return {};
            case he::schema::DynamicValue::Kind::Void: return he::schema::Void{};
            case he::schema::DynamicValue::Kind::Bool: return value.As<bool>();
            case he::schema::DynamicValue::Kind::Int: return value.As<int64_t>();
            case he::schema::DynamicValue::Kind::Uint: return value.As<uint64_t>();
            case he::schema::DynamicValue::Kind::Float: return value.As<double>();
            case he::schema::DynamicValue::Kind::Enum: return value.As<schema::DynamicEnum>();
            case he::schema::DynamicValue::Kind::Blob:
            {
                const he::schema::Blob::Builder src = value.As<schema::Blob>();
                return builder.AddBlob(src.AsBytes());
            }
            case he::schema::DynamicValue::Kind::String:
            {
                const he::schema::String::Builder src = value.As<schema::String>();
                return builder.AddString(src);
            }
            case he::schema::DynamicValue::Kind::Array:
            {
                HE_VERIFY(false,
                    HE_MSG("Arrays cannot be copied for undo operations because you cannot set the value of an array."),
                    HE_KV(kind, value.GetKind()),
                    HE_KV(parent_kind, data.GetKind()));
                return {};
            }
            case he::schema::DynamicValue::Kind::List:
            {
                const he::schema::DynamicList::Builder src = value.As<schema::DynamicList>();
                const he::schema::ElementSize elementSize = he::schema::GetTypeElementSize(src.ListType().GetElementType());
                const he::schema::ListBuilder copy = builder.AddList(elementSize, src.Size());
                return he::schema::DynamicList::Builder(src.Scope(), src.GetType(), copy);
            }
            case he::schema::DynamicValue::Kind::Struct:
            {
                const he::schema::DynamicStruct::Builder src = value.As<schema::DynamicStruct>();
                const he::schema::Declaration::Data::Struct::Reader structDecl = src.StructSchema();
                const he::schema::StructBuilder copy = builder.AddStruct(structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
                return he::schema::DynamicStruct::Builder(src.Decl(), copy);
            }
            case he::schema::DynamicValue::Kind::AnyPointer:
            {
                // TODO: implement copy for any pointer
                HE_VERIFY(false,
                    HE_MSG("AnyPointer cannot be copied for undo operations because I just haven't written it yet."),
                    HE_KV(kind, value.GetKind()),
                    HE_KV(parent_kind, data.GetKind()));
                return he::schema::DynamicValue::Builder();
            }
        }

        HE_VERIFY(false,
            HE_MSG("Value to copy is invalid, it is not of a valid kind."),
            HE_KV(kind, value.GetKind()),
            HE_KV(parent_kind, data.GetKind()));
        return {};
    }

    static he::schema::DynamicValue::Builder WalkExistingPath(schema::DynamicValue::Builder data, Span<const SchemaEditPathEntry> path, uint32_t& index)
    {
        for (; index < (path.Size() - 1); ++index)
        {
            const SchemaEditPathEntry& entry = path[index];
            if (!HasByPath(data, entry))
                break;

            data = GetByPath(data, entry);
            data = ResolveAnyPointer(data, entry);
        }

        return data;
    }

    // --------------------------------------------------------------------------------------------
    void SchemaEditContext::PushEdit(SchemaEdit&& edit)
    {
        if (edit.actions.IsEmpty())
            return;

        for (const SchemaEditAction& action : edit.actions)
        {
            if (!HE_VERIFY(!action.path.IsEmpty(),
                HE_MSG("Edit action has an empty path."),
                HE_KV(edit_name, edit.name),
                HE_KV(action_kind, action.kind)))
            {
                return;
            }
            for (const SchemaEditPathEntry& entry : action.path)
            {
                const he::schema::Field::Meta::Reader meta = entry.field.GetMeta();
                if (!meta.IsNormal())
                    continue;

                const he::schema::Field::Meta::Normal::Reader norm = meta.GetNormal();
                const he::schema::Type::Data::Reader typeData = norm.GetType().GetData();
                if (typeData.IsAnyPointer() || typeData.IsAnyList() || typeData.IsAnyStruct())
                {
                    if (!HE_VERIFY(entry.info,
                        HE_MSG("DeclInfo is required to edit AnyPointer fields."),
                        HE_KV(edit_name, edit.name),
                        HE_KV(action_kind, action.kind),
                        HE_KV(field_name, entry.field.GetName()),
                        HE_KV(field_index, entry.index)))
                    {
                        return;
                    }
                }
            }
        }

        if (edit.name.IsEmpty() && edit.actions.Size() == 1)
        {
            const SchemaEditAction& action = edit.actions[0];
            const he::schema::Field::Reader field = action.path.Back().field;
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

    void SchemaEditContext::Redo()
    {
        if (m_edits.IsEmpty())
            return;

        HE_ASSERT(m_activeEditCount <= m_edits.Size());

        if (m_activeEditCount == m_edits.Size())
            return;

        RedoEdit(m_edits[m_activeEditCount]);
        ++m_activeEditCount;
    }

    void SchemaEditContext::Undo()
    {
        if (m_edits.IsEmpty())
            return;

        if (m_activeEditCount == 0)
            return;

        --m_activeEditCount;
        UndoEdit(m_edits[m_activeEditCount]);
    }

    void SchemaEditContext::RedoEdit(SchemaEdit& edit)
    {
        for (SchemaEditAction& action : edit.actions)
        {
            RedoAction(action, edit);
        }
    }

    void SchemaEditContext::RedoAction(SchemaEditAction& action, SchemaEdit& edit)
    {
        // The flow for playing forward an action is:
        // 1. Walk the existing items of the path
        // 2. Build a 'ClearValue' undo operation for each non-existant path
        // 3. Build the undo action for the item we're actually modifying
        // 4. Init each non-existant item in the path, until we reach path[-1]
        // 5. Apply the action on that path

        uint32_t index = 0;
        he::schema::DynamicValue::Builder data = WalkExistingPath(m_data, action.path, index);

        if (action.undoActions.IsEmpty())
        {
            // if walking the existing path didn't bring us to the end
            for (uint32_t i = index; i < (action.path.Size() - 1); ++i)
            {
                SchemaEditAction undo;
                undo.kind = SchemaEditAction::Kind::ClearValue;
                undo.path.Insert(0, action.path.Begin(), action.path.Begin() + i);
                action.undoActions.PushBack(Move(undo));
            }

            const SchemaEditPathEntry& entry = action.path.Back();

            switch (action.kind)
            {
                case SchemaEditAction::Kind::AddListItem:
                case SchemaEditAction::Kind::RemoveListItem:
                    // TODO: Copy list elements so we can restore them
                    break;
                case SchemaEditAction::Kind::SetValue:
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::ClearValue;
                    undo.path = action.path;
                    if (HasByPath(data, entry))
                    {
                        undo.kind = SchemaEditAction::Kind::SetValue;
                        undo.value = CopyByPath(edit.m_builder, data, entry);
                    }
                    break;
                }
                case SchemaEditAction::Kind::InitValue:
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::ClearValue;
                    undo.path = action.path;
                    break;
                }
                case SchemaEditAction::Kind::ClearValue:
                {
                    HE_ASSERT(index == (action.path.Size() - 1));
                    HE_ASSERT(HasByPath(data, entry));
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::SetValue;
                    undo.path = action.path;
                    undo.value = CopyByPath(edit.m_builder, data, entry);
                    break;
                }
            }
        }

        for (uint32_t i = index; i < (action.path.Size() - 1); ++i)
        {
            InitByPath(data, action.path[i]);
        }
        ApplyAction(data, action);
    }

    void SchemaEditContext::UndoEdit(const SchemaEdit& edit)
    {
        const uint32_t len = edit.actions.Size();
        for (uint32_t i = (len - 1); i < len; --i)
        {
            const SchemaEditAction& action = edit.actions[i];
            UndoAction(action);
        }
    }

    void SchemaEditContext::UndoAction(const SchemaEditAction& action)
    {
        const uint32_t len = action.undoActions.Size();
        for (uint32_t i = (len - 1); i < len; --i)
        {
            const SchemaEditAction& undo = action.undoActions[i];

            uint32_t index = 0;
            he::schema::DynamicValue::Builder data = WalkExistingPath(m_data, undo.path, index);

            if (!HE_VERIFY(index == (undo.path.Size() - 1),
                HE_MSG("Undo stack is invalid, cannot perform undo operations."),
                HE_KV(undo_path_length, undo.path.Size()),
                HE_KV(undo_path_exist_length, index)))
            {
                return;
            }

            ApplyAction(data, undo);
        }
    }

    void SchemaEditContext::ApplyAction(schema::DynamicValue::Builder& data, const SchemaEditAction& action)
    {
        const SchemaEditPathEntry& entry = action.path.Back();

        switch (action.kind)
        {
            case SchemaEditAction::Kind::AddListItem:
            case SchemaEditAction::Kind::RemoveListItem:
            {
                he::schema::DynamicValue::Builder list = GetByPath(data, entry);
                if (HE_VERIFY(list.GetKind() == he::schema::DynamicValue::Kind::List))
                {
                    // TODO: Add/Remove list item
                }
                break;
            }
            case SchemaEditAction::Kind::SetValue:
            {
                SetByPath(data, entry, action.value.AsReader());
                break;
            }
            case SchemaEditAction::Kind::InitValue:
            {
                InitByPath(data, entry);
                break;
            }
            case SchemaEditAction::Kind::ClearValue:
            {
                ClearByPath(data, entry);
                break;
            }
        }
    }
}

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <>
    const char* AsString(editor::SchemaEditAction::Kind x)
    {
        switch (x)
        {
            case editor::SchemaEditAction::Kind::AddListItem: return "Add List Item";
            case editor::SchemaEditAction::Kind::RemoveListItem: return "Remove List Item";
            case editor::SchemaEditAction::Kind::SetValue: return "Set Value";
            case editor::SchemaEditAction::Kind::InitValue: return "Init Value";
            case editor::SchemaEditAction::Kind::ClearValue: return "Clear Value";
        }

        return "<unknown>";
    }
}

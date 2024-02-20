// Copyright Chad Engler

#include "he/editor/framework/schema_edit.h"

#include "he/core/fmt.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/types.h"

namespace he::editor
{
    // --------------------------------------------------------------------------------------------
    static schema::DynamicValue::Builder ResolveAnyPointer(const schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        if (data.GetKind() != schema::DynamicValue::Kind::AnyPointer)
            return data;

        HE_ASSERT(entry.info, HE_MSG("Editing an AnyPointer field requires DeclInfo to be set."));
        const schema::AnyPointer::Builder dataPtr = data.As<schema::AnyPointer>();
        const schema::StructBuilder builder = dataPtr.TryGetStruct();
        return schema::DynamicStruct::Builder(*entry.info, builder);
    }

    static bool HasByPath(const schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Has(static_cast<uint16_t>(entry.index));
            case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Has(entry.index);
            case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Has(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return false;
        }
    }

    static schema::DynamicValue::Builder GetByPath(const schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Get(static_cast<uint16_t>(entry.index));
            case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Get(entry.index);
            case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Get(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return schema::DynamicValue::Builder{};
        }
    }

    static void SetByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry, const schema::DynamicValue::Reader& value)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: data.As<schema::DynamicArray>().Set(static_cast<uint16_t>(entry.index), value); break;
            case schema::DynamicValue::Kind::List: data.As<schema::DynamicList>().Set(entry.index, value); break;
            case schema::DynamicValue::Kind::Struct: data.As<schema::DynamicStruct>().Set(entry.field, value); break;
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
        }
    }

    static schema::DynamicValue::Builder InitByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Init(static_cast<uint16_t>(entry.index));
            case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Get(entry.index); // No init necessary for lists of structs
            case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Init(entry.field);
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
                return schema::DynamicValue::Builder{};
        }
    }

    static schema::DynamicValue::Builder InitByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry, uint32_t size)
    {
       switch (data.GetKind())
       {
           case schema::DynamicValue::Kind::Array: return data.As<schema::DynamicArray>().Init(static_cast<uint16_t>(entry.index), size);
           case schema::DynamicValue::Kind::List: return data.As<schema::DynamicList>().Init(entry.index, size);
           case schema::DynamicValue::Kind::Struct: return data.As<schema::DynamicStruct>().Init(entry.field, size);
           default:
               HE_VERIFY(false,
                   HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                   HE_KV(kind, data.GetKind()));
               return schema::DynamicValue::Builder{};
       }
    }

    static void ClearByPath(schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    {
        switch (data.GetKind())
        {
            case schema::DynamicValue::Kind::Array: data.As<schema::DynamicArray>().Clear(static_cast<uint16_t>(entry.index)); break;
            case schema::DynamicValue::Kind::List: data.As<schema::DynamicList>().Clear(entry.index); break;
            case schema::DynamicValue::Kind::Struct: data.As<schema::DynamicStruct>().Clear(entry.field); break;
            default:
                HE_VERIFY(false,
                    HE_MSG("Path is invalid, it indexes into a field that is not an array, list, or struct."),
                    HE_KV(kind, data.GetKind()));
        }
    }

    // static schema::DynamicValue::Builder CopyByPath(schema::Builder& builder, const schema::DynamicValue::Builder& data, const SchemaEditPathEntry& entry)
    // {
    //     const schema::DynamicValue::Builder& value = GetByPath(data, entry);

    //     switch (value.GetKind())
    //     {
    //         case schema::DynamicValue::Kind::Unknown: return {};
    //         case schema::DynamicValue::Kind::Void: return schema::Void{};
    //         case schema::DynamicValue::Kind::Bool: return value.As<bool>();
    //         case schema::DynamicValue::Kind::Int: return value.As<int64_t>();
    //         case schema::DynamicValue::Kind::Uint: return value.As<uint64_t>();
    //         case schema::DynamicValue::Kind::Float: return value.As<double>();
    //         case schema::DynamicValue::Kind::Enum: return value.As<schema::DynamicEnum>();
    //         case schema::DynamicValue::Kind::Blob:
    //         {
    //             const schema::Blob::Builder src = value.As<schema::Blob>();
    //             return builder.AddBlob(src.AsBytes());
    //         }
    //         case schema::DynamicValue::Kind::String:
    //         {
    //             const schema::String::Builder src = value.As<schema::String>();
    //             return builder.AddString(src);
    //         }
    //         case schema::DynamicValue::Kind::Array:
    //         {
    //             HE_VERIFY(false,
    //                 HE_MSG("Arrays cannot be copied for undo operations because you cannot set the value of an array."),
    //                 HE_KV(kind, value.GetKind()),
    //                 HE_KV(parent_kind, data.GetKind()));
    //             return {};
    //         }
    //         case schema::DynamicValue::Kind::List:
    //         {
    //             const schema::DynamicList::Builder src = value.As<schema::DynamicList>();
    //             const schema::ElementSize elementSize = schema::GetTypeElementSize(src.ListType().GetElementType());
    //             const schema::ListBuilder copy = builder.AddList(elementSize, src.Size());
    //             return schema::DynamicList::Builder(src.Scope(), src.GetType(), copy);
    //         }
    //         case schema::DynamicValue::Kind::Struct:
    //         {
    //             const schema::DynamicStruct::Builder src = value.As<schema::DynamicStruct>();
    //             const schema::Declaration::Data::Struct::Reader structDecl = src.StructSchema();
    //             const schema::StructBuilder copy = builder.AddStruct(structDecl.GetDataFieldCount(), structDecl.GetDataWordSize(), structDecl.GetPointerCount());
    //             return schema::DynamicStruct::Builder(src.Decl(), copy);
    //         }
    //         case schema::DynamicValue::Kind::AnyPointer:
    //         {
    //             // TODO: implement copy for any pointer
    //             HE_VERIFY(false,
    //                 HE_MSG("AnyPointer cannot be copied for undo operations because I just haven't written it yet."),
    //                 HE_KV(kind, value.GetKind()),
    //                 HE_KV(parent_kind, data.GetKind()));
    //             return schema::DynamicValue::Builder();
    //         }
    //     }

    //     HE_VERIFY(false,
    //         HE_MSG("Value to copy is invalid, it is not of a valid kind."),
    //         HE_KV(kind, value.GetKind()),
    //         HE_KV(parent_kind, data.GetKind()));
    //     return {};
    // }

    static schema::DynamicValue::Builder WalkExistingPath(schema::DynamicValue::Builder data, Span<const SchemaEditPathEntry> path, uint32_t& index)
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
    SchemaEditContext::SchemaEditContext(const schema::DynamicStruct::Reader& data) noexcept
    {
        SetData(data);
    }

    void SchemaEditContext::SetData(const schema::DynamicStruct::Reader& data)
    {
        Clear();

        schema::Declaration::Data::Struct::Reader decl = data.StructSchema();
        schema::StructBuilder builder = m_builder.AddStruct(decl.GetDataFieldCount(), decl.GetDataWordSize(), decl.GetPointerCount());
        if (data.Struct().IsValid())
            builder.Copy(data.Struct());
        m_data = { data.Decl(), builder };
    }

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
                const schema::Field::Meta::Reader meta = entry.field.GetMeta();
                if (!meta.IsNormal())
                    continue;

                const schema::Field::Meta::Normal::Reader norm = meta.GetNormal();
                const schema::Type::Data::Reader typeData = norm.GetType().GetData();
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
            const schema::Field::Reader field = action.path.Back().field;
            const StringView fieldName = field.GetName().AsView();

            FormatTo(edit.name, "{:s}: {}", action.kind, fieldName);
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

    void SchemaEditContext::Clear()
    {
        ClearData();
        ClearEdits();
    }

    void SchemaEditContext::ClearData()
    {
        m_builder.Clear();
        m_data = {};
    }

    void SchemaEditContext::ClearEdits()
    {
        m_edits.Clear();
        m_activeEditCount = 0;
    }

    void SchemaEditContext::RedoEdit(SchemaEdit& edit)
    {
        for (SchemaEditAction& action : edit.actions)
        {
            RedoAction(action);
        }
    }

    void SchemaEditContext::RedoAction(SchemaEditAction& action)
    {
        // Pop off the element we're erasing so we can operate on the list itself. Erasing an
        // element actually creates a new list, so we want to set the original list pointer.
        SchemaEditPathEntry eraseElement;
        if (action.kind == SchemaEditAction::Kind::EraseListItem)
        {
            eraseElement = Move(action.path.Back());
            action.path.PopBack();
        }

        if (!HE_VERIFY(!action.path.IsEmpty()))
            return;

        uint32_t index = 0;
        schema::DynamicValue::Builder data = WalkExistingPath(m_data, action.path, index);

        if (action.undoActions.IsEmpty())
        {
            MakeUndoActions(data, action, index);
        }

        for (uint32_t i = index; i < (action.path.Size() - 1); ++i)
        {
            data = InitByPath(data, action.path[i]);
        }

        // Put the element back on the path right before we apply so we know which element to remove.
        if (action.kind == SchemaEditAction::Kind::EraseListItem)
        {
            action.path.PushBack(Move(eraseElement));
        }

        ApplyAction(data, action);
    }

    void SchemaEditContext::UndoEdit(const SchemaEdit& edit)
    {
        const uint32_t len = edit.actions.Size();
        for (uint32_t i = (len - 1); i != static_cast<uint32_t>(-1); --i)
        {
            const SchemaEditAction& action = edit.actions[i];
            UndoAction(action);
        }
    }

    void SchemaEditContext::UndoAction(const SchemaEditAction& action)
    {
        const uint32_t len = action.undoActions.Size();
        for (uint32_t i = (len - 1); i != static_cast<uint32_t>(-1); --i)
        {
            const SchemaEditAction& undo = action.undoActions[i];

            uint32_t index = 0;
            schema::DynamicValue::Builder data = WalkExistingPath(m_data, undo.path, index);

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

    void SchemaEditContext::MakeUndoActions(schema::DynamicValue::Builder data, SchemaEditAction& action, uint32_t index)
    {
        // If walking the existing path didn't bring us to the end then we need to create a
        // 'ClearValue' action for each non-existant path. This is so that when this edit is
        // undone we can restore the original state of the object.
        for (uint32_t i = index; i < (action.path.Size() - 1); ++i)
        {
            SchemaEditAction& undo = action.undoActions.EmplaceBack();
            undo.kind = SchemaEditAction::Kind::ClearValue;
            undo.path.Insert(0, action.path.Begin(), action.path.Begin() + i);

            data = GetByPath(data, action.path[i]);
        }

        const SchemaEditPathEntry& entry = action.path.Back();

        switch (action.kind)
        {
            case SchemaEditAction::Kind::AddListItem:
            {
                if (HasByPath(data, entry))
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::SetValue;
                    undo.path = action.path;
                    undo.value = GetByPath(data, entry);
                }
                else
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::ClearValue;
                    undo.path = action.path;
                }
                break;
            }
            case SchemaEditAction::Kind::EraseListItem:
            {
                if (HE_VERIFY(HasByPath(data, entry)))
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::SetValue;
                    undo.path = action.path;
                    undo.value = GetByPath(data, entry);
                }
                break;
            }
            case SchemaEditAction::Kind::SetValue:
            {
                if (HasByPath(data, entry))
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::SetValue;
                    undo.path = action.path;
                    undo.value = GetByPath(data, entry);
                }
                else
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::ClearValue;
                    undo.path = action.path;
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
                if (HE_VERIFY(HasByPath(data, entry)))
                {
                    SchemaEditAction& undo = action.undoActions.EmplaceBack();
                    undo.kind = SchemaEditAction::Kind::SetValue;
                    undo.path = action.path;
                    undo.value = GetByPath(data, entry);
                }
                break;
            }
        }
    }

    void SchemaEditContext::ApplyAction(schema::DynamicValue::Builder& data, const SchemaEditAction& action)
    {
        const SchemaEditPathEntry& entry = action.path.Back();

        switch (action.kind)
        {
            case SchemaEditAction::Kind::AddListItem:
            {
                // TODO: Redo of this action creates a new list each time.
                // Ideally we'd only create the new list once, then undo/redo just switches between
                // pointers to the two lists. Maybe change the action kind after creating the list?
                // Same for `EraseListItem`.

                if (HasByPath(data, entry))
                {
                    schema::DynamicValue::Builder listData = GetByPath(data, entry);
                    if (HE_VERIFY(listData.GetKind() == schema::DynamicValue::Kind::List))
                    {
                        schema::DynamicList::Builder list = listData.As<schema::DynamicList>();
                        schema::DynamicList::Builder newList = list.Insert(list.Size(), action.value.AsReader());
                        SetByPath(data, entry, newList.AsReader());
                    }
                }
                else
                {
                    InitByPath(data, entry, 1);
                }
                break;
            }
            case SchemaEditAction::Kind::EraseListItem:
            {
                const SchemaEditPathEntry& listEntry = action.path[action.path.Size() - 2];

                if (HE_VERIFY(HasByPath(data, listEntry)))
                {
                    schema::DynamicValue::Builder listValue = GetByPath(data, listEntry);
                    schema::DynamicList::Builder list = listValue.As<schema::DynamicList>();
                    schema::DynamicList::Builder newList = list.Erase(entry.index, 1);
                    SetByPath(data, listEntry, newList.AsReader());
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
            case editor::SchemaEditAction::Kind::EraseListItem: return "Erase List Item";
            case editor::SchemaEditAction::Kind::SetValue: return "Set Value";
            case editor::SchemaEditAction::Kind::InitValue: return "Init Value";
            case editor::SchemaEditAction::Kind::ClearValue: return "Clear Value";
        }

        return "<unknown>";
    }
}

// Copyright Chad Engler

#include "he/editor/services/type_edit_ui_service.h"

namespace he::editor
{
    static schema::TypeId GetFieldTypeId(schema::Field::Reader field)
    {
        const schema::Field::Meta::Reader meta = field.GetMeta();

        switch (meta.GetUnionTag())
        {
            case schema::Field::Meta::UnionTag::Normal:
            {
                const schema::Type::Data::Reader type = meta.GetNormal().GetType().GetData();
                return type.IsStruct() ? type.GetStruct().GetId() : 0;
            }
            case schema::Field::Meta::UnionTag::Group: return meta.GetGroup().GetTypeId();
            case schema::Field::Meta::UnionTag::Union: return meta.GetUnion().GetTypeId();
        }

        return 0;
    }

    const TypeEditUIService::Editor* TypeEditUIService::FindEditor(schema::TypeId typeId) const
    {
        if (!schema::IsTypeId(typeId))
            return nullptr;

        return m_typeEditors.Find(typeId);
    }

    const TypeEditUIService::Editor* TypeEditUIService::FindEditor(schema::Field::Reader field) const
    {
        if (!field.IsValid())
            return nullptr;

        const Editor* editor = m_fieldEditors.Find(field.Data());
        if (editor)
            return editor;

        const schema::TypeId typeId = GetFieldTypeId(field);
        return FindEditor(typeId);
    }

    void TypeEditUIService::RegisterTypeEditor(schema::TypeId typeId, Editor&& editor)
    {
        if (!HE_VERIFY(schema::IsTypeId(typeId)))
            return;

        const auto result = m_typeEditors.Emplace(typeId, Move(editor));
        HE_VERIFY(result.inserted,
            HE_MSG("An editor for this type has already been registered."),
            HE_KV(type_id, typeId));
    }

    void TypeEditUIService::UnregisterTypeEditor(schema::TypeId typeId)
    {
        if (!HE_VERIFY(schema::IsTypeId(typeId)))
            return;

        const bool result = m_typeEditors.Erase(typeId);
        HE_VERIFY(result,
            HE_MSG("No editor for this type has been registered."),
            HE_KV(type_id, typeId));
    }

    void TypeEditUIService::RegisterFieldEditor(schema::Field::Reader field, Editor&& editor)
    {
        if (!HE_VERIFY(field.IsValid()))
            return;

        const auto result = m_fieldEditors.Emplace(field.Data(), Move(editor));
        HE_VERIFY(result.inserted,
            HE_MSG("An editor for this field has already been registered."),
            HE_KV(name, field.GetName()));
    }

    void TypeEditUIService::UnregisterFieldEditor(schema::Field::Reader field)
    {
        if (!HE_VERIFY(field.IsValid()))
            return;

        const bool result = m_fieldEditors.Erase(field.Data());
        HE_VERIFY(result,
            HE_MSG("No editor for this field has been registered."),
            HE_KV(name, field.GetName()));
    }
}

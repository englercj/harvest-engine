// Copyright Chad Engler

#include "type_edit_ui_service.h"

namespace he::editor
{
    static he::schema::TypeId GetFieldTypeId(he::schema::Field::Reader field)
    {
        const he::schema::Field::Meta::Reader meta = field.GetMeta();

        switch (meta.GetUnionTag())
        {
            case he::schema::Field::Meta::UnionTag::Normal:
            {
                const he::schema::Type::Data::Reader type = meta.GetNormal().GetType().GetData();
                return type.IsStruct() ? type.GetStruct().GetId() : 0;
            }
            case he::schema::Field::Meta::UnionTag::Group: return meta.GetGroup().GetTypeId();
            case he::schema::Field::Meta::UnionTag::Union: return meta.GetUnion().GetTypeId();
        }

        return 0;
    }

    void TypeEditUIService::RegisterTypeEditor(he::schema::TypeId typeId, Editor&& editor)
    {
        if (!HE_VERIFY(he::schema::IsTypeId(typeId)))
            return;

        const auto result = m_typeEditors.Emplace(typeId, Move(editor));
        HE_VERIFY(result.inserted,
            HE_MSG("An editor for this type has already been registered."),
            HE_KV(type_id, typeId));
    }

    void TypeEditUIService::RegisterFieldEditor(he::schema::Field::Reader field, Editor&& editor)
    {
        if (!HE_VERIFY(field.IsValid()))
            return;

        const auto result = m_fieldEditors.Emplace(field.Data(), Move(editor));
        HE_VERIFY(result.inserted,
            HE_MSG("An editor for this field has already been registered."),
            HE_KV(name, field.GetName()));
    }

    const TypeEditUIService::Editor* TypeEditUIService::FindEditor(he::schema::TypeId typeId) const
    {
        if (!he::schema::IsTypeId(typeId))
            return nullptr;

        return m_typeEditors.Find(typeId);
    }

    const TypeEditUIService::Editor* TypeEditUIService::FindEditor(he::schema::Field::Reader field) const
    {
        if (!field.IsValid())
            return nullptr;

        const Editor* editor = m_fieldEditors.Find(field.Data());
        if (editor)
            return editor;

        const he::schema::TypeId typeId = GetFieldTypeId(field);
        return FindEditor(typeId);
    }
}

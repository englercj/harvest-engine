// Copyright Chad Engler

#include "type_edit_ui_service.h"

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

    void TypeEditUIService::RegisterTypeEditor(schema::TypeId typeId, Editor&& editor)
    {
        if (!HE_VERIFY(schema::IsTypeId(typeId)))
            return;

        const auto pair = m_typeEditors.try_emplace(typeId, Move(editor));
        HE_VERIFY(pair.second,
            HE_MSG("An editor for this type has already been registered."),
            HE_KV(type_id, typeId));
    }

    void TypeEditUIService::RegisterFieldEditor(schema::Field::Reader field, Editor&& editor)
    {
        if (!HE_VERIFY(field.IsValid()))
            return;

        const auto pair = m_fieldEditors.try_emplace(field.Data(), Move(editor));
        HE_VERIFY(pair.second,
            HE_MSG("An editor for this field has already been registered."),
            HE_KV(name, field.GetName()));
    }

    const TypeEditUIService::Editor* TypeEditUIService::FindEditor(schema::TypeId typeId) const
    {
        if (!schema::IsTypeId(typeId))
            return nullptr;

        const auto it = m_typeEditors.find(typeId);
        if (it == m_typeEditors.end())
            return nullptr;

        return &it->second;
    }

    const TypeEditUIService::Editor* TypeEditUIService::FindEditor(schema::Field::Reader field) const
    {
        if (!field.IsValid())
            return nullptr;

        const auto it = m_fieldEditors.find(field.Data());
        if (it != m_fieldEditors.end())
            return &it->second;

        const schema::TypeId typeId = GetFieldTypeId(field);
        return FindEditor(typeId);
    }
}

// Copyright Chad Engler

#include "type_edit_ui_service.h"

namespace he::editor
{
    void TypeEditUIService::RegisterTypeEditor(const TypeInfo& info, EditorDelegate editor)
    {
        const auto pair = m_editors.try_emplace(info, editor);
        HE_VERIFY(pair.second,
            HE_MSG("An editor for this type has already been registered"),
            HE_KV(name, info.Name()),
            HE_KV(hash, info.Hash()));
    }

    TypeEditUIService::ShowResult TypeEditUIService::ShowTypeEditor(const TypeInfo& info, const void* value, schema::StructBuilder data, Span<const schema::Field::Reader> path)
    {
        const auto it = m_editors.find(info);

        if (it == m_editors.end())
            return ShowResult::NoEditor;

        if (it->second(value, data, path))
            return ShowResult::ValueChanged;

        return ShowResult::ValueUnchanged;
    }
}

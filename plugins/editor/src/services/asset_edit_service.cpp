// Copyright Chad Engler

#include "asset_edit_service.h"

#include "he/core/appender.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/string_view_fmt.h"

#include "fmt/format.h"

namespace he::editor
{
    class StructValueSetter : public schema::StructVisitor
    {
    public:
        StructValueSetter(AssetEditAction& action)
            : m_path(action.path)
        {}

    private:
        void VisitField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            if (m_path[m_index].field.Data() == field.Data())
            {
                ++m_index;
                schema::StructVisitor::VisitField(data, field, scope);
            }
        }

        void VisitNormalField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            // TODO: If the field is a pointer we need to create it before continuing.
            schema::StructVisitor::VisitNormalField(data, field, scope);
        }

        void VisitUnionField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            // TODO: Set the tag of the union to something else, then visit the newly set one.
            schema::StructVisitor::VisitUnionField(data, field, scope);
        }

        void VisitValue(schema::StructReader data, schema::Type::Reader type, uint16_t index, uint32_t dataOffset, const schema::DeclInfo& scope) override
        {
            // TODO: cache the StructReader as a builder so when we visit a field's value we can set it.
            schema::StructVisitor::VisitValue(data, type, index, dataOffset, scope);
        }

        void VisitValue(schema::ListReader data, schema::Type::Reader elementType, uint32_t index, const schema::DeclInfo& scope) override
        {
            // TODO: cache the ListReader as a builder so when we visit a field's value we can set it.
            schema::StructVisitor::VisitValue(data, elementType, index, scope);
        }

        bool ShouldVisitNormalField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(data, field, scope);
            return true;
        }

        bool ShouldVisitGroupField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(data, field, scope);
            return true;
        }

        bool ShouldVisitUnionField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(data, field, scope);
            return true;
        }

    private:
        schema::StructBuilder AsBuilder(schema::StructReader reader, schema::Builder& builder)
        {
            HE_ASSERT(reader.Data() >= builder.Data() && reader.Data() < builder.Data() + builder.Size());
            const uint32_t wordOffset = static_cast<uint32_t>(reader.Data() - builder.Data());
            return schema::StructBuilder(&builder, wordOffset, reader.DataFieldCount(), reader.DataWordSize(), reader.PointerCount());
        }

        schema::ListBuilder AsBuilder(schema::ListReader reader, schema::Builder& builder)
        {
            HE_ASSERT(reader.Data() >= builder.Data() && reader.Data() < builder.Data() + builder.Size());
            const uint32_t wordOffset = static_cast<uint32_t>(reader.Data() - builder.Data());

            if (reader.GetElementSize() == schema::ElementSize::Composite)
                return schema::ListBuilder(&builder, wordOffset, reader.Size(), reader.StepSize(), reader.StructDataFieldCount());

            return schema::ListBuilder(&builder, wordOffset, reader.Size(), reader.StepSize(), reader.GetElementSize());
        }

    private:
        uint32_t m_index{ 0 };
        Span<const AssetEditPathEntry> m_path;
    };

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
        switch (action.kind)
        {
            case AssetEditAction::Kind::AddListItem:
            case AssetEditAction::Kind::RemoveListItem:
            case AssetEditAction::Kind::SetValue:
            case AssetEditAction::Kind::ResetToDefault:
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
        switch (action.kind)
        {
            case AssetEditAction::Kind::AddListItem:
            case AssetEditAction::Kind::RemoveListItem:
            case AssetEditAction::Kind::SetValue:
            case AssetEditAction::Kind::ResetToDefault:
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
            case editor::AssetEditAction::Kind::ResetToDefault: return "Reset To Default";
        }

        return "<unknown>";
    }
}

// Copyright Chad Engler

#include "property_grid.h"

#include "input_text.h"
#include "fonts/icons_material_design.h"
#include "services/asset_edit_service.h"

#include "he/assets/types.h"
#include "he/core/ascii.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/schema/schema.h"

namespace he::editor
{
    static int s_sectionIndex{ 0 };
    static int s_tableIndex{ 0 };
    static int s_rowIndex{ 0 };

    bool BeginPropertyGrid(ImGuiID id)
    {
        ImGui::PushID(id);
        ImGui::PushID("##pg");
        s_sectionIndex = 0;
        s_tableIndex = 0;
        s_rowIndex = 0;
        return true;
    }

    void EndPropertyGrid()
    {
        ImGui::PopID();
        ImGui::PopID();
    }

    bool BeginPropertyGridHeader(String* searchText)
    {
        ImGui::PushID("##pg-header");

        if (searchText)
        {
            ImGui::TextUnformatted(ICON_MDI_MAGNIFY);
            InputText("##search", *searchText);
        }
        return true;
    }

    void EndPropertyGridHeader()
    {
        ImGui::PopID();
    }

    bool BeginPropertyGridSection(const char* label)
    {
        if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_CollapsingHeader))
        {
            ImGui::PushID(s_sectionIndex++);
            s_tableIndex = 0;
            return true;
        }

        return false;
    }

    void EndPropertyGridSection()
    {
        ImGui::PopID();
        ImGui::EndTable();
    }

    bool BeginPropertyGridTable()
    {
        const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable("##pg-table", 3, flags))
        {
            ImGui::PushID(s_tableIndex++);
            s_rowIndex = 0;

            ImGui::TableSetupColumn("##pg-col-name", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##pg-col-value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##pg-col-actions", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            return true;
        }

        return false;
    }

    void EndPropertyGridTable()
    {
        ImGui::PopID();
        ImGui::EndTable();
    }

    bool BeginPropertyGridRow()
    {
        ImGui::PushID(s_rowIndex++);
        ImGui::TableNextRow();
        return true;
    }

    void EndPropertyGridRow()
    {
        ImGui::PopID();
    }

    class PropertyGridStructVisitor : public schema::DynamicStructVisitor<schema::DynamicStruct::Builder>
    {
    public:
        using SuperType = schema::DynamicStructVisitor<schema::DynamicStruct::Builder>;

    public:
        PropertyGridStructVisitor(AssetEdit& edit) : m_edit(edit) {}

        AssetEdit& Edit() { return m_edit; }

    private:
        void VisitStruct(schema::DynamicStruct::Builder& data) override
        {
            // For the root struct just visit it normally
            if (m_edit.path.IsEmpty())
            {
                SuperType::VisitStruct(data);
                return;
            }

            const schema::Field::Reader field = m_edit.path.Back().field;

            StringView name = field.GetName();
            StringView desc;
            GetNameAndDescription(name, desc, field.GetAttributes());

            bool open = false;
            if (BeginPropertyGridRow())
            {
                // Name column
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                open = ImGui::TreeNodeEx(name.Data(), flags);

                if (!desc.IsEmpty() && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", desc.Data());

                // Value column
                ImGui::TableNextColumn();
                if (data.StructSchema().GetIsUnion())
                {
                    const schema::Field::Reader activeField = data.ActiveUnionField();
                    StringView activeFieldName = field.GetName();
                    StringView activeFieldDesc;
                    GetNameAndDescription(activeFieldName, activeFieldDesc, field.GetAttributes());

                    if (ImGui::BeginCombo("##union-field-type", activeFieldName.Data()))
                    {
                        for (const schema::Field::Reader& unionField : data.StructSchema().GetFields())
                        {
                            StringView unionFieldName = unionField.GetName();
                            StringView unionFieldDesc;
                            GetNameAndDescription(unionFieldName, unionFieldDesc, unionField.GetAttributes());

                            const bool isSelected = unionField.GetUnionTag() == activeField.GetUnionTag();
                            if (ImGui::Selectable(unionFieldName.Data(), isSelected))
                            {
                                data.Clear(unionField);
                            }

                            if (!unionFieldDesc.IsEmpty() && ImGui::IsItemHovered())
                                ImGui::SetTooltip("%s", unionFieldDesc.Data());

                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }
                }

                // Actions column
                ImGui::TableNextColumn();
                RevertActionButton(data);

                EndPropertyGridRow();
            }

            if (open)
            {
                const ImVec2& padding = ImGui::GetStyle().FramePadding;
                ImGui::Indent(padding.x);
                ImGui::PushID(name.Data());
                SuperType::VisitStruct(data);
                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        void VisitField(schema::DynamicStruct::Builder& data, schema::Field::Reader field) override
        {
            m_edit.path.PushBack({ field });
            SuperType::VisitField(data, field);
            m_edit.path.PopBack();
        }

        void VisitNormalField(schema::DynamicStruct::Builder& data, schema::Field::Reader field) override
        {
            StringView name = field.GetName();
            StringView desc;
            GetNameAndDescription(name, desc, field.GetAttributes());

            const schema::Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const schema::Type::Reader type = norm.GetType();
            const schema::Type::Data::Reader typeData = type.GetData();

            // Use the default visit behavior for struct fields.
            if (typeData.IsStruct() || typeData.IsAnyStruct())
            {
                SuperType::VisitNormalField(data, field);
                return;
            }

            bool open = false;
            if (BeginPropertyGridRow())
            {
                // Name column
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                if (typeData.IsArray() || typeData.IsList())
                {
                    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    open = ImGui::TreeNodeEx(name.Data(), flags);
                }
                else
                {
                    const float framePaddingX = ImGui::GetStyle().FramePadding.x;
                    const float fontSize = ImGui::GetFontSize();
                    const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
                    ImGui::Dummy(ImVec2{ fontSize + (framePaddingX * 2) - itemSpacing, 0 });
                    ImGui::SameLine();
                    ImGui::TextUnformatted(name.Begin(), name.End());
                }

                if (!desc.IsEmpty() && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", desc.Data());

                // Value column
                ImGui::TableNextColumn();
                if (typeData.IsList())
                {
                    if (ImGui::Button(ICON_MDI_PLUS " Add Item"))
                    {
                        // TODO: Add list item
                    }
                }
                else if (!typeData.IsArray())
                {
                    schema::DynamicValue::Builder value = data.Get(field);
                    SuperType::VisitValue(value);
                }

                // Actions column
                ImGui::TableNextColumn();
                RevertActionButton(data);

                EndPropertyGridRow();
            }

            if (open)
            {
                const ImVec2& padding = ImGui::GetStyle().FramePadding;
                ImGui::Indent(padding.x);
                ImGui::PushID(name.Data());

                schema::DynamicValue::Builder value = data.Get(field);
                SuperType::VisitValue(value);

                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        void VisitValue(const schema::DynamicValue::Builder& value) override
        {
            const schema::Field::Reader field = m_edit.path.Back().field;

            switch (value.GetKind())
            {
                case schema::DynamicValue::Kind::Unknown: ImGui::TextUnformatted("Unknown DynamicValue type."); break; //TODO: Log once? VERIFY once?
                case schema::DynamicValue::Kind::Void: ShowValueEditor(value.As<schema::Void>()); break;
                case schema::DynamicValue::Kind::Bool: ShowValueEditor(value.As<bool>()); break;
                case schema::DynamicValue::Kind::Blob: ShowValueEditor(value.As<schema::Blob>()); break;
                case schema::DynamicValue::Kind::String: ShowValueEditor(value.As<schema::String>()); break;
                case schema::DynamicValue::Kind::Array: ShowValueEditor(value.As<schema::DynamicArray>()); break;
                case schema::DynamicValue::Kind::List: ShowValueEditor(value.As<schema::DynamicList>()); break;
                case schema::DynamicValue::Kind::Enum: ShowValueEditor(value.As<schema::DynamicEnum>()); break;
                case schema::DynamicValue::Kind::AnyPointer: ShowValueEditor(value.As<schema::AnyPointer>()); break;
                case schema::DynamicValue::Kind::Int:
                case schema::DynamicValue::Kind::Uint:
                case schema::DynamicValue::Kind::Float:
                {
                    const schema::Type::Data::Reader typeData = field.GetMeta().GetNormal().GetType().GetData();
                    if (typeData.IsInt8())
                        ShowValueEditor(value.As<int8_t>());
                    else if (typeData.IsInt16())
                        ShowValueEditor(value.As<int16_t>());
                    else if (typeData.IsInt32())
                        ShowValueEditor(value.As<int32_t>());
                    else if (typeData.IsInt64())
                        ShowValueEditor(value.As<int64_t>());
                    else if (typeData.IsUint8())
                        ShowValueEditor(value.As<uint8_t>());
                    else if (typeData.IsUint16())
                        ShowValueEditor(value.As<uint16_t>());
                    else if (typeData.IsUint32())
                        ShowValueEditor(value.As<uint32_t>());
                    else if (typeData.IsUint64())
                        ShowValueEditor(value.As<uint64_t>());
                    else if (typeData.IsFloat32())
                        ShowValueEditor(value.As<float>());
                    else if (typeData.IsFloat64())
                        ShowValueEditor(value.As<double>());
                    else
                        ImGui::TextUnformatted("Unknown DynamicValue type."); // TODO: Log once? VERIFY once?
                    break;
                }
                case schema::DynamicValue::Kind::Struct:
                {
                    schema::DynamicStruct::Builder builder = value.As<schema::DynamicStruct>();
                    VisitStruct(builder);
                    break;
                }
            }
        }

        bool ShouldVisitNormalField(const schema::DynamicStruct::Builder& data, schema::Field::Reader field) override
        {
            HE_UNUSED(data);
            const bool hidden = schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

        bool ShouldVisitGroupField(const schema::DynamicStruct::Builder& data, schema::Field::Reader field) override
        {
            HE_UNUSED(data);
            const bool hidden = schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

        bool ShouldVisitUnionField(const schema::DynamicStruct::Builder& data, schema::Field::Reader field) override
        {
            HE_UNUSED(data);
            const bool hidden = schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

    private:
        void RevertActionButton(schema::DynamicStruct::Builder& data)
        {
            const schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
            const bool hasValue = data.Has(field);

            if (!readOnly && hasValue)
            {
                if (ImGui::Button(ICON_MDI_UNDO_VARIANT))
                {
                    data.Clear(field);
                }
            }
        }

        StringView FormatName(StringView name)
        {
            static String s_buf;

            s_buf.Clear();

            if (name.IsEmpty())
                return {};

            s_buf.Reserve(name.Size() + 8);

            const char* p = name.Begin();
            const char* end = name.End();

            s_buf.PushBack(ToUpper(*p));
            ++p;

            while (p < end)
            {
                if (IsLower(p[-1]) && IsUpper(*p))
                    s_buf.PushBack(' ');
                s_buf.PushBack(*p);
                ++p;
            }

            return s_buf;
        }

        void GetNameAndDescription(StringView& name, StringView& desc, schema::List<schema::Attribute>::Reader attributes)
        {
            const schema::Attribute::Reader displayNameAttr = schema::FindAttribute<assets::schema::Display::Name>(attributes);
            const schema::Attribute::Reader descriptionAttr = schema::FindAttribute<assets::schema::Display::Description>(attributes);

            if (displayNameAttr.IsValid())
                name = displayNameAttr.GetValue().GetData().GetString();
            else
                name = FormatName(name);

            if (descriptionAttr.IsValid())
                desc = descriptionAttr.GetValue().GetData().GetString();
        }

        template <typename T>
        bool InputScalar(T& v, ImGuiDataType type)
        {
            const schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            bool changed = false;

            ImGui::BeginDisabled(readOnly);
            if (ImGui::InputScalar("##scalar-value", type, &v))
            {
                changed = true;
            }
            if (readOnly && ImGui::IsItemHovered())
                ImGui::SetTooltip("This field is read-only.");
            ImGui::EndDisabled();

            return changed;
        }

        void ShowValueEditor(schema::Void value)
        {
            HE_UNUSED(value);
            ImGui::TextUnformatted("void");
        }

        void ShowValueEditor(bool value)
        {
            const schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            bool v = value;

            ImGui::BeginDisabled(readOnly);
            if (ImGui::Checkbox("##bool-value", &v))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetBool(v);
            }
            if (readOnly && ImGui::IsItemHovered())
                ImGui::SetTooltip("This field is read-only.");
            ImGui::EndDisabled();
        }

        void ShowValueEditor(int8_t value)
        {
            int8_t v = value;
            if (InputScalar(v, ImGuiDataType_S8))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt8(v);
            }
        }

        void ShowValueEditor(int16_t value)
        {
            int16_t v = value;
            if (InputScalar(v, ImGuiDataType_S16))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt16(v);
            }
        }

        void ShowValueEditor(int32_t value)
        {
            int32_t v = value;
            if (InputScalar(v, ImGuiDataType_S32))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt32(v);
            }
        }

        void ShowValueEditor(int64_t value)
        {
            int64_t v = value;
            if (InputScalar(v, ImGuiDataType_S64))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt64(v);
            }
        }

        void ShowValueEditor(uint8_t value)
        {
            uint8_t v = value;
            if (InputScalar(v, ImGuiDataType_U8))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint8(v);
            }
        }

        void ShowValueEditor(uint16_t value)
        {
            uint16_t v = value;
            if (InputScalar(v, ImGuiDataType_U16))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint16(v);
            }
        }

        void ShowValueEditor(uint32_t value)
        {
            uint32_t v = value;
            if (InputScalar(v, ImGuiDataType_U32))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint32(v);
            }
        }

        void ShowValueEditor(uint64_t value)
        {
            uint64_t v = value;
            if (InputScalar(v, ImGuiDataType_U64))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint64(v);
            }
        }

        void ShowValueEditor(float value)
        {
            float v = value;
            if (InputScalar(v, ImGuiDataType_Float))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetFloat32(v);
            }
        }

        void ShowValueEditor(double value)
        {
            double v = value;
            if (InputScalar(v, ImGuiDataType_Double))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetFloat64(v);
            }
        }

        void ShowValueEditor(schema::Blob::Reader value)
        {
            // TODO: What editor do we show for blobs? None?
            HE_UNUSED(value);
            ImGui::TextUnformatted("Binary Data");
        }

        void ShowValueEditor(schema::String::Reader value)
        {
            const schema::Field::Reader field = m_edit.path.Back().field;

            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            static String v;
            v = value;

            const ImGuiInputTextFlags flags = readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

            ImGui::BeginDisabled(readOnly);
            if (InputText("##string-value", v, flags))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().InitString(v);
            }
            if (readOnly && ImGui::IsItemHovered())
                ImGui::SetTooltip("This field is read-only.");
            ImGui::EndDisabled();
        }

        void ShowValueEditor(const schema::DynamicArray::Builder& value)
        {
            for (uint16_t i = 0; i < value.Size(); ++i)
            {
                // TODO: Edit each element - share VisitNormalField code?
                schema::DynamicValue::Builder v = value.Get(i);
            }
        }

        void ShowValueEditor(const schema::DynamicList::Builder& value)
        {
            for (uint16_t i = 0; i < value.Size(); ++i)
            {
                // TODO: Edit each element
            }
        }

        void ShowValueEditor(schema::DynamicEnum value)
        {
            const schema::Declaration::Data::Enum::Reader enumDecl = value.EnumSchema();

            const char* valueName = "Select...";
            for (const schema::Enumerator::Reader e : enumDecl.GetEnumerators())
            {
                if (e.GetOrdinal() == value.Value())
                {
                    valueName = e.GetName().Data();
                    break;
                }
            }

            if (ImGui::BeginCombo("##enum-value", valueName))
            {
                for (const schema::Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    StringView name = e.GetName();
                    StringView desc;
                    GetNameAndDescription(name, desc, e.GetAttributes());

                    const bool isSelected = e.GetOrdinal() == value.Value();
                    if (ImGui::Selectable(name.Data(), isSelected))
                    {
                        AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                        action.value.value.GetData().SetEnum(e.GetOrdinal());
                    }

                    if (!desc.IsEmpty() && ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", desc.Data());

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
        }

        void ShowValueEditor(schema::AnyPointer::Reader ptr)
        {
            // TODO
            HE_UNUSED(ptr);
            ImGui::TextUnformatted("Any Pointer");
        }

    private:
        AssetEdit& m_edit;
    };

    void PropertyGrid(schema::DynamicStruct::Builder& data, AssetEdit& edit)
    {
        const ImGuiID id = ImGui::GetID(&edit);
        if (BeginPropertyGrid(id))
        {
            if (BeginPropertyGridHeader())
            {
                EndPropertyGridHeader();
            }

            if (BeginPropertyGridTable())
            {
                PropertyGridStructVisitor visitor(edit);
                visitor.Visit(data);

                EndPropertyGridTable();
            }

            EndPropertyGrid();
        }
    }
}

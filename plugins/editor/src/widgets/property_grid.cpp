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

    class PropertyGridStructVisitor : public schema::StructVisitor
    {
    public:
        PropertyGridStructVisitor(AssetEdit& edit) : m_edit(edit) {}

        AssetEdit& Edit() { return m_edit; }

    private:
        void VisitStruct(schema::StructReader data, const schema::DeclInfo& info) override
        {
            const schema::Declaration::Reader decl = schema::GetSchema(info);
            const schema::Field::Reader field = m_edit.path.Back().field;

            // For the root struct just visit it normally
            if (!field.IsValid())
            {
                schema::StructVisitor::VisitStruct(data, info);
                return;
            }

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
                // TODO: Anything need to go here? Custom editor for struct types?

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
                schema::StructVisitor::VisitStruct(data, info);
                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        void VisitNormalField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            m_edit.path.PushBack({ field });
            HE_AT_SCOPE_EXIT([&]() { m_edit.path.PopBack(); });

            StringView name = field.GetName();
            StringView desc;
            GetNameAndDescription(name, desc, field.GetAttributes());

            const schema::Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const schema::Type::Reader type = norm.GetType();
            const schema::Type::Data::Reader typeData = type.GetData();

            const uint16_t index = norm.GetIndex();
            const uint32_t dataOffset = norm.GetDataOffset();

            if (typeData.IsStruct() || typeData.IsAnyStruct())
            {
                schema::StructVisitor::VisitNormalField(data, field, scope);
                return;
            }

            if (typeData.IsArray())
            {
                // TODO: Custom editor
                return;
            }

            if (typeData.IsList())
            {
                // TODO: Custom editor
                return;
            }

            if (BeginPropertyGridRow())
            {
                // Name column
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                const float framePaddingX = ImGui::GetStyle().FramePadding.x;
                const float fontSize = ImGui::GetFontSize();
                const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
                ImGui::Dummy(ImVec2{ fontSize + (framePaddingX * 2) - itemSpacing, 0 });
                ImGui::SameLine();
                ImGui::TextUnformatted(name.Begin(), name.End());
                if (!desc.IsEmpty() && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", desc.Data());

                // Value column
                ImGui::TableNextColumn();
                const bool isPointer = schema::IsPointer(type);
                const bool hasValue = isPointer ? data.HasPointerField(index) : data.HasDataField(index);

                if (hasValue)
                    schema::StructVisitor::VisitValue(data, type, index, dataOffset, scope);
                else if (norm.HasDefaultValue())
                    schema::StructVisitor::VisitValue(norm.GetDefaultValue(), type, scope);
                else
                    VisitDefaultValue(type, scope);

                // Actions column
                ImGui::TableNextColumn();
                RevertActionButton(data);

                EndPropertyGridRow();
            }
        }

        void VisitGroupField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope)
        {
            m_edit.path.PushBack({ field });
            HE_AT_SCOPE_EXIT([&]() { m_edit.path.PopBack(); });

            schema::StructVisitor::VisitGroupField(data, field, scope);
        }

        void VisitUnionField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            m_edit.path.PushBack({ field });
            HE_AT_SCOPE_EXIT([&]() { m_edit.path.PopBack(); });

            const schema::DeclInfo* info = FindGroupOrUnionInfo(field, scope);
            if (!info)
                return;

            const schema::Declaration::Reader decl = GetSchema(*info);
            const schema::Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
            const schema::List<schema::Field>::Reader fields = structDecl.GetFields();

            const uint16_t activeFieldTag = data.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());

            schema::Field::Reader activeField;
            for (const schema::Field::Reader& unionField : fields)
            {
                if (unionField.GetUnionTag() == activeFieldTag)
                {
                    activeField = unionField;
                    break;
                }
            }

            if (!HE_VERIFY(activeField.IsValid()))
                return;

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
                StringView activeFieldName = field.GetName();
                StringView activeFieldDesc;
                GetNameAndDescription(activeFieldName, activeFieldDesc, field.GetAttributes());

                ImGui::TableNextColumn();
                if (ImGui::BeginCombo("##union-field-type", activeFieldName.Data()))
                {
                    for (const schema::Field::Reader& unionField : fields)
                    {
                        StringView unionFieldName = unionField.GetName();
                        StringView unionFieldDesc;
                        GetNameAndDescription(unionFieldName, unionFieldDesc, unionField.GetAttributes());

                        const bool isSelected = unionField.GetUnionTag() == activeField.GetUnionTag();
                        if (ImGui::Selectable(unionFieldName.Data(), isSelected))
                        {
                            // TODO: set union tag & clear field values
                        }

                        if (!unionFieldDesc.IsEmpty() && ImGui::IsItemHovered())
                            ImGui::SetTooltip("%s", unionFieldDesc.Data());

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
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
                schema::StructVisitor::VisitField(data, activeField, *info);
                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        void VisitValue(bool value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            const schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
            const ImGuiInputTextFlags flags = readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

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

        void VisitValue(int8_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            int8_t v = value;
            if (InputScalar(v, ImGuiDataType_S8))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt8(v);
            }
        }

        void VisitValue(int16_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            int16_t v = value;
            if (InputScalar(v, ImGuiDataType_S16))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt16(v);
            }
        }

        void VisitValue(int32_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            int32_t v = value;
            if (InputScalar(v, ImGuiDataType_S32))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt32(v);
            }
        }

        void VisitValue(int64_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            int64_t v = value;
            if (InputScalar(v, ImGuiDataType_S64))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetInt64(v);
            }
        }

        void VisitValue(uint8_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            uint8_t v = value;
            if (InputScalar(v, ImGuiDataType_U8))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint8(v);
            }
        }

        void VisitValue(uint16_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            uint16_t v = value;
            if (InputScalar(v, ImGuiDataType_U16))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint16(v);
            }
        }

        void VisitValue(uint32_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            uint32_t v = value;
            if (InputScalar(v, ImGuiDataType_U32))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint32(v);
            }
        }

        void VisitValue(uint64_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            uint64_t v = value;
            if (InputScalar(v, ImGuiDataType_U64))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetUint64(v);
            }
        }

        void VisitValue(float value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            float v = value;
            if (InputScalar(v, ImGuiDataType_Float))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetFloat32(v);
            }
        }

        void VisitValue(double value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(type, scope);

            double v = value;
            if (InputScalar(v, ImGuiDataType_Double))
            {
                AssetEditAction& action = m_edit.EmplaceAction(AssetEditAction::Kind::SetValue);
                action.value.value.GetData().SetFloat64(v);
            }
        }

        void VisitValue(schema::Blob::Reader value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            // TODO: What editor do we show for blobs? None?
            HE_UNUSED(value, type, scope);
            ImGui::TextUnformatted("Binary Data");
        }

        void VisitValue(schema::String::Reader value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            const schema::Field::Reader field = m_edit.path.Back().field;

            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
            HE_UNUSED(type, scope);

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

        void VisitValue(EnumValueTag, uint16_t value, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            struct EnumValue { const char* name; uint16_t value; };
            static Vector<EnumValue> values;

            const schema::Type::Data::Enum::Reader enumType = type.GetData().GetEnum();
            const schema::DeclInfo* info = schema::FindDependency(scope, enumType.GetId());

            if (!HE_VERIFY(info,
                HE_MSG("Invalid schema. No dependency of the parent scope matches the enum's type id."),
                HE_KV(enum_id, enumType.GetId()),
                HE_KV(scope_id, scope.id),
                HE_KV(scope_name, GetSchema(scope).GetName())))
            {
                return;
            }

            const schema::Declaration::Reader decl = schema::GetSchema(*info);
            const schema::Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

            const char* valueName = "Select...";
            for (const schema::Enumerator::Reader e : enumDecl.GetEnumerators())
            {
                if (e.GetOrdinal() == value)
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

                    const bool isSelected = e.GetOrdinal() == value;
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

        void VisitAnyPointer(schema::PointerReader ptr, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            // TODO
            HE_UNUSED(ptr, type, scope);
            ImGui::TextUnformatted("Any Pointer");
        }

        void VisitAnyStruct(schema::PointerReader ptr, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            // TODO
            HE_UNUSED(ptr, type, scope);
            ImGui::TextUnformatted("Any Struct");
            //VisitStruct(ptr.TryGetStruct(), assets::schema::Texture2D::DeclInfo);
        }

        void VisitAnyList(schema::PointerReader ptr, schema::Type::Reader type, const schema::DeclInfo& scope) override
        {
            // TODO
            HE_UNUSED(ptr, type, scope);
            ImGui::TextUnformatted("Any List");
        }

        bool ShouldVisitNormalField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(data, scope);
            const bool hidden = schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

        bool ShouldVisitGroupField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(data, scope);
            const bool hidden = schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

        bool ShouldVisitUnionField(schema::StructReader data, schema::Field::Reader field, const schema::DeclInfo& scope) override
        {
            HE_UNUSED(data, scope);
            const bool hidden = schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

    private:
        void VisitDefaultValue(schema::Type::Reader type, const schema::DeclInfo& scope)
        {
            const schema::Type::Data::Reader typeData = type.GetData();
            const schema::Type::Data::UnionTag typeTag = typeData.GetUnionTag();

            switch (typeTag)
            {
                case schema::Type::Data::UnionTag::Void: break;
                case schema::Type::Data::UnionTag::Bool: VisitValue(false, type, scope); break;
                case schema::Type::Data::UnionTag::Int8: VisitValue(static_cast<int8_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Int16: VisitValue(static_cast<int16_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Int32: VisitValue(static_cast<int32_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Int64: VisitValue(static_cast<int64_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Uint8: VisitValue(static_cast<uint8_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Uint16: VisitValue(static_cast<uint16_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Uint32: VisitValue(static_cast<uint32_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Uint64: VisitValue(static_cast<uint64_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Float32: VisitValue(static_cast<float>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Float64: VisitValue(static_cast<double>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Blob: VisitValue(schema::Blob::Reader{}, type, scope); break;
                case schema::Type::Data::UnionTag::String: VisitValue(schema::String::Reader{}, type, scope); break;
                case schema::Type::Data::UnionTag::Enum: VisitValue(EnumValueTag{}, static_cast<uint16_t>(0), type, scope); break;
                case schema::Type::Data::UnionTag::Array:
                case schema::Type::Data::UnionTag::List:
                case schema::Type::Data::UnionTag::Struct:
                    HE_ASSERT(!typeData.IsStruct(), HE_MSG("No {} support in default value visit, yet.", typeTag));
                    break;
                case schema::Type::Data::UnionTag::AnyPointer:
                case schema::Type::Data::UnionTag::AnyStruct:
                case schema::Type::Data::UnionTag::AnyList:
                case schema::Type::Data::UnionTag::Interface:
                case schema::Type::Data::UnionTag::Parameter:
                    HE_VERIFY(false, HE_MSG("{} types cannot have default values.", typeTag));
                    break;
            }
        }

        void RevertActionButton(schema::StructReader data)
        {
            const schema::Field::Reader field = m_edit.path.Back().field;

            if (!field.GetMeta().IsNormal())
                return;

            const schema::Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
            const schema::Type::Reader type = norm.GetType();

            const bool readOnly = schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
            const bool isPointer = schema::IsPointer(type);
            const bool hasValue = isPointer ? data.HasPointerField(norm.GetIndex()) : data.HasDataField(norm.GetIndex());

            if (!readOnly && hasValue)
            {
                ImGui::Button(ICON_MDI_UNDO_VARIANT);
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
            const ImGuiInputTextFlags flags = readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

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

    private:
        AssetEdit& m_edit;
    };

    void PropertyGrid(schema::StructReader data, const schema::DeclInfo& declInfo, AssetEdit& edit)
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
                visitor.Visit(data, declInfo);

                EndPropertyGridTable();
            }

            EndPropertyGrid();
        }
    }
}

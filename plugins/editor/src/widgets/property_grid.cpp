// Copyright Chad Engler

#include "property_grid.h"

#include "input_text.h"
#include "fonts/icons_material_design.h"
#include "framework/schema_edit.h"
#include "services/type_edit_ui_service.h"

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

    class PropertyGridStructVisitor : public he::schema::DynamicStructVisitor::Reader
    {
    public:
        using SuperType = he::schema::DynamicStructVisitor::Reader;

    public:
        PropertyGridStructVisitor(SchemaEdit& edit, TypeEditUIService& service)
            : m_edit(edit)
            , m_editUIService(service)
        {}

        const SchemaEdit& Edit() const { return m_edit; }
        SchemaEdit& Edit() { return m_edit; }

    private:
        void VisitStruct(const he::schema::DynamicStruct::Reader& data) override
        {
            // For the root struct just visit it normally
            if (m_edit.path.IsEmpty())
            {
                SuperType::VisitStruct(data);
                return;
            }

            const he::schema::Field::Reader field = m_edit.path.Back().field;
            ShowEditorRow(data, field);
        }

        void VisitField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            m_edit.path.PushBack({ field });
            SuperType::VisitField(data, field);
            m_edit.path.PopBack();
        }

        void VisitNormalField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            ShowEditorRow(data, field);
        }

        void VisitGroupField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            ShowEditorRow(data, field);
        }

        void VisitUnionField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            ShowEditorRow(data, field);
        }

        void VisitValue(const he::schema::DynamicValue::Reader& value) override
        {
            const he::schema::Field::Reader field = m_edit.path.Back().field;

            switch (value.GetKind())
            {
                case he::schema::DynamicValue::Kind::Unknown: ImGui::TextUnformatted("Unknown DynamicValue type."); break; //TODO: Log once? VERIFY once?
                case he::schema::DynamicValue::Kind::Void: ShowValueEditor(value.As<he::schema::Void>()); break;
                case he::schema::DynamicValue::Kind::Bool: ShowValueEditor(value.As<bool>()); break;
                case he::schema::DynamicValue::Kind::Blob: ShowValueEditor(value.As<he::schema::Blob>()); break;
                case he::schema::DynamicValue::Kind::String: ShowValueEditor(value.As<he::schema::String>()); break;
                case he::schema::DynamicValue::Kind::Array: ShowValueEditor(value.As<he::schema::DynamicArray>()); break;
                case he::schema::DynamicValue::Kind::List: ShowValueEditor(value.As<he::schema::DynamicList>()); break;
                case he::schema::DynamicValue::Kind::Enum: ShowValueEditor(value.As<he::schema::DynamicEnum>()); break;
                case he::schema::DynamicValue::Kind::AnyPointer: ShowValueEditor(value.As<he::schema::AnyPointer>()); break;
                case he::schema::DynamicValue::Kind::Int: ShowValueEditor(value.As<int64_t>()); break;
                case he::schema::DynamicValue::Kind::Uint: ShowValueEditor(value.As<uint64_t>()); break;
                case he::schema::DynamicValue::Kind::Float: ShowValueEditor(value.As<double>()); break;
                case he::schema::DynamicValue::Kind::Struct:
                {
                    he::schema::DynamicStruct::Reader builder = value.As<he::schema::DynamicStruct>();
                    SuperType::VisitStruct(builder);
                    break;
                }
            }
        }

        bool ShouldVisitNormalField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            HE_UNUSED(data);
            const bool hidden = he::schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

        bool ShouldVisitGroupField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            HE_UNUSED(data);
            const bool hidden = he::schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

        bool ShouldVisitUnionField(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field) override
        {
            HE_UNUSED(data);
            const bool hidden = he::schema::HasAttribute<assets::schema::Display::Hidden>(field.GetAttributes());
            return !hidden;
        }

    private:
        void RevertActionButton(const he::schema::DynamicStruct::Reader& data)
        {
            const he::schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            if (!readOnly && data.Has(field))
            {
                if (ImGui::Button(ICON_MDI_UNDO_VARIANT))
                {
                    m_edit.EmplaceAction(SchemaEditAction::Kind::ClearValue);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Reset to default");
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

        void GetNameAndDescription(StringView& name, StringView& desc, he::schema::List<he::schema::Attribute>::Reader attributes)
        {
            const he::schema::Attribute::Reader displayNameAttr = he::schema::FindAttribute<assets::schema::Display::Name>(attributes);
            const he::schema::Attribute::Reader descriptionAttr = he::schema::FindAttribute<assets::schema::Display::Description>(attributes);

            if (displayNameAttr.IsValid())
                name = displayNameAttr.GetValue().GetData().GetString();
            else
                name = FormatName(name);

            if (descriptionAttr.IsValid())
                desc = descriptionAttr.GetValue().GetData().GetString();
        }

        template <typename T> bool ReadScalarRange(const he::schema::Value::Reader value, T& min, T& max);

        template <> bool ReadScalarRange(const he::schema::Value::Reader value, int64_t& min, int64_t& max)
        {
            if (!value.IsValid() || !value.GetData().IsStruct())
                return false;

            const assets::schema::ScalarRange::Reader scalarRange = value.GetData().GetStruct().TryGetStruct<assets::schema::ScalarRange>();

            if (!scalarRange.IsValid() || !scalarRange.GetData().IsInt())
                return false;

            const assets::schema::ScalarRange::Data::Int::Reader range = scalarRange.GetData().GetInt();
            min = range.GetMin();
            max = range.GetMax();
            return true;
        }

        template <> bool ReadScalarRange(const he::schema::Value::Reader value, uint64_t& min, uint64_t& max)
        {
            if (!value.IsValid() || !value.GetData().IsStruct())
                return false;

            const assets::schema::ScalarRange::Reader scalarRange = value.GetData().GetStruct().TryGetStruct<assets::schema::ScalarRange>();

            if (!scalarRange.IsValid() || !scalarRange.GetData().IsUint())
                return false;

            const assets::schema::ScalarRange::Data::Uint::Reader range = scalarRange.GetData().GetUint();
            min = range.GetMin();
            max = range.GetMax();
            return true;
        }

        template <> bool ReadScalarRange(const he::schema::Value::Reader value, double& min, double& max)
        {
            if (!value.IsValid() || !value.GetData().IsStruct())
                return false;

            const assets::schema::ScalarRange::Reader scalarRange = value.GetData().GetStruct().TryGetStruct<assets::schema::ScalarRange>();

            if (!scalarRange.IsValid() || !scalarRange.GetData().IsFloat())
                return false;

            const assets::schema::ScalarRange::Data::Float::Reader range = scalarRange.GetData().GetFloat();
            min = range.GetMin();
            max = range.GetMax();
            return true;
        }

        void ShowValueEditor(he::schema::Void value)
        {
            HE_UNUSED(value);
            ImGui::TextUnformatted("void");
        }

        void ShowValueEditor(bool value)
        {
            const he::schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            bool v = value;

            ImGui::BeginDisabled(readOnly);
            if (ImGui::Checkbox("##bool-value", &v))
            {
                SchemaEditAction& action = m_edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
                action.value = v;
            }
            ImGui::EndDisabled();
            if (readOnly && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("This field is read-only.");
        }

        template <Arithmetic T> struct _ImGuiDataTypeForType;
        template <> struct _ImGuiDataTypeForType<int64_t> { static constexpr ImGuiDataType Value = ImGuiDataType_S64; };
        template <> struct _ImGuiDataTypeForType<uint64_t> { static constexpr ImGuiDataType Value = ImGuiDataType_U64; };
        template <> struct _ImGuiDataTypeForType<double> { static constexpr ImGuiDataType Value = ImGuiDataType_Double; };

        template <Arithmetic T>
        void ShowValueEditor(T value)
        {
            constexpr ImGuiDataType DataType = _ImGuiDataTypeForType<T>::Value;

            const he::schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
            const he::schema::Attribute::Reader slider = he::schema::FindAttribute<assets::schema::Display::Slider>(field.GetAttributes());
            const he::schema::Attribute::Reader clamp = he::schema::FindAttribute<assets::schema::Display::Clamp>(field.GetAttributes());

            bool changed = false;

            ImGui::BeginDisabled(readOnly);
            ImGui::PushItemWidth(-1.0f);

            T v = value;
            if (slider.IsValid())
            {
                T min = std::numeric_limits<T>::lowest();
                T max = std::numeric_limits<T>::max();
                ReadScalarRange(slider.GetValue(), min, max);

                if (ImGui::SliderScalar("##scalar-slider", DataType, &v, &min, &max, nullptr, ImGuiSliderFlags_AlwaysClamp))
                {
                    changed = true;
                }
            }
            else
            {
                if (ImGui::InputScalar("##scalar-value", DataType, &v, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    changed = true;
                }
            }

            ImGui::PopItemWidth();
            ImGui::EndDisabled();

            if (readOnly && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("This field is read-only.");


            if (clamp.IsValid())
            {
                T min = 0;
                T max = 0;
                if (ReadScalarRange(clamp.GetValue(), min, max))
                {
                    v = Clamp(v, min, max);
                }
            }

            if (changed)
            {
                SchemaEditAction& action = m_edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
                action.value = v;
            }
        }

        void ShowValueEditor(he::schema::Blob::Reader value)
        {
            HE_UNUSED(value);
            ImGui::TextUnformatted("<binary data>");
        }

        void ShowValueEditor(he::schema::String::Reader value)
        {
            const he::schema::Field::Reader field = m_edit.path.Back().field;

            const bool readOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            static String v;
            v = value;

            const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue
                | ImGuiInputTextFlags_NoUndoRedo
                | (readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);

            ImGui::BeginDisabled(readOnly);
            ImGui::PushItemWidth(-1.0f);

            if (InputText("##string-value", v, flags))
            {
                SchemaEditAction& action = m_edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
                action.value = m_edit.m_builder.AddString(v);
            }

            ImGui::PopItemWidth();
            ImGui::EndDisabled();

            if (readOnly && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("This field is read-only.");
        }

        void ShowValueEditor(const he::schema::DynamicArray::Reader& value)
        {
            HE_ASSERT(m_edit.path.Back().field.GetMeta().GetNormal().GetType().GetData().IsArray());

            for (uint16_t i = 0; i < value.Size(); ++i)
            {
                m_edit.path.PushBack({ m_edit.path.Back().field, i });
                ShowEditorRow(value, i);
                m_edit.path.PopBack();
            }
        }

        void ShowValueEditor(const he::schema::DynamicList::Reader& value)
        {
            HE_ASSERT(m_edit.path.Back().field.GetMeta().GetNormal().GetType().GetData().IsList());

            for (uint32_t i = 0; i < value.Size(); ++i)
            {
                m_edit.path.PushBack({ m_edit.path.Back().field, i });
                ShowEditorRow(value, i);
                m_edit.path.PopBack();
            }
        }

        void ShowValueEditor(he::schema::DynamicEnum value)
        {
            const he::schema::Declaration::Data::Enum::Reader enumDecl = value.EnumSchema();

            const char* valueName = "Select...";
            for (const he::schema::Enumerator::Reader e : enumDecl.GetEnumerators())
            {
                if (e.GetOrdinal() == value.Value())
                {
                    valueName = e.GetName().Data();
                    break;
                }
            }

            const he::schema::Field::Reader field = m_edit.path.Back().field;
            const bool readOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());

            ImGui::BeginDisabled(readOnly);
            ImGui::PushItemWidth(-1.0f);

            if (ImGui::BeginCombo("##enum-value", valueName))
            {
                for (const he::schema::Enumerator::Reader e : enumDecl.GetEnumerators())
                {
                    StringView name = e.GetName();
                    StringView desc;
                    GetNameAndDescription(name, desc, e.GetAttributes());

                    const bool isSelected = e.GetOrdinal() == value.Value();
                    if (ImGui::Selectable(name.Data(), isSelected))
                    {
                        SchemaEditAction& action = m_edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
                        action.value = he::schema::DynamicEnum(value.Decl(), e.GetOrdinal());
                    }

                    if (!desc.IsEmpty() && ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", desc.Data());

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            ImGui::PopItemWidth();
            ImGui::EndDisabled();
        }

        void ShowValueEditor(he::schema::AnyPointer::Reader ptr)
        {
            HE_UNUSED(ptr);
            ImGui::TextUnformatted("<any pointer>");
        }

        template <AnyOf<he::schema::DynamicArray::Reader, he::schema::DynamicList::Reader> T, AnyOf<uint16_t, uint32_t> U>
        void ShowEditorRow(const T& data, U index)
        {
            String name;
            fmt::format_to(Appender(name), "[{}]", index);

            const he::schema::Type::Reader elementType = [&]()
            {
                if constexpr (std::is_same_v<T, he::schema::DynamicArray::Reader>)
                    return data.ArrayType().GetElementType();
                else
                    return data.ListType().GetElementType();
            }();
            const he::schema::Type::Data::Reader typeData = elementType.GetData();

            const TypeEditUIService::Editor* customValueEditor = nullptr;
            if (typeData.IsStruct())
            {
                customValueEditor = m_editUIService.FindEditor(typeData.GetStruct().GetId());
            }

            bool open = false;
            if (BeginPropertyGridRow())
            {
                // Name column
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                const bool isSequenceValue = typeData.IsArray() || typeData.IsList();
                const bool isStructValue = typeData.IsStruct();
                const bool hasCustomInlineEditor = customValueEditor && customValueEditor->isInline;
                const bool hasCustomFullEditor = customValueEditor && !customValueEditor->isInline;
                const bool isExpandable = isSequenceValue || hasCustomFullEditor || (isStructValue && !hasCustomInlineEditor);

                if (isExpandable)
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

                // Value column
                ImGui::TableNextColumn();
                if (customValueEditor)
                {
                    if (customValueEditor->isInline)
                    {
                        he::schema::DynamicValue::Reader value = data.Get(index);
                        TypeEditUIService::Context ctx{ data, {}, index, m_edit, *this };
                        customValueEditor->func(value, ctx);
                    }
                }
                else if (!isExpandable)
                {
                    he::schema::DynamicValue::Reader value = data.Get(index);
                    VisitValue(value);
                }

                // Actions column
                ImGui::TableNextColumn();
                const he::schema::Field::Reader field = m_edit.path.Back().field;
                const bool readOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
                if (!readOnly)
                {
                    if (typeData.IsList())
                    {
                        if (ImGui::Button(ICON_MDI_PLUS))
                        {
                            m_edit.EmplaceAction(SchemaEditAction::Kind::AddListItem);
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Add new element");
                    }

                    if (IsPointer(elementType) && data.Has(index))
                    {
                        if (ImGui::Button(ICON_MDI_UNDO_VARIANT))
                        {
                            m_edit.EmplaceAction(SchemaEditAction::Kind::ClearValue);
                        }
                    }
                }

                EndPropertyGridRow();
            }

            if (open)
            {
                const ImVec2& padding = ImGui::GetStyle().FramePadding;
                ImGui::Indent(padding.x);
                ImGui::PushID(name.Data());

                if (customValueEditor)
                {
                    if (!customValueEditor->isInline)
                    {
                        he::schema::DynamicValue::Reader value = data.Get(index);
                        TypeEditUIService::Context ctx{ data, {}, index, m_edit, *this };
                        customValueEditor->func(value, ctx);
                    }
                }
                else
                {
                    he::schema::DynamicValue::Reader value = data.Get(index);
                    VisitValue(value);
                }

                ImGui::PopID();
                ImGui::Unindent();
            }
        }

        void ShowEditorRow(const he::schema::DynamicStruct::Reader& data, he::schema::Field::Reader field)
        {
            StringView name = field.GetName();
            StringView desc;
            GetNameAndDescription(name, desc, field.GetAttributes());

            he::schema::Type::Data::Reader typeData;
            if (field.GetMeta().IsNormal())
            {
                const he::schema::Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                const he::schema::Type::Reader type = norm.GetType();
                typeData = type.GetData();
            }

            const TypeEditUIService::Editor* customValueEditor = m_editUIService.FindEditor(field);

            bool open = false;
            if (BeginPropertyGridRow())
            {
                // Name column
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                const bool isReadOnly = he::schema::HasAttribute<assets::schema::Display::ReadOnly>(field.GetAttributes());
                const bool isModified = !isReadOnly && data.Has(field);

                const bool isSequenceValue = typeData.IsValid() && (typeData.IsArray() || typeData.IsList());
                const bool isStructValue = (typeData.IsValid() && typeData.IsStruct()) || (field.IsValid() && field.GetMeta().IsGroup());
                const bool hasCustomInlineEditor = customValueEditor && customValueEditor->isInline;
                const bool hasCustomFullEditor = customValueEditor && !customValueEditor->isInline;
                const bool isExpandable = isSequenceValue || hasCustomFullEditor || (isStructValue && !hasCustomInlineEditor);

                if (isModified)
                {
                    // TODO: Move this color into a theme palette somewhere
                    const ImVec4 color(166 / 255.0f, 255 / 255.0f, 161 / 255.0f, 1.0f); // Mint Green
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                }

                if (isExpandable)
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

                if (isModified)
                {
                    ImGui::PopStyleColor();
                }

                if (!desc.IsEmpty() && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", desc.Data());

                // Value column
                ImGui::TableNextColumn();

                // custom inline editor for this field
                if (customValueEditor && customValueEditor->isInline)
                {
                    he::schema::DynamicValue::Reader value = data.Get(field);
                    TypeEditUIService::Context ctx{ data, field, 0, m_edit, *this };
                    customValueEditor->func(value, ctx);
                }
                // union field
                else if (field.GetMeta().IsUnion())
                {
                    const he::schema::DynamicStruct::Reader& unionStruct = data.Get(field).As<he::schema::DynamicStruct>();
                    const he::schema::Field::Reader activeField = unionStruct.ActiveUnionField();
                    StringView activeFieldName = activeField.GetName();
                    StringView activeFieldDesc;
                    GetNameAndDescription(activeFieldName, activeFieldDesc, activeField.GetAttributes());

                    if (ImGui::BeginCombo("##union-field-type", activeFieldName.Data()))
                    {
                        for (const he::schema::Field::Reader& unionField : unionStruct.StructSchema().GetFields())
                        {
                            StringView unionFieldName = unionField.GetName();
                            StringView unionFieldDesc;
                            GetNameAndDescription(unionFieldName, unionFieldDesc, unionField.GetAttributes());

                            const bool isSelected = unionField.GetUnionTag() == activeField.GetUnionTag();
                            if (ImGui::Selectable(unionFieldName.Data(), isSelected))
                            {
                                SchemaEditAction& action = m_edit.EmplaceAction(SchemaEditAction::Kind::InitValue);
                                action.path.PushBack({ unionField });
                            }

                            if (!unionFieldDesc.IsEmpty() && ImGui::IsItemHovered())
                                ImGui::SetTooltip("%s", unionFieldDesc.Data());

                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }

                    if (!activeFieldDesc.IsEmpty() && ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", activeFieldDesc.Data());
                }
                // normal field
                else if (!isExpandable && !customValueEditor && typeData.IsValid())
                {
                    he::schema::DynamicValue::Reader value = data.Get(field);
                    VisitValue(value);
                }

                // Actions column
                ImGui::TableNextColumn();
                if (typeData.IsValid() && typeData.IsList())
                {
                    if (ImGui::Button(ICON_MDI_PLUS))
                    {
                        m_edit.EmplaceAction(SchemaEditAction::Kind::AddListItem);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Add new element");
                }
                RevertActionButton(data);

                EndPropertyGridRow();
            }

            if (open)
            {
                const ImVec2& padding = ImGui::GetStyle().FramePadding;
                ImGui::Indent(padding.x);
                ImGui::PushID(name.Data());

                if (customValueEditor)
                {
                    HE_ASSERT(!customValueEditor->isInline);
                    he::schema::DynamicValue::Reader value = data.Get(field);
                    TypeEditUIService::Context ctx{ data, field, 0, m_edit, *this };
                    customValueEditor->func(value, ctx);
                }
                else
                {
                    he::schema::DynamicValue::Reader value = data.Get(field);
                    VisitValue(value);
                }

                ImGui::PopID();
                ImGui::Unindent();
            }
        }

    private:
        SchemaEdit& m_edit;
        TypeEditUIService& m_editUIService;
    };

    void PropertyGrid(const he::schema::DynamicStruct::Reader& data, TypeEditUIService& service, SchemaEdit& edit)
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
                PropertyGridStructVisitor visitor(edit, service);
                visitor.Visit(data);

                EndPropertyGridTable();
            }

            EndPropertyGrid();
        }
    }
}

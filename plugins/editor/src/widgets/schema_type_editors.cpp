// Copyright Chad Engler

#include "he/editor/widgets/schema_type_editors.h"

#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/schema/types.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    struct FieldName_X { static constexpr StringView Name = "x"; };
    struct FieldName_Y { static constexpr StringView Name = "y"; };
    struct FieldName_Z { static constexpr StringView Name = "z"; };
    struct FieldName_W { static constexpr StringView Name = "w"; };

    constexpr ImU32 ColorX = IM_COL32(168, 46, 2, 255);
    constexpr ImU32 ColorY = IM_COL32(112, 162, 22, 255);
    constexpr ImU32 ColorZ = IM_COL32(51, 122, 210, 255);
    constexpr ImU32 ColorW = IM_COL32(51, 122, 210, 255);

    template <typename T, typename F>
    const schema::Field::Reader GetField()
    {
        static schema::Field::Reader s_field{};
        if (s_field.IsValid())
            return s_field;

        const schema::Declaration::Reader decl = GetSchema(T::DeclInfo);
        s_field = FindFieldByName(F::Name, decl.GetData().GetStruct());
        return s_field;
    }

    static bool VecFloatEditor(float& value, const char* name, ImU32 color)
    {
        const ImVec2 startPos = ImGui::GetCursorScreenPos();
        const float dpiScale = ImGui::GetWindowDpiScale();

        ImGui::PushItemWidth(65.0f * dpiScale);
        ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
        const bool changed = ImGui::InputFloat("##vec-float", &value, 0, 0, "%.5f", ImGuiInputTextFlags_EnterReturnsTrue);
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        ImGui::PopItemWidth();

        const ImVec2 offset{ 4.0f * dpiScale, 3.0f * dpiScale };
        const ImVec2 size{ 4.0f * dpiScale, ImGui::GetFrameHeight() - (offset.y * 2.0f) };

        const ImRect colorRect
        {
            startPos.x + offset.x,
            startPos.y + offset.y,
            startPos.x + offset.x + size.x,
            startPos.y + offset.y + size.y,
        };
        ImGui::GetWindowDrawList()->AddRectFilled(colorRect.Min, colorRect.Max, color);

        if (hovered)
        {
            ImGui::SetTooltip("%s = %f", name, value);
        }

        return changed;
    }

    void Vec2fEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx)
    {
        const schema::Vec2f::Reader vec = value.As<schema::DynamicStruct>().As<schema::Vec2f>();

        float x = vec.GetX();
        if (VecFloatEditor(x, "X", ColorX))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec2f, FieldName_X>() });
            action.value = x;
        }

        float y = vec.GetY();
        ImGui::SameLine();
        if (VecFloatEditor(y, "Y", ColorY))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec2f, FieldName_Y>() });
            action.value = y;
        }
    }

    void Vec3fEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx)
    {
        const schema::Vec3f::Reader vec = value.As<schema::DynamicStruct>().As<schema::Vec3f>();

        float x = vec.GetX();
        if (VecFloatEditor(x, "X", ColorX))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec3f, FieldName_X>() });
            action.value = x;
        }

        float y = vec.GetY();
        ImGui::SameLine();
        if (VecFloatEditor(y, "Y", ColorY))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec3f, FieldName_Y>() });
            action.value = y;
        }

        float z = vec.GetZ();
        ImGui::SameLine();
        if (VecFloatEditor(z, "Z", ColorZ))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec3f, FieldName_Z>() });
            action.value = z;
        }
    }

    void Vec4fEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx)
    {
        const schema::Vec4f::Reader vec = value.As<schema::DynamicStruct>().As<schema::Vec4f>();

        float x = vec.GetX();
        if (VecFloatEditor(x, "X", ColorX))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec4f, FieldName_X>() });
            action.value = x;
        }

        float y = vec.GetY();
        ImGui::SameLine();
        if (VecFloatEditor(y, "Y", ColorY))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec4f, FieldName_Y>() });
            action.value = y;
        }

        float z = vec.GetZ();
        ImGui::SameLine();
        if (VecFloatEditor(z, "Z", ColorZ))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec4f, FieldName_Z>() });
            action.value = z;
        }

        float w = vec.GetW();
        ImGui::SameLine();
        if (VecFloatEditor(w, "W", ColorW))
        {
            SchemaEditAction& action = ctx.edit.EmplaceAction(SchemaEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec4f, FieldName_W>() });
            action.value = w;
        }
    }

    void UuidEditor(const void*, const schema::DynamicValue::Reader& value, TypeEditUIService::Context& ctx)
    {
        HE_UNUSED(ctx);

        const schema::Uuid::Reader uuid = value.As<schema::DynamicStruct>().As<schema::Uuid>();
        const uint8_t* b = uuid.GetValue().Data();

        static String s_buf;
        s_buf.Clear();
        FormatTo(s_buf, "{:02x}-{:02x}-{:02x}-{:02x}-{:02x}",
            FmtJoin(b + 0, b + 4, ""),
            FmtJoin(b + 4, b + 6, ""),
            FmtJoin(b + 6, b + 8, ""),
            FmtJoin(b + 8, b + 10, ""),
            FmtJoin(b + 10, b + 16, ""));

        ImGui::BeginDisabled(true);
        ImGui::PushItemWidth(-1.0f);
        ImGui::InputText("##uuid", s_buf.Data(), s_buf.Capacity() + 1, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("This field is read-only.");
    }
}

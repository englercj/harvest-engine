// Copyright Chad Engler

#include "he/assets/asset_editors.h"

#include "he/assets/asset_type_registry.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/appender.h"
#include "he/core/module_registry.h"

#include "imgui.h"

namespace he::assets
{
    extern Module* g_assetEditorModule;

    struct FieldName_X { static constexpr StringView Name = "x"; };
    struct FieldName_Y { static constexpr StringView Name = "y"; };
    struct FieldName_Z { static constexpr StringView Name = "z"; };

    template <typename T, typename F>
    const he::schema::Field::Reader GetField()
    {
        static he::schema::Field::Reader s_field{};
        if (s_field.IsValid())
            return s_field;

        const he::schema::Declaration::Reader decl = GetSchema(T::DeclInfo);
        s_field = FindFieldByName(F::Name, decl.GetData().GetStruct());
        return s_field;
    }

    void Vec2fEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx)
    {
        const schema::Vec2f::Reader vec = value.As<he::schema::DynamicStruct>().As<schema::Vec2f>();

        float x = vec.GetX();
        ImGui::TextUnformatted("X =");
        ImGui::SameLine();
        ImGui::PushItemWidth(96.0f);
        if (ImGui::InputFloat("##vec2f-x", &x))
        {
            editor::AssetEditAction& action = ctx.edit.EmplaceAction(editor::AssetEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec2f, FieldName_X>() });
            action.value = x;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        float y = vec.GetY();
        ImGui::TextUnformatted("Y =");
        ImGui::SameLine();
        ImGui::PushItemWidth(96.0f);
        if (ImGui::InputFloat("##vec2f-y", &y))
        {
            editor::AssetEditAction& action = ctx.edit.EmplaceAction(editor::AssetEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec2f, FieldName_Y>() });
            action.value = y;
        }
        ImGui::PopItemWidth();
    }

    void Vec3fEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx)
    {
        const schema::Vec3f::Reader vec = value.As<he::schema::DynamicStruct>().As<schema::Vec3f>();

        float x = vec.GetX();
        ImGui::TextUnformatted("X =");
        ImGui::SameLine();
        ImGui::PushItemWidth(96.0f);
        if (ImGui::InputFloat("##vec3f-x", &x))
        {
            editor::AssetEditAction& action = ctx.edit.EmplaceAction(editor::AssetEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec3f, FieldName_X>() });
            action.value = x;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        float y = vec.GetY();
        ImGui::TextUnformatted("Y =");
        ImGui::SameLine();
        ImGui::PushItemWidth(96.0f);
        if (ImGui::InputFloat("##vec3f-y", &y))
        {
            editor::AssetEditAction& action = ctx.edit.EmplaceAction(editor::AssetEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec3f, FieldName_Y>() });
            action.value = y;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        float z = vec.GetZ();
        ImGui::TextUnformatted("Z =");
        ImGui::SameLine();
        ImGui::PushItemWidth(96.0f);
        if (ImGui::InputFloat("##vec3f-z", &y))
        {
            editor::AssetEditAction& action = ctx.edit.EmplaceAction(editor::AssetEditAction::Kind::SetValue);
            action.path.PushBack({ GetField<schema::Vec3f, FieldName_Z>() });
            action.value = z;
        }
        ImGui::PopItemWidth();
    }

    void AssetUuidFieldEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx)
    {
        HE_UNUSED(ctx);

        const he::schema::Uuid::Reader uuid = value.As<he::schema::DynamicStruct>().As<he::schema::Uuid>();
        const AssetUuid assetUuid(uuid);

        static String s_buf;
        s_buf.Clear();
        fmt::format_to(Appender(s_buf), "{}", assetUuid);

        ImGui::BeginDisabled(true);
        ImGui::PushItemWidth(-1.0f);
        ImGui::InputText("##asset-uuid", s_buf.Data(), s_buf.Capacity() + 1, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("This field is read-only.");
    }

    void AssetDataFieldEditor(const void*, const he::schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx)
    {
        ModuleRegistry& registry = g_assetEditorModule->Registry();
        AssetTypeRegistry& types = registry.GetApi<AssetTypeRegistry>();

        // This field must be a pointer, it can be null though.
        if (!HE_VERIFY(value.GetKind() == he::schema::DynamicValue::Kind::AnyPointer))
            return;

        // If the type is empty we have no way to edit this data.
        const he::schema::String::Reader typeName = ctx.data.As<he::schema::DynamicStruct>().Get("type").As<he::schema::String>();
        if (!HE_VERIFY(typeName.IsValid() && !typeName.IsEmpty()))
            return;

        const AssetTypeId typeId(typeName);
        const AssetTypeRegistry::Entry* assetType = types.FindAssetType(typeId);

        // If the asset type is unknown we have no way to edit this data.
        if (!HE_VERIFY(assetType))
        {
            ImGui::Text("Unknown asset type: %s", typeName.Data());
            return;
        }

        // Visit the asset data using the decl info from the asset type.
        const he::schema::AnyPointer::Reader assetDataPtr = value.As<he::schema::AnyPointer>();
        const he::schema::StructReader assetDataStruct = assetDataPtr.TryGetStruct();
        const he::schema::DynamicStruct::Reader assetData(*assetType->declInfo, assetDataStruct);
        ctx.edit.path.Back().info = assetType->declInfo;
        ctx.visitor.VisitValue(assetData);
    }
}

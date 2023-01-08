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

    void AssetUuidFieldEditor(const void*, const schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx)
    {
        HE_UNUSED(ctx);

        const schema::Uuid::Reader uuid = value.As<schema::DynamicStruct>().As<schema::Uuid>();
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

    void AssetDataFieldEditor(const void*, const schema::DynamicValue::Reader& value, editor::TypeEditUIService::Context& ctx)
    {
        ModuleRegistry& registry = g_assetEditorModule->Registry();
        AssetTypeRegistry& types = registry.GetApi<AssetTypeRegistry>();

        // This field must be a pointer, it can be null though.
        if (!HE_VERIFY(value.GetKind() == schema::DynamicValue::Kind::AnyPointer))
            return;

        // If the type is empty we have no way to edit this data.
        const schema::String::Reader typeName = ctx.data.As<schema::DynamicStruct>().Get("type").As<schema::String>();
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
        const schema::AnyPointer::Reader assetDataPtr = value.As<schema::AnyPointer>();
        const schema::StructReader assetDataStruct = assetDataPtr.TryGetStruct();
        const schema::DynamicStruct::Reader assetData(*assetType->declInfo, assetDataStruct);
        ctx.edit.path.Back().info = assetType->declInfo;
        ctx.visitor.VisitValue(assetData);
    }
}

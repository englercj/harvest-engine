// Copyright Chad Engler

#include "he/editor/dialogs/import_asset_dialog.h"

#include "he/assets/asset_importer.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/key_value.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/utils.h"
#include "he/editor/framework/imgui_theme.h"
#include "he/editor/framework/schema_edit.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/type_edit_ui_service.h"
#include "he/editor/widgets/buttons.h"
#include "he/editor/widgets/input_text.h"
#include "he/editor/widgets/property_grid.h"
#include "he/schema/dynamic.h"
#include "he/schema/layout.h"
#include "imgui.h"

namespace he
{
    template <>
    const char* EnumTraits<editor::ImportAssetDialog::State>::ToString(editor::ImportAssetDialog::State x) noexcept
    {
        switch (x)
        {
            case editor::ImportAssetDialog::State::CopyOrMove: return "CopyOrMove";
            case editor::ImportAssetDialog::State::Importing: return "Importing";
            case editor::ImportAssetDialog::State::EditSettings: return "EditSettings";
            case editor::ImportAssetDialog::State::Error: return "Error";
            case editor::ImportAssetDialog::State::Done: return "Done";
        }

        return "<unknown>";
    }
}

namespace he::editor
{
    ImportAssetDialog::ImportAssetDialog(
        AssetService& assetService,
        DialogService& dialogService,
        PlatformService& platformService,
        TypeEditUIService& typeEditUIService) noexcept
        : m_assetService(assetService)
        , m_dialogService(dialogService)
        , m_platformService(platformService)
        , m_typeEditUIService(typeEditUIService)
    {
        m_importBinding = m_assetService.OnImport().Attach<&ImportAssetDialog::OnImportComplete>(this);
    }

    ImportAssetDialog::~ImportAssetDialog() noexcept
    {
        m_importBinding.Detach();
    }

    void ImportAssetDialog::Configure(StringView path, StringView moveHint, const schema::DynamicStruct::Reader& data)
    {
        const Span<const String> contentRoots = m_assetService.AssetDB().ContentRoots();

        HE_LOG_DEBUG(he_editor,
            HE_MSG("ImportAssetDialog opened."),
            HE_KV(path, path),
            HE_KV(move_hint, moveHint),
            HE_KV(has_data, data.Struct().IsValid()),
            HE_KV(state, m_state.Load()));

        if (!HE_VERIFY(m_state == State::Done))
            Close();

        m_hasMoveHint = !moveHint.IsEmpty();
        m_importDstValid = true;
        m_path = path;
        NormalizePath(m_path);
        m_importDst = moveHint;
        CheckImportDst();

        if (!HE_VERIFY(moveHint.IsEmpty() || m_importDstValid))
        {
            Close();
        }

        if (data.Struct().IsValid())
        {
            m_editCtx.SetData(data);
            m_state = State::EditSettings;
        }

        if (m_importDst.IsEmpty())
        {
            if (!IsPathInContentRoots(m_path))
            {
                m_state = State::CopyOrMove;
            }
        }
        else if (GetDirectory(m_path) != m_importDst)
        {
            m_state = State::CopyOrMove;
        }

        if (m_state == State::Done)
        {
            StartImport();
        }
    }

    void ImportAssetDialog::ShowContent()
    {
        switch (m_state)
        {
            case State::CopyOrMove:
            {
                const char* msg = m_hasMoveHint
                    ? "The asset source file is not in the directory you're importing into. Would you like to copy or move the file into the import destination?"
                    : "Asset source file is not within the assets directory. Would you like to copy or move the file into the import destination?";

                ImGui::TextUnformatted(msg);
                ImGui::NewLine();

                ImGui::TextUnformatted("Asset Source");
                InputText("##asset_source", m_path, ImGuiInputTextFlags_ReadOnly);

                ImGui::TextUnformatted("Import Destination");
                if (InputOpenFolder("##import_dest", m_importDst, m_platformService))
                {
                    CheckImportDst();
                }

                if (!m_importDstValid)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Error);
                    ImGui::TextUnformatted("The import destination must be specified, and must be within a module's content root.");
                    ImGui::PopStyleColor();
                }
                break;
            }
            case State::Importing:
            {
                ImGui::Text("Importing: %s", m_path.Data());
                ImGui::ProgressBar(-1.0f);
                break;
            }
            case State::EditSettings:
            {
                HE_ASSERT(m_editCtx.Data().Struct().IsValid());
                SchemaEdit edit(m_editCtx);
                PropertyGrid(m_editCtx.Data().AsReader(), m_typeEditUIService, edit);
                m_editCtx.PushEdit(Move(edit));
                break;
            }
            case State::Error:
            {
                ImGui::TextUnformatted("Failed to import file. Check the log for details.");
                break;
            }
            case State::Done:
            {
                ImGui::TextUnformatted("Import complete!");
                break;
            }
        }
    }

    void ImportAssetDialog::ShowButtons()
    {
        switch (m_state)
        {
            case State::CopyOrMove:
            {
                ImGui::BeginDisabled(!m_importDstValid);

                if (DialogButton("Copy"))
                    HandleCopyOrMove(true);

                ImGui::SameLine();
                if (DialogButton("Move"))
                    HandleCopyOrMove(false);

                ImGui::EndDisabled();

                ImGui::SameLine();
                if (DialogButton("Cancel"))
                    Close();
                break;
            }
            case State::Importing:
            {
                break;
            }
            case State::EditSettings:
            {
                if (DialogButton("Import"))
                    StartImport();

                ImGui::SameLine();
                if (DialogButton("Cancel"))
                    Close();
                break;
            }
            case State::Error:
            case State::Done:
            {
                if (DialogButton("OK"))
                    Close();
                break;
            }
        }
    }

    bool ImportAssetDialog::IsPathInContentRoots(const String& path)
    {
        const Span<const String> contentRoots = m_assetService.AssetDB().ContentRoots();

        for (const String& root : contentRoots)
        {
            if (path == root || IsChildPath(path, root))
            {
                return true;
            }
        }

        return false;
    }

    void ImportAssetDialog::CheckImportDst()
    {
        NormalizePath(m_importDst);
        m_importDstValid = !m_importDst.IsEmpty() && IsPathInContentRoots(m_importDst);
    }

    void ImportAssetDialog::HandleCopyOrMove(bool copy)
    {
        String dst = m_importDst;
        ConcatPath(dst, GetBaseName(m_path));

        Result r;

        if (copy)
            r = File::Copy(m_path.Data(), dst.Data());
        else
            r = File::Rename(m_path.Data(), dst.Data());

        if (!r)
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to copy asset source while importing."),
                HE_KV(result, r),
                HE_KV(source_path, m_path),
                HE_KV(dest_path, dst));
            m_state = State::Error;
            return;
        }

        m_path = dst;

        if (m_editCtx.Data().Struct().IsValid())
            m_state = State::EditSettings;
        else
            StartImport();
    }

    void ImportAssetDialog::StartImport()
    {
        // Do the move ahead of the state change, so we don't race against StartImport.
        // If we move this in the param of the StartImport() call and then cleared afterwards it
        // is possible that we'll handle a NeedSettings result in OnImportComplete before we call
        // Clear(). Which will clear out the new data that was put in.
        schema::Builder builder = Move(m_editCtx.Builder());
        m_editCtx.Clear();

        m_state = State::Importing;
        m_assetService.StartImport(m_path.Data(), Move(builder));
    }

    void ImportAssetDialog::OnImportComplete(assets::ImportError error, const assets::ImportContext& ctx, const assets::ImportResult& result)
    {
        if (m_state != State::Importing)
            return;

        if (m_path != ctx.file)
            return;

        switch (error)
        {
            case assets::ImportError::NeedSettings:
            {
                schema::DynamicStruct::Reader requestedSettings = result.RequestedSettings();
                m_editCtx.SetData(requestedSettings);
                m_state = State::EditSettings;
                break;
            }
            case assets::ImportError::Success:
                m_state = State::Done;
                break;
            case assets::ImportError::Failure:
                m_state = State::Error;
                break;
        }
    }
}

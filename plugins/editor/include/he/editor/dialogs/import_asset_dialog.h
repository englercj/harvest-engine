// Copyright Chad Engler

#pragma once

#include "he/assets/asset_importer.h"
#include "he/core/atomic.h"
#include "he/core/signal.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/editor/dialogs/dialog.h"
#include "he/editor/framework/schema_edit.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/type_edit_ui_service.h"
#include "he/schema/dynamic.h"

namespace he::assets
{
    class ImportResult;
    struct ImportContext;
}

namespace he::editor
{
    class ImportAssetDialog : public Dialog
    {
    public:
        ImportAssetDialog(
            AssetService& assetService,
            DialogService& dialogService,
            PlatformService& platformService,
            TypeEditUIService& typeEditUIService) noexcept;
        ~ImportAssetDialog() noexcept;

        void Configure(StringView path, StringView moveHint = {}, const schema::DynamicStruct::Reader& importSettings = {});

        void ShowContent() override;
        void ShowButtons() override;

    private:
        bool IsPathInContentRoots(const String& path);
        void CheckImportDst();
        void HandleCopyOrMove(bool copy);
        void StartImport();
        void OnImportComplete(assets::ImportError error, const assets::ImportContext& ctx, const assets::ImportResult& result);

    private:
        enum class State : uint8_t
        {
            CopyOrMove,
            Importing,
            EditSettings,
            Error,
            Done,
        };

        friend struct he::EnumTraits<State>;

    private:
        AssetService& m_assetService;
        DialogService& m_dialogService;
        PlatformService& m_platformService;
        TypeEditUIService& m_typeEditUIService;

        String m_path{};
        String m_importDst{};
        SchemaEditContext m_editCtx{};
        AssetService::ImportCompleteSignal::Binding m_importBinding{};

        bool m_hasMoveHint{ false };
        bool m_importDstValid{ true };
        Atomic<State> m_state{ State::Done };
    };
}

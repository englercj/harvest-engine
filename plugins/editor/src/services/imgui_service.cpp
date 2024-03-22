// Copyright Chad Engler

#include "he/editor/services/imgui_service.h"

#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/string.h"
#include "he/editor/framework/imgui_theme.h"
#include "he/math/types_fmt.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"

namespace he::editor
{
    const char* ImGuiService::DndPayloadId = "HE_DND_FILE";

    ImGuiService::ImGuiService(
        EditorData& editorData,
        ImGuiPlatformService& imguiPlatformService,
        ImGuiRenderService& imguiRenderService,
        RenderService& renderService) noexcept
        : m_editorData(editorData)
        , m_imguiPlatformService(imguiPlatformService)
        , m_imguiRenderService(imguiRenderService)
        , m_renderService(renderService)
    {
        ImGui::SetAllocatorFunctions(
            [](size_t size, void* alloc) -> void* { return static_cast<Allocator*>(alloc)->Malloc(size); },
            [](void* ptr, void* alloc) { static_cast<Allocator*>(alloc)->Free(ptr); },
            &Allocator::GetDefault());
    }

    bool ImGuiService::Initialize(window::View* view)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        SetThemeColors(ImGui::GetStyle(), ImPlot::GetStyle());
        SetThemeStyle(ImGui::GetStyle(), ImPlot::GetStyle(), 1.0f);

        auto setupFonts = ImGuiPlatformService::FontsSetupDelegate::Make<&ImGuiService::SetupFonts>(this);

        if (!m_imguiPlatformService.Initialize(m_editorData.device, view, setupFonts))
            return false;

        if (!m_imguiRenderService.Initialize(m_renderService.GetSwapChainFormat()))
            return false;

        // TODO: Fonts

        return true;
    }

    void ImGuiService::Terminate()
    {
        m_imguiRenderService.Terminate();
        m_imguiPlatformService.Terminate();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    void ImGuiService::NewFrame()
    {
        m_imguiRenderService.NewFrame();
        m_imguiPlatformService.NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiService::Update()
    {
        ImGui::Render();
        ImGui::UpdatePlatformWindows();
        m_imguiPlatformService.UpdateViews();

        if (m_dndActive)
        {
            ImGuiDragDropFlags flags = ImGuiDragDropFlags_SourceExtern | ImGuiDragDropFlags_SourceNoPreviewTooltip;
            if (ImGui::BeginDragDropSource(flags))
            {
                ImGui::SetDragDropPayload(DndPayloadId, nullptr, 0, ImGuiCond_Always);
                ImGui::EndDragDropSource();
            }
        }
    }

    void ImGuiService::Render()
    {
        m_imguiRenderService.Render();
        ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
    }

    void ImGuiService::OnEvent(const window::Event& ev)
    {
        m_imguiPlatformService.OnEvent(ev);

        ImGuiIO& io = ImGui::GetIO();

        switch (ev.kind)
        {
            case window::EventKind::ViewDndStart:
            {
                m_dndActive = true;
                m_dndPaths.Clear();
                io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
                break;
            }
            case window::EventKind::ViewDndMove:
            {
                const auto& evt = static_cast<const window::ViewDndMoveEvent&>(ev);
                io.AddMousePosEvent(evt.pos.x, evt.pos.y);
                break;
            }
            case window::EventKind::ViewDndDrop:
            {
                const auto& evt = static_cast<const window::ViewDndDropEvent&>(ev);
                m_dndPaths.EmplaceBack(evt.path);
                break;
            }
            case window::EventKind::ViewDndEnd:
            {
                io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
                m_dndActive = false;
                break;
            }
            default:
                break;
        }
    }

    window::ViewDropEffect ImGuiService::GetDropEffect([[maybe_unused]] window::View* view) const
    {
        const bool accept = ImGui::IsDragDropPayloadBeingAccepted();
        return accept ? window::ViewDropEffect::Copy : window::ViewDropEffect::Reject;
    }

    void ImGuiService::SetupFonts(ImFontAtlas& atlas, float dpiScale)
    {
        SetThemeFonts(atlas, dpiScale);

        // Inform the render service that there's a new font
        [[maybe_unused]] const bool result = m_imguiRenderService.SetupFontAtlas(atlas);
        HE_ASSERT(result);
    }
}

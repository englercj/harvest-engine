// Copyright Chad Engler

#include "animations.h"

#include "he/math/float.h"

#include "imgui.h"

namespace he::editor
{
    static ImVec2 LerpVec2(const ImVec2& from, const ImVec2& to, double startTime, double duration)
    {
        const double now = ImGui::GetTime();
        const double elapsed = Min(now - startTime, duration);

        const float t = static_cast<float>(elapsed / duration);
        const float x = Lerp(from.x, to.x, t);
        const float y = Lerp(from.y, to.y, t);

        return { x, y };
    }

    void AnimNextWindowPos(const ImVec2& from, const ImVec2& to, double startTime, double duration)
    {
        ImGui::SetNextWindowPos(LerpVec2(from, to, startTime, duration));
    }

    void AnimNextWindowSize(const ImVec2& from, const ImVec2& to, double startTime, double duration)
    {
        ImGui::SetNextWindowSize(LerpVec2(from, to, startTime, duration));
    }
}

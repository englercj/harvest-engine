// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

struct ImVec2;

namespace he::editor
{
    void AnimNextWindowPos(const ImVec2& from, const ImVec2& to, double startTime, double duration);
    void AnimNextWindowSize(const ImVec2& from, const ImVec2& to, double startTime, double duration);
}

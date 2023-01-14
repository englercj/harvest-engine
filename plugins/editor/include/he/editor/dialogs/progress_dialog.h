// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/sync.h"
#include "he/editor/dialogs/dialog.h"

namespace he::editor
{
    class ProgressDialog : public Dialog
    {
    public:
        void Configure(const char* title, const char* msg, float min = 0.0f, float max = 1.0f);

        void SetMessage(const char* msg);
        void SetProgress(float value);

        void ShowContent() override;
        void ShowButtons() override;

    private:
        String m_message{};
        float m_min{ 0.0f };
        float m_max{ 1.0f };
        float m_value{ 0.0f };

        RWLock m_rwLock{};
    };
}

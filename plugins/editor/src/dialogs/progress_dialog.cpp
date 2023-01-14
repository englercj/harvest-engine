// Copyright Chad Engler

#include "he/editor/dialogs/progress_dialog.h"

#include "he/editor/widgets/buttons.h"
#include "he/editor/widgets/progress.h"
#include "he/math/constants.h"

namespace he::editor
{
    void ProgressDialog::Configure(const char* title, const char* msg, float min, float max)
    {
        LockGuard lock(m_rwLock);
        m_title = title;
        m_message = msg;
        m_min = min;
        m_max = max;
    }

    void ProgressDialog::SetMessage(const char* msg)
    {
        LockGuard lock(m_rwLock);
        m_message = msg;
    }

    void ProgressDialog::SetProgress(float value)
    {
        LockGuard lock(m_rwLock);
        m_value = value;
    }

    void ProgressDialog::ShowContent()
    {
        ReadLockGuard lock(m_rwLock);

        const float value = m_value == Float_Infinity ? -1.0f : ((m_value - m_min) / (m_max - m_min));

        ImGui::TextUnformatted(m_message.Data());
        ImGui::ProgressBar(value);
    }

    void ProgressDialog::ShowButtons()
    {
        // TODO: cancel?
    }
}

// Copyright Chad Engler

#include "he/editor/documents/log_document.h"

#include "he/core/fmt.h"
#include "he/core/clock_fmt.h"
#include "he/core/key_value_fmt.h"
#include "he/core/string_fmt.h"

#include "imgui.h"

namespace he::editor
{
    LogDocument::LogDocument(LogService& logService) noexcept
        : m_logService(logService)
    {
        m_title = "Log";
    }

    void LogDocument::Show()
    {
        UpdateBuffer();

        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(m_buffer.Begin(), m_buffer.End());
        ImGui::EndChild();
    }

    void LogDocument::UpdateBuffer()
    {
        const uint32_t hash = m_logService.GetEntriesHash();

        if (m_lastHash == hash)
            return;

        m_lastHash = hash;

        m_buffer.Clear();
        m_logService.ForEach([&](const LogService::Entry& entry)
        {
            double fractionalSeconds = entry.timestamp.val / static_cast<double>(he::Seconds::Ratio);
            fractionalSeconds -= static_cast<uint64_t>(fractionalSeconds);

            FormatTo(
                m_buffer,
                "[{:%Y-%m-%d_%H-%M-%S}{:.04f}] [{:s}]({}) {}\n",
                FmtLocalTime(entry.timestamp),
                fractionalSeconds,
                entry.source.level,
                entry.source.category,
                FmtJoin(entry.kvs, ", "));

            return true;
        });
    }
}

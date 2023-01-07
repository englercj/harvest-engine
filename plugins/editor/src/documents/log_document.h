// Copyright Chad Engler

#pragma once

#include "document.h"

#include "services/log_service.h"

namespace he::editor
{
    class LogDocument : public Document
    {
    public:
        LogDocument(LogService& logService) noexcept;

        void Show() override;

    private:
        void UpdateBuffer();

    private:
        LogService& m_logService;

        String m_buffer{};
        uint32_t m_lastHash{ 0 };
    };
}

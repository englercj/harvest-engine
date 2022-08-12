// Copyright Chad Engler

#pragma once

#include "he/core/async_file_loader.h"

namespace he::editor
{
    class FileLoaderService
    {
    public:
        bool Initialize();
        void Terminate();

        AsyncFileLoader& Loader() { return *m_loader; }
        const AsyncFileLoader& Loader() const { return *m_loader; }

    private:
        AsyncFileLoader* m_loader{ nullptr };
    };
}

// Copyright Chad Engler

#include "file_loader_service.h"

#include "he/core/assert.h"
#include "he/core/result_fmt.h"
#include "he/core/log.h"

namespace he::editor
{
    bool FileLoaderService::Initialize()
    {
        HE_ASSERT(m_loader == nullptr);

        AsyncFileLoader::Config config;
        Result r = AsyncFileLoader::Create(config, m_loader);
        if (!r)
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to create async file loader."), HE_KV(result, r));
            return false;
        }
        return true;
    }

    void FileLoaderService::Terminate()
    {
        AsyncFileLoader::Destroy(m_loader);
        m_loader = nullptr;
    }
}

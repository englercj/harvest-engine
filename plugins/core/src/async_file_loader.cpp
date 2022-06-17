// Copyright Chad Engler

#include "he/core/async_file_loader.h"

#include "he/core/enum_ops.h"

namespace he
{
    extern AsyncFileLoader* _CreateAsyncFileLoader(Allocator& allocator);

    Result AsyncFileLoader::Create(const AsyncFileLoader::Config& config, AsyncFileLoader*& out)
    {
        Allocator& alloc = config.allocator ? *config.allocator : Allocator::GetDefault();
        out = _CreateAsyncFileLoader(alloc);

        Result r = out->Initialize(config);
        if (!r)
        {
            AsyncFileLoader::Destroy(out);
            out = nullptr;
            return r;
        }

        return Result::Success;
    }

    void AsyncFileLoader::Destroy(AsyncFileLoader* loader)
    {
        if (loader)
        {
            loader->GetAllocator().Delete(loader);
        }
    }

    template <>
    const char* AsString(AsyncFileRequest::CompressionFormat x)
    {
        switch (x)
        {
            case AsyncFileRequest::CompressionFormat::None: return "None";
            case AsyncFileRequest::CompressionFormat::Zlib: return "None";
        }
        return "<unknown>";
    }
}

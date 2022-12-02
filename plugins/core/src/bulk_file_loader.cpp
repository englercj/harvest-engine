// Copyright Chad Engler

#include "he/core/bulk_file_loader.h"

#include "he/core/enum_ops.h"

namespace he
{
    extern BulkFileLoader* _CreateBulkFileLoader(Allocator& allocator);

    Result BulkFileLoader::Create(const BulkFileLoader::Config& config, BulkFileLoader*& out)
    {
        Allocator& alloc = config.allocator ? *config.allocator : Allocator::GetDefault();
        out = _CreateBulkFileLoader(alloc);

        Result r = out->Initialize(config);
        if (!r)
        {
            BulkFileLoader::Destroy(out);
            out = nullptr;
            return r;
        }

        return Result::Success;
    }

    void BulkFileLoader::Destroy(BulkFileLoader* loader)
    {
        if (loader)
        {
            loader->GetAllocator().Delete(loader);
        }
    }

    template <>
    const char* AsString(BulkReadRequest::CompressionFormat x)
    {
        switch (x)
        {
            case BulkReadRequest::CompressionFormat::None: return "None";
            case BulkReadRequest::CompressionFormat::Zlib: return "Zlib";
        }
        return "<unknown>";
    }
}

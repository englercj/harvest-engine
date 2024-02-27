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
    const char* EnumTraits<BulkReadRequest::CompressionFormat>::ToString(BulkReadRequest::CompressionFormat x) noexcept
    {
        switch (x)
        {
            case BulkReadRequest::CompressionFormat::None: return "None";
            case BulkReadRequest::CompressionFormat::Zlib: return "Zlib";
            case BulkReadRequest::CompressionFormat::GDeflate: return "GDeflate";
        }
        return "<unknown>";
    }

    template <>
    const char* EnumTraits<BulkFileQueue::Config::Priority>::ToString(BulkFileQueue::Config::Priority x) noexcept
    {
        switch (x)
        {
            case BulkFileQueue::Config::Priority::Low: return "Low";
            case BulkFileQueue::Config::Priority::Normal: return "Normal";
            case BulkFileQueue::Config::Priority::High: return "High";
            case BulkFileQueue::Config::Priority::Realtime: return "Realtime";
        }
        return "<unknown>";
    }
}

// Copyright Chad Engler

#pragma once

#include "he/core/vector.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    struct BufferImpl final : Buffer
    {
        explicit BufferImpl(Allocator& allocator) noexcept : data(allocator) {}

        HeapType heapType{ HeapType::Default };
        BufferUsage usage{ BufferUsage::None };
        uint32_t size{ 0 };
        uint32_t stride{ 1 };
        Vector<uint8_t> data;
    };

    struct CpuFenceImpl final : CpuFence
    {
        bool signaled{ true };
    };

    struct GpuFenceImpl final : GpuFence
    {
        uint64_t value{ 0 };
    };

    struct TimestampQuerySetImpl final : TimestampQuerySet
    {
        explicit TimestampQuerySetImpl(Allocator& allocator) noexcept : timestamps(allocator) {}

        CmdListType type{ CmdListType::Render };
        Vector<uint64_t> timestamps;
    };
}

#endif

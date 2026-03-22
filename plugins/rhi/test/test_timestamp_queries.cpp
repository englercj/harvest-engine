// Copyright Chad Engler

#include "he/core/test.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"

namespace he
{
    HE_TEST(rhi, timestamp_queries, null_round_trip)
    {
        rhi::Instance* instance = nullptr;
        rhi::Device* device = nullptr;
        rhi::RenderCmdList* cmdList = nullptr;
        rhi::TimestampQuerySet* querySet = nullptr;
        rhi::Buffer* readback = nullptr;

        rhi::InstanceDesc instanceDesc{};
        instanceDesc.api = rhi::Api_Null;
        HE_EXPECT(rhi::Instance::Create(instanceDesc, instance));
        HE_EXPECT_NE_PTR(instance, nullptr);

        if (instance)
        {
            rhi::DeviceDesc deviceDesc{};
            HE_EXPECT(instance->CreateDevice(deviceDesc, device));
            HE_EXPECT_NE_PTR(device, nullptr);
        }

        if (device)
        {
            HE_EXPECT_GT(device->GetRenderCmdQueue().GetTimestampFrequency(), 0ull);

            rhi::TimestampQuerySetDesc queryDesc{};
            queryDesc.type = rhi::CmdListType::Render;
            queryDesc.count = 2;
            HE_EXPECT(device->CreateTimestampQuerySet(queryDesc, querySet));
            HE_EXPECT_NE_PTR(querySet, nullptr);

            rhi::BufferDesc bufferDesc{};
            bufferDesc.heapType = rhi::HeapType::Readback;
            bufferDesc.usage = rhi::BufferUsage::CopyDst;
            bufferDesc.size = sizeof(uint64_t) * 2;
            bufferDesc.stride = sizeof(uint64_t);
            HE_EXPECT(device->CreateBuffer(bufferDesc, readback));
            HE_EXPECT_NE_PTR(readback, nullptr);

            rhi::CmdListDesc cmdListDesc{};
            HE_EXPECT(device->CreateRenderCmdList(cmdListDesc, cmdList));
            HE_EXPECT_NE_PTR(cmdList, nullptr);
        }

        if (cmdList && querySet && readback)
        {
            HE_EXPECT(cmdList->Begin(nullptr));
            cmdList->WriteTimestamp(querySet, 0);
            cmdList->WriteTimestamp(querySet, 1);
            cmdList->ResolveTimestamps(querySet, 0, 2, readback);
            HE_EXPECT(cmdList->End());

            device->GetRenderCmdQueue().Submit(cmdList);

            const uint64_t* timestamps = static_cast<const uint64_t*>(
                device->Map(readback, 0, sizeof(uint64_t) * 2));

            HE_EXPECT_NE_PTR(timestamps, nullptr);
            if (timestamps)
            {
                HE_EXPECT_LE(timestamps[0], timestamps[1]);
                HE_EXPECT_GT(timestamps[1], 0ull);
                device->Unmap(readback);
            }
        }

        if (device)
        {
            device->SafeDestroy(cmdList);
            device->SafeDestroy(readback);
            device->SafeDestroy(querySet);
            instance->DestroyDevice(device);
        }

        if (instance)
        {
            rhi::Instance::Destroy(instance);
        }
    }
}

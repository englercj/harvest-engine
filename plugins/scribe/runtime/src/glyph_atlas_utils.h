// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"

#include "glyph_atlas.h"

#include "he/core/allocator.h"
#include "he/core/hash.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"

namespace he::scribe
{
    inline bool UploadTexturePair2D(
        rhi::Device& device,
        rhi::Texture*& outTexture0,
        rhi::TextureView*& outView0,
        const TextureDataDesc& textureData0,
        rhi::Format format0,
        const char* textureName0,
        const char* uploadBufferName0,
        rhi::Texture*& outTexture1,
        rhi::TextureView*& outView1,
        const TextureDataDesc& textureData1,
        rhi::Format format1,
        const char* textureName1,
        const char* uploadBufferName1)
    {
        struct PendingTextureUpload
        {
            rhi::Texture** texture{ nullptr };
            rhi::TextureView** view{ nullptr };
            TextureDataDesc data{};
            rhi::Format format{ rhi::Format::Invalid };
            const char* textureName{ nullptr };
            const char* uploadBufferName{ nullptr };
            rhi::Buffer* uploadBuffer{ nullptr };
            uint32_t uploadPitch{ 0 };
        };

        PendingTextureUpload uploads[2]{};
        uploads[0].texture = &outTexture0;
        uploads[0].view = &outView0;
        uploads[0].data = textureData0;
        uploads[0].format = format0;
        uploads[0].textureName = textureName0;
        uploads[0].uploadBufferName = uploadBufferName0;
        uploads[1].texture = &outTexture1;
        uploads[1].view = &outView1;
        uploads[1].data = textureData1;
        uploads[1].format = format1;
        uploads[1].textureName = textureName1;
        uploads[1].uploadBufferName = uploadBufferName1;

        HE_AT_SCOPE_EXIT([&]()
        {
            for (PendingTextureUpload& upload : uploads)
            {
                device.SafeDestroy(upload.uploadBuffer);
            }
        });

        const uint32_t alignment = device.GetDeviceInfo().uploadDataPitchAlignment;
        for (PendingTextureUpload& upload : uploads)
        {
            const Vec3u textureSize{ upload.data.size.x, upload.data.size.y, 1 };

            rhi::TextureDesc textureDesc{};
            textureDesc.type = rhi::TextureType::_2D;
            textureDesc.format = upload.format;
            textureDesc.size = textureSize;
            textureDesc.initialState = rhi::TextureState::CopyDst;
            HE_RHI_SET_NAME(textureDesc, upload.textureName);

            Result r = device.CreateTexture(textureDesc, *upload.texture);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create texture '{}'. Error: {}", upload.textureName, r);
                return false;
            }

            upload.uploadPitch = AlignUp<uint32_t>(upload.data.rowPitch, alignment);
            const uint32_t uploadSize = upload.data.size.y * upload.uploadPitch;

            rhi::BufferDesc bufferDesc{};
            bufferDesc.heapType = rhi::HeapType::Upload;
            bufferDesc.usage = rhi::BufferUsage::CopySrc;
            bufferDesc.size = uploadSize;
            HE_RHI_SET_NAME(bufferDesc, upload.uploadBufferName);

            r = device.CreateBuffer(bufferDesc, upload.uploadBuffer);
            if (!r)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create upload buffer '{}'. Error: {}", upload.uploadBufferName, r);
                return false;
            }

            uint8_t* uploadMem = static_cast<uint8_t*>(device.Map(upload.uploadBuffer));
            const uint8_t* src = static_cast<const uint8_t*>(upload.data.data);
            for (uint32_t y = 0; y < upload.data.size.y; ++y)
            {
                MemCopy(uploadMem + (y * upload.uploadPitch), src + (y * upload.data.rowPitch), upload.data.rowPitch);
            }
            device.Unmap(upload.uploadBuffer);
        }

        rhi::CmdAllocator* cmdAllocator = nullptr;
        rhi::RenderCmdList* cmdList = nullptr;
        HE_AT_SCOPE_EXIT([&]()
        {
            device.SafeDestroy(cmdList);
            device.SafeDestroy(cmdAllocator);
        });

        rhi::CmdAllocatorDesc allocatorDesc{};
        allocatorDesc.type = rhi::CmdListType::Render;
        HE_RHI_SET_NAME(allocatorDesc, "Scribe Upload Cmd Allocator");

        Result r = device.CreateCmdAllocator(allocatorDesc, cmdAllocator);
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to create upload cmd allocator. Error: {}", r);
            return false;
        }

        rhi::CmdListDesc cmdListDesc{};
        cmdListDesc.alloc = cmdAllocator;
        HE_RHI_SET_NAME(cmdListDesc, "Scribe Upload Cmd List");

        r = device.CreateRenderCmdList(cmdListDesc, cmdList);
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to create upload cmd list. Error: {}", r);
            return false;
        }

        r = cmdList->Begin(cmdAllocator);
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to begin upload cmd list. Error: {}", r);
            return false;
        }

        for (PendingTextureUpload& upload : uploads)
        {
            rhi::BufferTextureCopy copyRegion{};
            copyRegion.bufferRowPitch = upload.uploadPitch;
            copyRegion.textureSize = { upload.data.size.x, upload.data.size.y, 1 };
            cmdList->Copy(upload.uploadBuffer, *upload.texture, copyRegion);
            cmdList->TransitionBarrier(*upload.texture, rhi::TextureState::CopyDst, rhi::TextureState::PixelShaderRead);
        }

        r = cmdList->End();
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to end upload cmd list. Error: {}", r);
            return false;
        }

        rhi::RenderCmdQueue& cmdQueue = device.GetRenderCmdQueue();
        cmdQueue.Submit(cmdList);
        cmdQueue.WaitForFlush();

        for (PendingTextureUpload& upload : uploads)
        {
            rhi::TextureViewDesc viewDesc{};
            viewDesc.texture = *upload.texture;
            Result viewResult = device.CreateTextureView(viewDesc, *upload.view);
            if (!viewResult)
            {
                HE_LOGF_ERROR(scribe_render, "Failed to create texture view '{}'. Error: {}", upload.textureName, viewResult);
                return false;
            }
        }

        return true;
    }

    inline bool CreateAtlasDescriptorTable(rhi::Device& device, GlyphAtlas& atlas)
    {
        rhi::DescriptorRange ranges[2]{};
        ranges[0].type = rhi::DescriptorRangeType::Texture;
        ranges[0].baseRegister = 0;
        ranges[0].registerSpace = 0;
        ranges[0].count = 1;
        ranges[1].type = rhi::DescriptorRangeType::Texture;
        ranges[1].baseRegister = 1;
        ranges[1].registerSpace = 0;
        ranges[1].count = 1;

        rhi::DescriptorTableDesc tableDesc{};
        tableDesc.rangeCount = HE_LENGTH_OF(ranges);
        tableDesc.ranges = ranges;

        Result r = device.CreateDescriptorTable(tableDesc, atlas.descriptorTable);
        if (!r)
        {
            HE_LOGF_ERROR(scribe_render, "Failed to create glyph descriptor table. Error: {}", r);
            return false;
        }

        const rhi::TextureView* curveView = atlas.curveView;
        const rhi::TextureView* bandView = atlas.bandView;
        device.SetTextureViews(atlas.descriptorTable, 0, 0, 1, &curveView);
        device.SetTextureViews(atlas.descriptorTable, 1, 0, 1, &bandView);
        return true;
    }

    inline void DestroyAtlas(rhi::Device& device, GlyphAtlas*& atlas)
    {
        if (!atlas)
        {
            return;
        }

        device.SafeDestroy(atlas->descriptorTable);
        device.SafeDestroy(atlas->bandView);
        device.SafeDestroy(atlas->bandTexture);
        device.SafeDestroy(atlas->curveView);
        device.SafeDestroy(atlas->curveTexture);
        Allocator::GetDefault().Delete(atlas);
        atlas = nullptr;
    }
}

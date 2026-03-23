// Copyright Chad Engler

#include "image_compile_geometry.h"

#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/retained_vector_image.h"
#include "he/scribe/renderer.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/test.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"

using namespace he;
using namespace he::scribe;
using namespace he::scribe::editor;

namespace
{
    constexpr const char* kSvgSource =
        "<svg viewBox=\"0 0 180 180\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#111827\" d=\"M0 0 L180 0 L180 180 L0 180 Z\"/>"
        "<path fill=\"#e5e7eb\" fill-rule=\"evenodd\" d=\"M24 24 L156 24 L156 156 L24 156 Z M52 52 L52 128 L128 128 L128 52 Z\"/>"
        "<path fill=\"#22c55e\" d=\"M90 36 L108 72 L148 78 L118 106 L126 146 L90 126 L54 146 L62 106 L32 78 L72 72 Z\"/>"
        "</svg>";

    constexpr const char* kSvgWithUse =
        "<svg viewBox=\"0 0 64 64\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
        "<defs>"
        "<path id=\"diamond\" d=\"M8 0 L16 8 L8 16 L0 8 Z\"/>"
        "</defs>"
        "<g fill=\"#ef4444\">"
        "<use xlink:href=\"#diamond\" x=\"8\" y=\"8\"/>"
        "</g>"
        "</svg>";

    constexpr const char* kSvgWithTextNodes =
        "<svg viewBox=\"0 0 64 64\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#111827\" d=\"M4 4 L60 4 L60 60 L4 60 Z\"/>"
        "<text x=\"8\" y=\"20\">Scribe SVG</text>"
        "<path fill=\"#22c55e\" d=\"M16 16 L48 16 L48 48 L16 48 Z\"/>"
        "</svg>";

    bool BuildLoadedVectorImage(Vector<schema::Word>& storage, LoadedVectorImageBlob& out)
    {
        CompiledVectorImageData imageData{};
        if (!BuildCompiledVectorImageData(imageData, Span(reinterpret_cast<const uint8_t*>(kSvgSource), StrLen(kSvgSource)), 0.25f))
        {
            return false;
        }

        schema::Builder metadataBuilder;
        VectorImageRuntimeMetadata::Builder metadata = metadataBuilder.AddStruct<VectorImageRuntimeMetadata>();
        metadata.SetSourceViewBoxMinX(imageData.viewBoxMinX);
        metadata.SetSourceViewBoxMinY(imageData.viewBoxMinY);
        metadata.SetSourceViewBoxWidth(imageData.viewBoxWidth);
        metadata.SetSourceViewBoxHeight(imageData.viewBoxHeight);
        metadata.SetSourceBoundsMinX(imageData.boundsMinX);
        metadata.SetSourceBoundsMinY(imageData.boundsMinY);
        metadata.SetSourceBoundsMaxX(imageData.boundsMaxX);
        metadata.SetSourceBoundsMaxY(imageData.boundsMaxY);
        metadataBuilder.SetRoot(metadata);

        schema::Builder renderBuilder;
        VectorImageRenderData::Builder render = renderBuilder.AddStruct<VectorImageRenderData>();
        render.SetCurveTextureWidth(imageData.curveTextureWidth);
        render.SetCurveTextureHeight(imageData.curveTextureHeight);
        render.SetBandTextureWidth(imageData.bandTextureWidth);
        render.SetBandTextureHeight(imageData.bandTextureHeight);
        render.SetBandOverlapEpsilon(imageData.bandOverlapEpsilon);
        auto shapes = render.InitShapes(imageData.shapes.Size());
        for (uint32_t shapeIndex = 0; shapeIndex < imageData.shapes.Size(); ++shapeIndex)
        {
            const CompiledVectorShapeRenderEntry& srcShape = imageData.shapes[shapeIndex];
            VectorImageShapeRenderData::Builder dstShape = shapes[shapeIndex];
            dstShape.SetBoundsMinX(srcShape.boundsMinX);
            dstShape.SetBoundsMinY(srcShape.boundsMinY);
            dstShape.SetBoundsMaxX(srcShape.boundsMaxX);
            dstShape.SetBoundsMaxY(srcShape.boundsMaxY);
            dstShape.SetBandScaleX(srcShape.bandScaleX);
            dstShape.SetBandScaleY(srcShape.bandScaleY);
            dstShape.SetBandOffsetX(srcShape.bandOffsetX);
            dstShape.SetBandOffsetY(srcShape.bandOffsetY);
            dstShape.SetGlyphBandLocX(srcShape.glyphBandLocX);
            dstShape.SetGlyphBandLocY(srcShape.glyphBandLocY);
            dstShape.SetBandMaxX(srcShape.bandMaxX);
            dstShape.SetBandMaxY(srcShape.bandMaxY);
            dstShape.SetFillRule(srcShape.fillRule);
            dstShape.SetFlags(srcShape.flags);
        }
        renderBuilder.SetRoot(render);

        schema::Builder paintBuilder;
        VectorImagePaintData::Builder paint = paintBuilder.AddStruct<VectorImagePaintData>();
        auto layers = paint.InitLayers(imageData.layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < imageData.layers.Size(); ++layerIndex)
        {
            const CompiledVectorImageLayerEntry& srcLayer = imageData.layers[layerIndex];
            VectorImageLayer::Builder dstLayer = layers[layerIndex];
            dstLayer.SetShapeIndex(srcLayer.shapeIndex);
            dstLayer.SetRed(srcLayer.red);
            dstLayer.SetGreen(srcLayer.green);
            dstLayer.SetBlue(srcLayer.blue);
            dstLayer.SetAlpha(srcLayer.alpha);
        }
        paintBuilder.SetRoot(paint);

        schema::Builder rootBuilder;
        CompiledVectorImageBlob::Builder root = rootBuilder.AddStruct<CompiledVectorImageBlob>();
        RuntimeBlobHeader::Builder header = root.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::VectorImage);
        header.SetFlags(0);
        root.SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
        root.SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
        root.SetPaintData(rootBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
        root.SetMetadataData(rootBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
        root.SetRenderData(rootBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        return LoadCompiledVectorImageBlob(out, storage);
    }

    bool BuildRetainedVectorImageFromTemporaryCopy(RetainedVectorImageModel& out, const LoadedVectorImageBlob& image)
    {
        LoadedVectorImageBlob temporaryImage = image;

        RetainedVectorImageBuildDesc desc{};
        desc.image = &temporaryImage;
        return out.Build(desc);
    }

    struct NullRendererHarness
    {
        rhi::Instance* instance{ nullptr };
        rhi::Device* device{ nullptr };
        Renderer renderer{};

        ~NullRendererHarness() noexcept
        {
            Terminate();
        }

        bool Initialize()
        {
            rhi::InstanceDesc instanceDesc{};
            instanceDesc.api = rhi::Api_Null;
            if (!rhi::Instance::Create(instanceDesc, instance) || !instance)
            {
                return false;
            }

            rhi::DeviceDesc deviceDesc{};
            if (!instance->CreateDevice(deviceDesc, device) || !device)
            {
                Terminate();
                return false;
            }

            if (!renderer.Initialize(*device, rhi::Format::BGRA8Unorm_sRGB))
            {
                Terminate();
                return false;
            }

            return true;
        }

        void Terminate()
        {
            renderer.Terminate();

            if (device)
            {
                instance->DestroyDevice(device);
                device = nullptr;
            }

            if (instance)
            {
                rhi::Instance::Destroy(instance);
                instance = nullptr;
            }
        }
    };
}

HE_TEST(scribe, vector_image_pipeline, compiles_svg_source_to_payloads)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(imageData, Span(reinterpret_cast<const uint8_t*>(kSvgSource), StrLen(kSvgSource)), 0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_GT(imageData.curveTexels.Size(), 0u);
    HE_EXPECT_GT(imageData.bandTexels.Size(), 0u);
    HE_EXPECT_EQ(imageData.layers.Size(), 3u);
    HE_EXPECT_EQ(imageData.viewBoxWidth, 180.0f);
    HE_EXPECT_EQ(imageData.viewBoxHeight, 180.0f);
    HE_EXPECT_EQ(imageData.shapes[1].fillRule, FillRule::EvenOdd);

    const CompiledVectorShapeRenderEntry& shape0 = imageData.shapes[0];
    const uint32_t glyphBandStart = shape0.glyphBandLocX + (shape0.glyphBandLocY * ScribeBandTextureWidth);
    const uint32_t horizontalBandCount = shape0.bandMaxY + 1;
    const uint32_t verticalBandCount = shape0.bandMaxX + 1;
    const uint32_t headerCount = horizontalBandCount + verticalBandCount;

    HE_EXPECT_LT(glyphBandStart + headerCount, imageData.bandTexels.Size());
    HE_EXPECT_EQ(imageData.bandTexels[glyphBandStart].y, headerCount);
    HE_EXPECT_GE(imageData.bandTexels[glyphBandStart + horizontalBandCount].y, headerCount);
    HE_EXPECT_GT(imageData.bandHeaderCount, 0u);
    HE_EXPECT_GE(imageData.bandTexels.Size(), imageData.bandHeaderCount + imageData.emittedBandPayloadTexelCount);
    HE_EXPECT_EQ(imageData.bandTexels.Size(), imageData.bandTextureWidth * imageData.bandTextureHeight);
    HE_EXPECT_GT(imageData.reusedBandCount, 0u);
    HE_EXPECT_GT(imageData.reusedBandPayloadTexelCount, 0u);
}

HE_TEST(scribe, vector_image_pipeline, compiles_repeatable_svg_payloads)
{
    CompiledVectorImageData first{};
    CompiledVectorImageData second{};
    const Span<const uint8_t> source(reinterpret_cast<const uint8_t*>(kSvgSource), StrLen(kSvgSource));

    HE_ASSERT(BuildCompiledVectorImageData(first, source, 0.25f));
    HE_ASSERT(BuildCompiledVectorImageData(second, source, 0.25f));

    HE_EXPECT_EQ(first.curveTexels.Size(), second.curveTexels.Size());
    HE_EXPECT_EQ(first.bandTexels.Size(), second.bandTexels.Size());
    HE_EXPECT_EQ(first.shapes.Size(), second.shapes.Size());
    HE_EXPECT_EQ(first.layers.Size(), second.layers.Size());
    HE_EXPECT_EQ(first.bandHeaderCount, second.bandHeaderCount);
    HE_EXPECT_EQ(first.emittedBandPayloadTexelCount, second.emittedBandPayloadTexelCount);
    HE_EXPECT_EQ(first.reusedBandCount, second.reusedBandCount);
    HE_EXPECT_EQ(first.reusedBandPayloadTexelCount, second.reusedBandPayloadTexelCount);

    HE_EXPECT_EQ_MEM(
        first.curveTexels.Data(),
        second.curveTexels.Data(),
        first.curveTexels.Size() * sizeof(PackedCurveTexel));
    HE_EXPECT_EQ_MEM(
        first.bandTexels.Data(),
        second.bandTexels.Data(),
        first.bandTexels.Size() * sizeof(PackedBandTexel));
    HE_EXPECT_EQ_MEM(
        first.shapes.Data(),
        second.shapes.Data(),
        first.shapes.Size() * sizeof(CompiledVectorShapeRenderEntry));
    HE_EXPECT_EQ_MEM(
        first.layers.Data(),
        second.layers.Data(),
        first.layers.Size() * sizeof(CompiledVectorImageLayerEntry));
}

HE_TEST(scribe, vector_image_pipeline, ignores_definition_geometry_and_instantiates_use_references)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithUse), StrLen(kSvgWithUse)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.layers.Size(), 1u);
    HE_EXPECT_EQ(imageData.shapes.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers[0].red, 239.0f / 255.0f);
    HE_EXPECT_EQ(imageData.layers[0].green, 68.0f / 255.0f);
    HE_EXPECT_EQ(imageData.layers[0].blue, 68.0f / 255.0f);
}

HE_TEST(scribe, vector_image_pipeline, skips_text_elements_without_failing_svg_compile)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithTextNodes), StrLen(kSvgWithTextNodes)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.layers.Size(), 2u);
    HE_EXPECT_EQ(imageData.shapes.Size(), 2u);
}

HE_TEST(scribe, vector_image_pipeline, loads_compiled_vector_blob)
{
    Vector<schema::Word> storage;
    LoadedVectorImageBlob image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    HE_EXPECT(image.root.IsValid());
    HE_EXPECT(image.metadata.IsValid());
    HE_EXPECT(image.render.IsValid());
    HE_EXPECT(image.paint.IsValid());
    HE_EXPECT_EQ(image.metadata.GetSourceViewBoxWidth(), 180.0f);
    HE_EXPECT_EQ(image.render.GetShapes().Size(), 3u);
    HE_EXPECT_EQ(image.paint.GetLayers().Size(), 3u);
}

HE_TEST(scribe, vector_image_pipeline, resolves_layers_and_shape_resources)
{
    Vector<schema::Word> storage;
    LoadedVectorImageBlob image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    Vector<CompiledVectorImageLayer> layers{};
    HE_EXPECT(GetCompiledVectorImageLayers(layers, image));
    HE_EXPECT_EQ(layers.Size(), 3u);
    HE_EXPECT_EQ(layers[0].shapeIndex, 0u);
    HE_EXPECT_EQ(layers[1].shapeIndex, 1u);
    HE_EXPECT_GT(layers[0].color.w, 0.0f);

    CompiledVectorShapeResourceData shape{};
    HE_EXPECT(BuildCompiledVectorShapeResourceData(shape, image, 1));
    HE_EXPECT_EQ(shape.createInfo.vertexCount, ScribeGlyphVertexCount);
    HE_EXPECT_EQ(shape.createInfo.bandTexture.size.x, ScribeBandTextureWidth);
    HE_EXPECT_EQ(shape.shape.GetFillRule(), FillRule::EvenOdd);
    HE_EXPECT_EQ(shape.vertices[0].col.x, layers[1].color.x);
    HE_EXPECT_EQ(shape.vertices[0].col.y, layers[1].color.y);
    HE_EXPECT_EQ(shape.vertices[0].col.z, layers[1].color.z);
    HE_EXPECT_EQ(shape.vertices[0].col.w, layers[1].color.w);
}

HE_TEST(scribe, retained_vector_image, builds_layered_draws_from_runtime_blob)
{
    Vector<schema::Word> storage;
    LoadedVectorImageBlob image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    RetainedVectorImageBuildDesc desc{};
    desc.image = &image;
    HE_ASSERT(retainedImage.Build(desc));

    HE_EXPECT_EQ(retainedImage.GetDrawCount(), image.paint.GetLayers().Size());
    HE_EXPECT_EQ(retainedImage.GetEstimatedVertexCount(), retainedImage.GetDrawCount() * ScribeGlyphVertexCount);
    HE_EXPECT_EQ(retainedImage.GetViewBoxSize().x, 180.0f);
    HE_EXPECT_EQ(retainedImage.GetViewBoxSize().y, 180.0f);
    HE_EXPECT_NE_PTR(retainedImage.GetImage(), nullptr);
}

HE_TEST(scribe, retained_vector_image, prepares_with_renderer_after_temporary_image_copy_expires)
{
    Vector<schema::Word> storage;
    LoadedVectorImageBlob image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    HE_ASSERT(BuildRetainedVectorImageFromTemporaryCopy(retainedImage, image));
    HE_EXPECT_GT(retainedImage.GetDrawCount(), 0u);
    HE_EXPECT_NE_PTR(retainedImage.GetImage(), nullptr);

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());
    HE_EXPECT(harness.renderer.PrepareRetainedVectorImage(retainedImage));

    RetainedVectorImageInstanceDesc instance{};
    harness.renderer.QueueRetainedVectorImage(retainedImage, instance);
}

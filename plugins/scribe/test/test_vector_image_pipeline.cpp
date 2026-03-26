// Copyright Chad Engler

#include "image_compile_geometry.h"
#include "resource_build_utils.h"

#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/retained_vector_image.h"
#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

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

    constexpr const char* kSvgBasicShapes =
        "<svg viewBox=\"0 0 128 128\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<g transform=\"translate(4 6)\" style=\"fill:#ef4444; opacity:0.5\">"
        "<rect x=\"4\" y=\"4\" width=\"20\" height=\"16\"/>"
        "<circle cx=\"40\" cy=\"20\" r=\"10\"/>"
        "</g>"
        "<ellipse cx=\"88\" cy=\"28\" rx=\"18\" ry=\"10\" fill=\"#22c55e\"/>"
        "<polygon fill=\"#3b82f6\" points=\"20,72 44,56 68,72 58,100 30,102\"/>"
        "</svg>";

    constexpr const char* kSvgSmoothPathCommands =
        "<svg viewBox=\"0 0 128 96\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#111827\" d=\"M8 8 Q24 0 40 8 T72 8 L72 40 L8 40 Z\"/>"
        "<path fill=\"#22c55e\" d=\"M8 56 C20 32 44 32 56 56 S92 80 104 56 L104 88 L8 88 Z\"/>"
        "</svg>";

    constexpr const char* kSvgWithAuthoredStroke =
        "<svg viewBox=\"0 0 96 64\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"none\" stroke=\"#ef4444\" stroke-width=\"6\" stroke-linejoin=\"round\" d=\"M8 52 L24 12 L40 52 Z\"/>"
        "<rect x=\"56\" y=\"14\" width=\"20\" height=\"20\" fill=\"#22c55e\" stroke=\"#111827\" stroke-width=\"4\"/>"
        "</svg>";

    constexpr const char* kSvgWithClipPath =
        "<svg viewBox=\"0 0 64 64\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<defs><clipPath id=\"clipBox\"><rect x=\"0\" y=\"0\" width=\"20\" height=\"20\"/></clipPath></defs>"
        "<g clip-path=\"url(#clipBox)\">"
        "<rect fill=\"#111827\" x=\"4\" y=\"4\" width=\"8\" height=\"8\"/>"
        "<rect fill=\"#22c55e\" x=\"40\" y=\"40\" width=\"8\" height=\"8\"/>"
        "</g>"
        "</svg>";

    bool BuildLoadedVectorImage(Vector<schema::Word>& storage, VectorImageResourceReader& out)
    {
        CompiledVectorImageData imageData{};
        if (!BuildCompiledVectorImageData(imageData, Span(reinterpret_cast<const uint8_t*>(kSvgSource), StrLen(kSvgSource)), 0.25f))
        {
            return false;
        }

        schema::Builder rootBuilder;
        VectorImageResource::Builder root = rootBuilder.AddStruct<VectorImageResource>();

        FillVectorImageResourceMetadata(root.GetMetadata(), imageData);
        FillVectorImageResourceFillData(root.GetFill(), imageData);
        FillVectorImageResourceStrokeData(root.GetStroke(), imageData);
        FillVectorImageResourcePaintData(root.GetPaint(), imageData);
        root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
        root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        out = schema::ReadRoot<VectorImageResource>(storage.Data());
        return out.IsValid();
    }

    bool BuildRetainedVectorImageFromTemporaryCopy(
        RetainedVectorImageModel& out,
        ScribeContext& context,
        const VectorImageResourceReader& image)
    {
        const VectorImageHandle handle = context.RegisterVectorImage(image);
        if (!handle.IsValid())
        {
            return false;
        }

        RetainedVectorImageBuildDesc desc{};
        desc.context = &context;
        desc.image = handle;
        return out.Build(desc);
    }

    struct NullRendererHarness
    {
        rhi::Instance* instance{ nullptr };
        rhi::Device* device{ nullptr };
        ScribeContext context{};
        Renderer& renderer;

        NullRendererHarness() noexcept
            : renderer(context.GetRenderer())
        {
        }

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

            if (!context.Initialize(*device)
                || !renderer.Initialize(rhi::Format::BGRA8Unorm_sRGB))
            {
                Terminate();
                return false;
            }

            return true;
        }

        void Terminate()
        {
            renderer.Terminate();
            context.Terminate();

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

HE_TEST(scribe, vector_image_pipeline, compiles_basic_svg_shape_elements)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgBasicShapes), StrLen(kSvgBasicShapes)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 4u);
    HE_EXPECT_EQ(imageData.layers.Size(), 4u);
    HE_EXPECT_GT(imageData.strokeCommands.Size(), 0u);
    HE_EXPECT_GT(imageData.strokePoints.Size(), 0u);
    HE_EXPECT_EQ(imageData.layers[0].red, 239.0f / 255.0f);
    HE_EXPECT_EQ(imageData.layers[0].alpha, 0.5f);
    HE_EXPECT_EQ(imageData.layers[1].alpha, 0.5f);
    HE_EXPECT_EQ(imageData.layers[2].green, 197.0f / 255.0f);
    HE_EXPECT_EQ(imageData.layers[3].blue, 246.0f / 255.0f);
}

HE_TEST(scribe, vector_image_pipeline, supports_smooth_path_commands)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgSmoothPathCommands), StrLen(kSvgSmoothPathCommands)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 2u);
    HE_EXPECT_EQ(imageData.layers.Size(), 2u);
    HE_EXPECT_GT(imageData.curveTexels.Size(), 0u);
    HE_EXPECT_GT(imageData.strokeCommands.Size(), 0u);
    HE_EXPECT_GT(imageData.shapes[0].strokeCommandCount, 0u);
    HE_EXPECT_GT(imageData.shapes[1].strokeCommandCount, 0u);
}

HE_TEST(scribe, vector_image_pipeline, emits_authored_stroke_layers)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithAuthoredStroke), StrLen(kSvgWithAuthoredStroke)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 2u);
    HE_EXPECT_EQ(imageData.layers.Size(), 3u);
    HE_EXPECT_EQ(imageData.layers[0].kind, VectorLayerKind::Stroke);
    HE_EXPECT_EQ(imageData.layers[0].strokeJoin, StrokeJoinKind::Round);
    HE_EXPECT_EQ(imageData.layers[0].strokeWidth, 6.0f);
    HE_EXPECT_EQ(imageData.layers[1].kind, VectorLayerKind::Stroke);
    HE_EXPECT_EQ(imageData.layers[2].kind, VectorLayerKind::Fill);
}

HE_TEST(scribe, vector_image_pipeline, skips_shapes_outside_simple_clip_paths)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithClipPath), StrLen(kSvgWithClipPath)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers[0].red, 17.0f / 255.0f);
}

HE_TEST(scribe, vector_image_pipeline, loads_compiled_vector_blob)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    const VectorImageRuntimeMetadata::Reader metadata = image.GetMetadata();
    const VectorImageFillData::Reader fill = image.GetFill();
    const VectorImagePaintData::Reader paint = image.GetPaint();

    HE_EXPECT(image.IsValid());
    HE_EXPECT(metadata.IsValid());
    HE_EXPECT(fill.IsValid());
    HE_EXPECT(paint.IsValid());
    HE_EXPECT_GT(image.GetStroke().GetCommands().Size(), 0u);
    HE_EXPECT_GT(image.GetStroke().GetPoints().Size(), 0u);
    HE_EXPECT_EQ(metadata.GetSourceViewBoxWidth(), 180.0f);
    HE_EXPECT_EQ(fill.GetShapes().Size(), 3u);
    HE_EXPECT_EQ(paint.GetLayers().Size(), 3u);
}

HE_TEST(scribe, vector_image_pipeline, resolves_layers_and_shape_resources)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    const schema::List<VectorImageLayer>::Reader layers = image.GetPaint().GetLayers();
    HE_EXPECT_EQ(layers.Size(), 3u);
    HE_EXPECT_EQ(layers[0].GetShapeIndex(), 0u);
    HE_EXPECT_EQ(layers[1].GetShapeIndex(), 1u);
    HE_EXPECT_GT(layers[0].GetAlpha(), 0.0f);

    CompiledVectorShapeResourceData shape{};
    HE_EXPECT(BuildCompiledVectorShapeResourceData(shape, image, 1));
    HE_EXPECT_EQ(shape.createInfo.vertexCount, ScribeGlyphVertexCount);
    HE_EXPECT_EQ(shape.createInfo.bandTexture.size.x, ScribeBandTextureWidth);
    HE_EXPECT_EQ(shape.shape.GetFillRule(), FillRule::EvenOdd);
    HE_EXPECT_EQ(shape.vertices[0].col.x, 1.0f);
    HE_EXPECT_EQ(shape.vertices[0].col.y, 1.0f);
    HE_EXPECT_EQ(shape.vertices[0].col.z, 1.0f);
    HE_EXPECT_EQ(shape.vertices[0].col.w, 1.0f);
}

HE_TEST(scribe, retained_vector_image, builds_layered_draws_from_runtime_resource)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    const VectorImageHandle handle = context.RegisterVectorImage(image);
    HE_ASSERT(handle.IsValid());
    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = handle;
    HE_ASSERT(retainedImage.Build(desc));

    HE_EXPECT_EQ(retainedImage.GetDrawCount(), image.GetPaint().GetLayers().Size());
    HE_EXPECT_EQ(retainedImage.GetEstimatedVertexCount(), retainedImage.GetDrawCount() * ScribeGlyphVertexCount);
    HE_EXPECT_EQ(retainedImage.GetViewBoxSize().x, 180.0f);
    HE_EXPECT_EQ(retainedImage.GetViewBoxSize().y, 180.0f);
    HE_EXPECT(retainedImage.GetImageHandle().IsValid());
}

HE_TEST(scribe, retained_vector_image, builds_runtime_stroke_draws_from_runtime_resource)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    const VectorImageHandle handle = context.RegisterVectorImage(image);
    HE_ASSERT(handle.IsValid());

    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = handle;
    desc.strokeColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    desc.strokeStyle.width = 6.0f;
    desc.strokeStyle.joinStyle = StrokeJoinStyle::Round;
    desc.strokeStyle.capStyle = StrokeCapStyle::Round;
    HE_ASSERT(retainedImage.Build(desc));

    HE_EXPECT_EQ(retainedImage.GetDrawCount(), image.GetPaint().GetLayers().Size() * 2u);
    uint32_t strokeDrawCount = 0;
    for (const RetainedVectorImageDraw& draw : retainedImage.GetDraws())
    {
        if ((draw.flags & RetainedVectorImageDrawFlagStroke) != 0)
        {
            ++strokeDrawCount;
            HE_EXPECT_EQ(draw.color.x, desc.strokeColor.x);
            HE_EXPECT_GT(draw.strokeStyle.width, 0.0f);
        }
    }

    HE_EXPECT_EQ(strokeDrawCount, image.GetPaint().GetLayers().Size());
}

HE_TEST(scribe, retained_vector_image, builds_authored_stroke_draws_from_runtime_resource)
{
    CompiledVectorImageData imageData{};
    HE_ASSERT(BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithAuthoredStroke), StrLen(kSvgWithAuthoredStroke)),
        0.25f));

    schema::Builder rootBuilder;
    VectorImageResource::Builder root = rootBuilder.AddStruct<VectorImageResource>();
    FillVectorImageResourceMetadata(root.GetMetadata(), imageData);
    FillVectorImageResourceFillData(root.GetFill(), imageData);
    FillVectorImageResourceStrokeData(root.GetStroke(), imageData);
    FillVectorImageResourcePaintData(root.GetPaint(), imageData);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
    rootBuilder.SetRoot(root);

    Vector<schema::Word> storage;
    storage = Span<const schema::Word>(rootBuilder);
    const VectorImageResourceReader image = schema::ReadRoot<VectorImageResource>(storage.Data());
    HE_ASSERT(image.IsValid());

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    const VectorImageHandle handle = context.RegisterVectorImage(image);
    HE_ASSERT(handle.IsValid());

    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = handle;
    HE_ASSERT(retainedImage.Build(desc));

    uint32_t authoredStrokeDrawCount = 0;
    uint32_t fillDrawCount = 0;
    for (const RetainedVectorImageDraw& draw : retainedImage.GetDraws())
    {
        if ((draw.flags & RetainedVectorImageDrawFlagStroke) != 0)
        {
            ++authoredStrokeDrawCount;
            HE_EXPECT_GT(draw.strokeStyle.width, 0.0f);
        }
        else
        {
            ++fillDrawCount;
        }
    }

    HE_EXPECT_EQ(authoredStrokeDrawCount, 2u);
    HE_EXPECT_EQ(fillDrawCount, 1u);
}

HE_TEST(scribe, retained_vector_image, prepares_with_renderer_after_temporary_image_copy_expires)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    HE_ASSERT(BuildRetainedVectorImageFromTemporaryCopy(retainedImage, context, image));
    HE_EXPECT_GT(retainedImage.GetDrawCount(), 0u);
    HE_EXPECT(retainedImage.GetImageHandle().IsValid());
    storage.Clear();

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());
    HE_EXPECT(harness.renderer.PrepareRetainedVectorImage(retainedImage));

    RetainedVectorImageInstanceDesc instance{};
    harness.renderer.QueueRetainedVectorImage(retainedImage, instance);
}

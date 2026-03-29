// Copyright Chad Engler

#include "image_compile_geometry.h"
#include "font_compile_geometry.h"
#include "font_import_utils.h"
#include "resource_build_utils.h"

#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/retained_vector_image.h"
#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

#include "he/core/test.h"
#include "he/core/file.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"

using namespace he;
using namespace he::scribe;
using namespace he::scribe::editor;

namespace
{
    assets::AssetUuid MakeTestAssetUuid()
    {
        assets::AssetUuid id{ Uuid::CreateV4() };
        while (id == assets::AssetUuid{})
        {
            id = assets::AssetUuid{ Uuid::CreateV4() };
        }

        return id;
    }

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

    constexpr const char* kSvgWithShapeAndUnresolvedText =
        "<svg viewBox=\"0 0 64 64\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#111827\" d=\"M4 4 L60 4 L60 60 L4 60 Z\"/>"
        "<text x=\"8\" y=\"20\" font-family=\"Missing Font\" font-size=\"18\">SVG</text>"
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

    constexpr const char* kSvgWithTextElement =
        "<svg viewBox=\"0 0 160 48\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text x=\"8\" y=\"30\" font-family=\"Noto Sans\" font-size=\"24\" fill=\"#111827\">SVG</text>"
        "</svg>";

    constexpr const char* kSvgWithTransformedTextElement =
        "<svg viewBox=\"0 0 100 100\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<g transform=\"translate(40 10)\">"
        "<text x=\"5\" y=\"30\" font-family=\"Noto Sans\" font-size=\"20\" fill=\"#111827\">A</text>"
        "</g>"
        "</svg>";

    constexpr const char* kSvgWithTspanPositionedText =
        "<svg viewBox=\"0 0 120 120\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text font-family=\"Noto Sans\" font-size=\"20\" fill=\"#111827\">"
        "<tspan x=\"40\" y=\"70\">Hello</tspan>"
        "</text>"
        "</svg>";

    constexpr const char* kSvgWithExplicitGlyphPositionText =
        "<svg viewBox=\"0 0 120 120\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text xml:space=\"preserve\" font-family=\"Noto Sans\" font-size=\"20\" fill=\"#111827\">"
        "<tspan x=\"12 32 64\" y=\"70\">A+B</tspan>"
        "</text>"
        "</svg>";

    constexpr const char* kSvgWithEncodedEntityText =
        "<svg viewBox=\"0 0 120 120\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text xml:space=\"preserve\" font-family=\"Times New Roman\" font-size=\"20\" fill=\"#111827\">"
        "<tspan x=\"12 32\">&#x2212;+</tspan>"
        "<tspan x=\"52\">&#x02c6;</tspan>"
        "<tspan x=\"72\">&#x00a9;</tspan>"
        "</text>"
        "</svg>";

    constexpr const char* kSvgWithPreservedLeadingWhitespaceText =
        "<svg viewBox=\"0 0 120 120\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text xml:space=\"preserve\" font-family=\"Noto Sans\" font-size=\"20\" fill=\"#111827\">"
        "<tspan x=\"12 24\"> A</tspan>"
        "</text>"
        "</svg>";

    constexpr const char* kSvgWithTransformedAuthoredStroke =
        "<svg viewBox=\"0 0 100 100\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<g transform=\"translate(40 20)\">"
        "<path fill=\"#22c55e\" stroke=\"#111827\" stroke-width=\"4\" d=\"M0 0 L20 0 L20 20 L0 20 Z\"/>"
        "</g>"
        "</svg>";

    constexpr const char* kSvgWithScaledSquareStroke =
        "<svg viewBox=\"0 0 32 32\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<g transform=\"translate(8 8) scale(0.5)\">"
        "<path fill=\"none\" stroke=\"#111827\" stroke-width=\"4\" stroke-linecap=\"square\" d=\"M0 0 L16 16\"/>"
        "</g>"
        "</svg>";

    constexpr const char* kSvgWithDiagonalButtStroke =
        "<svg viewBox=\"0 0 8 8\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"none\" stroke=\"#030404\" stroke-width=\"0.5\" stroke-linecap=\"butt\" d=\"M0 0 5.232 -5.465\"/>"
        "</svg>";

    constexpr const char* kSvgWithImplicitlyClosedFillPath =
        "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#e5e7eb\" d=\"M1 1 L15 1 L15 15 L1 15\"/>"
        "</svg>";

    constexpr const char* kSvgWithExplicitClosedQuadPath =
        "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#ef4444\" d=\"M1 1 L15 1 L15 15 L1 15 L1 1 z\"/>"
        "</svg>";

    constexpr const char* kSvgWithClosedQuadPolygon =
        "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<polygon fill=\"#ef4444\" points=\"1,1 15,1 15,15 1,15\"/>"
        "</svg>";

    constexpr const char* kSvgWithExplicitClosedSlantedQuadPath =
        "<svg viewBox=\"420 120 20 20\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"#ff2d00\" d=\"M428.044701 132.077347L432.841962 126.491452L426.940855 123.25305L422.149018 128.862084L428.044701 132.077347z\"/>"
        "</svg>";

    constexpr const char* kSvgWithClosedSlantedQuadPolygon =
        "<svg viewBox=\"420 120 20 20\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<polygon fill=\"#ff2d00\" points=\"428.044701,132.077347 432.841962,126.491452 426.940855,123.25305 422.149018,128.862084\"/>"
        "</svg>";

    constexpr const char* kSvgWithReferencedClosedSlantedQuadPath =
        "<svg viewBox=\"420 120 20 20\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
        "<defs>"
        "<path id=\"quad\" d=\"M428.044701 -299.922653L432.841962 -305.508548L426.940855 -308.746950L422.149018 -303.137916L428.044701 -299.922653z\"/>"
        "</defs>"
        "<use fill=\"#ff2d00\" xlink:href=\"#quad\" x=\"0\" y=\"432\"/>"
        "</svg>";

    constexpr const char* kSvgWithDashedStroke =
        "<svg viewBox=\"0 0 64 24\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<path fill=\"none\" stroke=\"#1475bc\" stroke-width=\"2\" stroke-dasharray=\"4,4\" d=\"M4 12 L60 12\"/>"
        "</svg>";

    constexpr const char* kSvgWithPostScriptTimesText =
        "<svg viewBox=\"0 0 64 32\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text x=\"8\" y=\"20\" font-family=\"CIZBRT+TimesNewRomanPS-BoldMT\" font-weight=\"bold\" font-size=\"18\">p</text>"
        "</svg>";

    constexpr const char* kSvgWithPostScriptArialText =
        "<svg viewBox=\"0 0 64 32\" xmlns=\"http://www.w3.org/2000/svg\">"
        "<text x=\"8\" y=\"20\" font-family=\"XLKSZD+ArialMT\" font-size=\"18\">x</text>"
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
        FillVectorImageResourceTextData(rootBuilder, root.GetText(), imageData);
        root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
        root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        out = schema::ReadRoot<VectorImageResource>(storage.Data());
        return out.IsValid();
    }

    bool BuildLoadedFontFace(
        Vector<schema::Word>& storage,
        FontFaceResourceReader& out,
        const char* fileName)
    {
        Vector<uint8_t> fontBytes{};
        if (!File::ReadAll(fontBytes, fileName))
        {
            return false;
        }

        FontFaceInfo faceInfo{};
        if (!InspectFontFace(fontBytes, 0, faceInfo))
        {
            return false;
        }

        CompiledFontRenderData renderData{};
        if (!BuildCompiledFontRenderData(renderData, fontBytes, 0))
        {
            return false;
        }

        schema::Builder rootBuilder;
        FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();
        Vector<uint8_t> shapingBytes{};
        if (!BuildFontFaceShapingBytes(shapingBytes, Span<const uint8_t>(fontBytes), faceInfo.faceIndex))
        {
            return false;
        }

        FontFaceShapingData::Builder shaping = root.GetShaping();
        shaping.SetFaceIndex(faceInfo.faceIndex);
        shaping.SetSourceBytes(rootBuilder.AddBlob(Span<const uint8_t>(shapingBytes)));
        FillFontFaceRuntimeMetadata(
            root.GetMetadata(),
            renderData.glyphs.Size(),
            faceInfo.unitsPerEm,
            faceInfo.ascender,
            faceInfo.descender,
            faceInfo.lineHeight,
            faceInfo.capHeight,
            faceInfo.hasColorGlyphs);
        FillFontFaceResourceFillData(root.GetFill(), renderData);
        FillFontFaceResourceStrokeData(root.GetStroke(), renderData);
        FillFontFaceResourcePaintData(root.GetPaint(), renderData.paint);
        root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
        root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        out = schema::ReadRoot<FontFaceResource>(storage.Data());
        return out.IsValid();
    }

    bool BuildRetainedVectorImageFromTemporaryCopy(
        RetainedVectorImageModel& out,
        ScribeContext& context,
        Span<const schema::Word> imageWords,
        const VectorImageResourceReader& image)
    {
        RetainedVectorImageBuildDesc desc{};
        desc.context = &context;
        desc.image = image;
        desc.imageWords = imageWords;
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
    HE_EXPECT_GT(imageData.textRuns.Size(), 0u);
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
    bool hasCubicCommand = false;
    for (const CompiledStrokeCommand& command : imageData.strokeCommands)
    {
        hasCubicCommand |= command.type == StrokeCommandType::CubicTo;
    }
    HE_EXPECT(hasCubicCommand);
}

HE_TEST(scribe, vector_image_pipeline, emits_authored_stroke_layers)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithAuthoredStroke), StrLen(kSvgWithAuthoredStroke)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 3u);
    HE_EXPECT_EQ(imageData.layers.Size(), 3u);
    HE_EXPECT_EQ(imageData.layers[0].kind, VectorLayerKind::Stroke);
    HE_EXPECT_EQ(imageData.layers[0].strokeJoin, StrokeJoinKind::Round);
    HE_EXPECT_EQ(imageData.layers[0].strokeWidth, 6.0f);
    HE_EXPECT_EQ(imageData.layers[1].kind, VectorLayerKind::Fill);
    HE_EXPECT_EQ(imageData.layers[2].kind, VectorLayerKind::Stroke);
    HE_EXPECT_NE(imageData.layers[0].shapeIndex, imageData.layers[1].shapeIndex);
    HE_EXPECT_NE(imageData.layers[2].shapeIndex, imageData.layers[1].shapeIndex);
}

HE_TEST(scribe, vector_image_pipeline, scales_authored_stroke_metrics_with_svg_transform)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithScaledSquareStroke), StrLen(kSvgWithScaledSquareStroke)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.layers.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers[0].kind, VectorLayerKind::Stroke);
    HE_EXPECT_GE(imageData.layers[0].strokeWidth, 1.999f);
    HE_EXPECT_LE(imageData.layers[0].strokeWidth, 2.001f);
}

HE_TEST(scribe, vector_image_pipeline, keeps_diagonal_butt_strokes_at_authored_length)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithDiagonalButtStroke), StrLen(kSvgWithDiagonalButtStroke)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 1u);

    const float dx = 5.232f;
    const float dy = -5.465f;
    const float length = Sqrt((dx * dx) + (dy * dy));
    const float halfWidth = 0.25f;
    const float nx = (-dy / length) * halfWidth;
    const float ny = (dx / length) * halfWidth;

    const float x0a = 0.0f + nx;
    const float x0b = 0.0f - nx;
    const float x1a = dx + nx;
    const float x1b = dx - nx;
    const float y0a = 0.0f + ny;
    const float y0b = 0.0f - ny;
    const float y1a = dy + ny;
    const float y1b = dy - ny;

    const float expectedMinX = Min(Min(x0a, x0b), Min(x1a, x1b));
    const float expectedMaxX = Max(Max(x0a, x0b), Max(x1a, x1b));
    const float expectedMinY = Min(Min(y0a, y0b), Min(y1a, y1b));
    const float expectedMaxY = Max(Max(y0a, y0b), Max(y1a, y1b));

    const CompiledVectorShapeRenderEntry& shape = imageData.shapes[0];
    const float actualMinX = shape.originX + shape.boundsMinX;
    const float actualMaxX = shape.originX + shape.boundsMaxX;
    const float actualMinY = shape.originY + shape.boundsMinY;
    const float actualMaxY = shape.originY + shape.boundsMaxY;

    HE_EXPECT_GE(actualMinX, expectedMinX - 0.001f);
    HE_EXPECT_LE(actualMinX, expectedMinX + 0.001f);
    HE_EXPECT_GE(actualMaxX, expectedMaxX - 0.001f);
    HE_EXPECT_LE(actualMaxX, expectedMaxX + 0.001f);
    HE_EXPECT_GE(actualMinY, expectedMinY - 0.001f);
    HE_EXPECT_LE(actualMinY, expectedMinY + 0.001f);
    HE_EXPECT_GE(actualMaxY, expectedMaxY - 0.001f);
    HE_EXPECT_LE(actualMaxY, expectedMaxY + 0.001f);
}

HE_TEST(scribe, vector_image_pipeline, implicitly_closes_open_fill_subpaths)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithImplicitlyClosedFillPath), StrLen(kSvgWithImplicitlyClosedFillPath)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers[0].kind, VectorLayerKind::Fill);
    HE_EXPECT_GE(imageData.shapes[0].boundsMaxX, 13.999f);
    HE_EXPECT_GE(imageData.shapes[0].boundsMaxY, 13.999f);
}

HE_TEST(scribe, vector_image_pipeline, compiles_explicit_closed_line_only_path_quads)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithExplicitClosedQuadPath), StrLen(kSvgWithExplicitClosedQuadPath)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers.Size(), 1u);
    HE_EXPECT_EQ(imageData.layers[0].kind, VectorLayerKind::Fill);
    HE_EXPECT_GE(imageData.shapes[0].boundsMaxX, 13.999f);
    HE_EXPECT_GE(imageData.shapes[0].boundsMaxY, 13.999f);
}

HE_TEST(scribe, vector_image_pipeline, axis_aligned_closed_quad_path_matches_polygon_shape)
{
    CompiledVectorImageData pathData{};
    CompiledVectorImageData polygonData{};

    const bool pathOk = BuildCompiledVectorImageData(
        pathData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithExplicitClosedQuadPath), StrLen(kSvgWithExplicitClosedQuadPath)),
        0.25f);
    const bool polygonOk = BuildCompiledVectorImageData(
        polygonData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithClosedQuadPolygon), StrLen(kSvgWithClosedQuadPolygon)),
        0.25f);

    HE_EXPECT(pathOk);
    HE_EXPECT(polygonOk);
    HE_EXPECT_EQ(pathData.shapes.Size(), 1u);
    HE_EXPECT_EQ(polygonData.shapes.Size(), 1u);
    HE_EXPECT_EQ(pathData.curveTexels.Size(), polygonData.curveTexels.Size());

    for (uint32_t texelIndex = 0; texelIndex < pathData.curveTexels.Size(); ++texelIndex)
    {
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].x, polygonData.curveTexels[texelIndex].x);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].y, polygonData.curveTexels[texelIndex].y);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].z, polygonData.curveTexels[texelIndex].z);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].w, polygonData.curveTexels[texelIndex].w);
    }
}

HE_TEST(scribe, vector_image_pipeline, closed_slanted_quad_paths_match_polygon_and_use_shapes)
{
    CompiledVectorImageData pathData{};
    CompiledVectorImageData polygonData{};
    CompiledVectorImageData useData{};

    const bool pathOk = BuildCompiledVectorImageData(
        pathData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithExplicitClosedSlantedQuadPath), StrLen(kSvgWithExplicitClosedSlantedQuadPath)),
        0.25f);
    const bool polygonOk = BuildCompiledVectorImageData(
        polygonData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithClosedSlantedQuadPolygon), StrLen(kSvgWithClosedSlantedQuadPolygon)),
        0.25f);
    const bool useOk = BuildCompiledVectorImageData(
        useData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithReferencedClosedSlantedQuadPath), StrLen(kSvgWithReferencedClosedSlantedQuadPath)),
        0.25f);

    HE_EXPECT(pathOk);
    HE_EXPECT(polygonOk);
    HE_EXPECT(useOk);
    HE_EXPECT_EQ(pathData.shapes.Size(), 1u);
    HE_EXPECT_EQ(polygonData.shapes.Size(), 1u);
    HE_EXPECT_EQ(useData.shapes.Size(), 1u);
    HE_EXPECT_EQ(pathData.curveTexels.Size(), polygonData.curveTexels.Size());
    HE_EXPECT_EQ(pathData.curveTexels.Size(), useData.curveTexels.Size());

    for (uint32_t texelIndex = 0; texelIndex < pathData.curveTexels.Size(); ++texelIndex)
    {
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].x, polygonData.curveTexels[texelIndex].x);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].y, polygonData.curveTexels[texelIndex].y);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].z, polygonData.curveTexels[texelIndex].z);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].w, polygonData.curveTexels[texelIndex].w);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].x, useData.curveTexels[texelIndex].x);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].y, useData.curveTexels[texelIndex].y);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].z, useData.curveTexels[texelIndex].z);
        HE_EXPECT_EQ(pathData.curveTexels[texelIndex].w, useData.curveTexels[texelIndex].w);
    }

    HE_EXPECT_EQ(pathData.shapes[0].bandMaxX, 0u);
    HE_EXPECT_EQ(pathData.shapes[0].bandMaxY, 0u);
    HE_EXPECT_EQ(polygonData.shapes[0].bandMaxX, 0u);
    HE_EXPECT_EQ(polygonData.shapes[0].bandMaxY, 0u);
    HE_EXPECT_EQ(useData.shapes[0].bandMaxX, 0u);
    HE_EXPECT_EQ(useData.shapes[0].bandMaxY, 0u);
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

HE_TEST(scribe, vector_image_pipeline, compiles_svg_text_elements_to_geometry)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithTextElement), StrLen(kSvgWithTextElement)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.shapes.Size(), 0u);
    HE_EXPECT_EQ(imageData.layers.Size(), 0u);
    HE_EXPECT_GT(imageData.textRuns.Size(), 0u);
    HE_EXPECT_EQ(imageData.fontFaces.Size(), 1u);
}

HE_TEST(scribe, vector_image_pipeline, applies_group_transform_to_svg_text_geometry)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithTransformedTextElement), StrLen(kSvgWithTransformedTextElement)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_GT(imageData.textRuns.Size(), 0u);
    HE_EXPECT_GT(imageData.textRuns[0].transformTranslation.x, 35.0f);
}

HE_TEST(scribe, vector_image_pipeline, uses_tspan_position_for_svg_text_geometry)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithTspanPositionedText), StrLen(kSvgWithTspanPositionedText)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_GT(imageData.textRuns.Size(), 0u);
    HE_EXPECT_GT(imageData.textRuns[0].position.x, 35.0f);
}

HE_TEST(scribe, vector_image_pipeline, splits_svg_text_into_explicit_glyph_runs_when_x_positions_are_per_glyph)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithExplicitGlyphPositionText), StrLen(kSvgWithExplicitGlyphPositionText)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.textRuns.Size(), 3u);
    HE_EXPECT_EQ(StringView(imageData.textRuns[0].text.Data(), imageData.textRuns[0].text.Size()), "A");
    HE_EXPECT_EQ(StringView(imageData.textRuns[1].text.Data(), imageData.textRuns[1].text.Size()), "+");
    HE_EXPECT_EQ(StringView(imageData.textRuns[2].text.Data(), imageData.textRuns[2].text.Size()), "B");
    HE_EXPECT_EQ(imageData.textRuns[0].position.x, 12.0f);
    HE_EXPECT_EQ(imageData.textRuns[1].position.x, 32.0f);
    HE_EXPECT_EQ(imageData.textRuns[2].position.x, 64.0f);
    HE_EXPECT(imageData.textRuns[0].positionUsesGlyphOriginX);
    HE_EXPECT(imageData.textRuns[1].positionUsesGlyphOriginX);
    HE_EXPECT(imageData.textRuns[2].positionUsesGlyphOriginX);
}

HE_TEST(scribe, vector_image_pipeline, decodes_svg_text_entities_before_emitting_text_runs)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithEncodedEntityText), StrLen(kSvgWithEncodedEntityText)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.textRuns.Size(), 4u);
    HE_EXPECT_EQ(StringView(imageData.textRuns[0].text.Data(), imageData.textRuns[0].text.Size()), StringView("\xE2\x88\x92", 3));
    HE_EXPECT_EQ(StringView(imageData.textRuns[1].text.Data(), imageData.textRuns[1].text.Size()), "+");
    HE_EXPECT_EQ(StringView(imageData.textRuns[2].text.Data(), imageData.textRuns[2].text.Size()), StringView("\xCB\x86", 2));
    HE_EXPECT_EQ(StringView(imageData.textRuns[3].text.Data(), imageData.textRuns[3].text.Size()), StringView("\xC2\xA9", 2));
}

HE_TEST(scribe, vector_image_pipeline, preserves_svg_whitespace_when_xml_space_is_preserve)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithPreservedLeadingWhitespaceText), StrLen(kSvgWithPreservedLeadingWhitespaceText)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.textRuns.Size(), 2u);
    HE_EXPECT_EQ(StringView(imageData.textRuns[0].text.Data(), imageData.textRuns[0].text.Size()), " ");
    HE_EXPECT_EQ(StringView(imageData.textRuns[1].text.Data(), imageData.textRuns[1].text.Size()), "A");
}

HE_TEST(scribe, vector_image_pipeline, compiles_svg_text_with_postscript_font_names)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithPostScriptTimesText), StrLen(kSvgWithPostScriptTimesText)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_GT(imageData.textRuns.Size(), 0u);
    HE_EXPECT_EQ(imageData.fontFaces.Size(), 1u);
    HE_EXPECT_EQ(StringView(imageData.fontFaces[0].key.Data(), imageData.fontFaces[0].key.Size()), "TimesNewRomanPS-BoldMT");
}

HE_TEST(scribe, vector_image_pipeline, compiles_svg_text_with_postscript_sans_font_names)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithPostScriptArialText), StrLen(kSvgWithPostScriptArialText)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_GT(imageData.textRuns.Size(), 0u);
    HE_EXPECT_EQ(imageData.fontFaces.Size(), 1u);
    HE_EXPECT_EQ(StringView(imageData.fontFaces[0].key.Data(), imageData.fontFaces[0].key.Size()), "ArialMT");
}

HE_TEST(scribe, vector_image_pipeline, applies_group_transform_to_authored_path_strokes)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithTransformedAuthoredStroke), StrLen(kSvgWithTransformedAuthoredStroke)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_EQ(imageData.layers.Size(), 2u);
    HE_EXPECT_EQ(imageData.layers[0].kind, VectorLayerKind::Fill);
    HE_EXPECT_EQ(imageData.layers[1].kind, VectorLayerKind::Stroke);

    const CompiledVectorShapeRenderEntry& fillShape = imageData.shapes[imageData.layers[0].shapeIndex];
    const CompiledVectorShapeRenderEntry& strokeShape = imageData.shapes[imageData.layers[1].shapeIndex];
    HE_EXPECT_GT(strokeShape.originX, 35.0f);
    HE_EXPECT_GT(fillShape.originX, 35.0f);
    HE_EXPECT_GE(strokeShape.boundsMinX, 0.0f);
    HE_EXPECT_GE(fillShape.boundsMinX, 0.0f);
}

HE_TEST(scribe, vector_image_pipeline, compiles_dashed_authored_strokes)
{
    CompiledVectorImageData imageData{};
    const bool ok = BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithDashedStroke), StrLen(kSvgWithDashedStroke)),
        0.25f);

    HE_EXPECT(ok);
    HE_EXPECT_GT(imageData.layers.Size(), 1u);
    for (const CompiledVectorImageLayerEntry& layer : imageData.layers)
    {
        HE_EXPECT_EQ(layer.kind, VectorLayerKind::Stroke);
    }
    HE_EXPECT_GT(imageData.curveTexels.Size(), 16u);
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
    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));

    HE_EXPECT_EQ(retainedImage.GetDrawCount(), image.GetPaint().GetLayers().Size());
    HE_EXPECT_EQ(retainedImage.GetEstimatedVertexCount(), retainedImage.GetDrawCount() * ScribeGlyphVertexCount);
    HE_EXPECT_EQ(retainedImage.GetViewBoxSize().x, 180.0f);
    HE_EXPECT_EQ(retainedImage.GetViewBoxSize().y, 180.0f);
    HE_EXPECT(retainedImage.GetImage().IsValid());
}

HE_TEST(scribe, retained_vector_image, builds_runtime_stroke_draws_from_runtime_resource)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = image;
    desc.imageWords = storage;
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
            HE_EXPECT((draw.flags & RetainedVectorImageDrawFlagRuntimeRestroke) != 0);
        }
        else
        {
            HE_EXPECT((draw.flags & RetainedVectorImageDrawFlagRuntimeRestroke) == 0);
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
    FillVectorImageResourceTextData(rootBuilder, root.GetText(), imageData);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
    rootBuilder.SetRoot(root);

    Vector<schema::Word> storage;
    storage = Span<const schema::Word>(rootBuilder);
    const VectorImageResourceReader image = schema::ReadRoot<VectorImageResource>(storage.Data());
    HE_ASSERT(image.IsValid());

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));

    uint32_t authoredStrokeDrawCount = 0;
    uint32_t fillDrawCount = 0;
    for (const RetainedVectorImageDraw& draw : retainedImage.GetDraws())
    {
        if ((draw.flags & RetainedVectorImageDrawFlagStroke) != 0)
        {
            ++authoredStrokeDrawCount;
            HE_EXPECT((draw.flags & RetainedVectorImageDrawFlagRuntimeRestroke) == 0);
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

HE_TEST(scribe, retained_vector_image, builds_draws_from_svg_text_elements)
{
    CompiledVectorImageData imageData{};
    HE_ASSERT(BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithTextElement), StrLen(kSvgWithTextElement)),
        0.25f));

    schema::Builder rootBuilder;
    VectorImageResource::Builder root = rootBuilder.AddStruct<VectorImageResource>();
    FillVectorImageResourceMetadata(root.GetMetadata(), imageData);
    FillVectorImageResourceFillData(root.GetFill(), imageData);
    FillVectorImageResourceStrokeData(root.GetStroke(), imageData);
    FillVectorImageResourcePaintData(root.GetPaint(), imageData);
    FillVectorImageResourceTextData(rootBuilder, root.GetText(), imageData);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
    rootBuilder.SetRoot(root);

    Vector<schema::Word> storage;
    storage = Span<const schema::Word>(rootBuilder);
    const VectorImageResourceReader image = schema::ReadRoot<VectorImageResource>(storage.Data());
    HE_ASSERT(image.IsValid());
    HE_EXPECT_EQ(image.GetText().GetFontFaces().Size(), 1u);
    HE_EXPECT_EQ(image.GetText().GetFontFaces()[0].AsView(), "Noto Sans");
    HE_EXPECT_GT(image.GetText().GetRuns().Size(), 0u);

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    Vector<schema::Word> fontStorage{};
    FontFaceResourceReader fontFace{};
    HE_ASSERT(BuildLoadedFontFace(
        fontStorage,
        fontFace,
        "C:/Users/engle/source/repos/harvest-engine/plugins/editor/src/fonts/NotoSans-Regular.ttf"));
    const FontFaceHandle fontHandle = context.RegisterFontFace(Move(fontStorage), MakeTestAssetUuid(), "Noto Sans");
    HE_ASSERT(fontHandle.IsValid());
    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));
    HE_EXPECT_GT(retainedImage.GetDrawCount(), 0u);
    bool foundOffsetTextDraw = false;
    HE_EXPECT_GT(retainedImage.GetTextDrawCount(), 0u);
    for (const RetainedTextDraw& draw : retainedImage.GetTextDraws())
    {
        if (draw.position.x > 5.0f)
        {
            foundOffsetTextDraw = true;
            break;
        }
    }
    HE_EXPECT(foundOffsetTextDraw);
}

HE_TEST(scribe, retained_vector_image, falls_back_to_other_registered_svg_font_faces_for_missing_glyphs)
{
    CompiledVectorImageData imageData{};
    imageData.viewBoxWidth = 64.0f;
    imageData.viewBoxHeight = 32.0f;
    imageData.boundsMaxX = 64.0f;
    imageData.boundsMaxY = 32.0f;
    imageData.fontFaces.Resize(2);
    imageData.fontFaces[0].key = "Material Design Icons";
    imageData.fontFaces[1].key = "ArialMT";
    CompiledVectorImageTextRunEntry& run = imageData.textRuns.EmplaceBack();
    run.fontFaceIndex = 0;
    run.text = StringView("\xE2\x88\x92", 3);
    run.position = { 12.0f, 20.0f };
    run.fontSize = 18.0f;
    run.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    schema::Builder rootBuilder;
    VectorImageResource::Builder root = rootBuilder.AddStruct<VectorImageResource>();
    FillVectorImageResourceMetadata(root.GetMetadata(), imageData);
    FillVectorImageResourceFillData(root.GetFill(), imageData);
    FillVectorImageResourceStrokeData(root.GetStroke(), imageData);
    FillVectorImageResourcePaintData(root.GetPaint(), imageData);
    FillVectorImageResourceTextData(rootBuilder, root.GetText(), imageData);
    rootBuilder.SetRoot(root);

    Vector<schema::Word> storage;
    storage = Span<const schema::Word>(rootBuilder);
    const VectorImageResourceReader image = schema::ReadRoot<VectorImageResource>(storage.Data());
    HE_ASSERT(image.IsValid());

    ScribeContext context{};
    Vector<schema::Word> iconFontStorage{};
    FontFaceResourceReader iconFontFace{};
    HE_ASSERT(BuildLoadedFontFace(
        iconFontStorage,
        iconFontFace,
        "C:/Users/engle/source/repos/harvest-engine/plugins/editor/src/fonts/materialdesignicons.ttf"));
    HE_ASSERT(context.RegisterFontFace(Move(iconFontStorage), MakeTestAssetUuid(), "Material Design Icons").IsValid());

    Vector<schema::Word> textFontStorage{};
    FontFaceResourceReader textFontFace{};
    HE_ASSERT(BuildLoadedFontFace(
        textFontStorage,
        textFontFace,
        "C:/Windows/Fonts/arial.ttf"));
    HE_ASSERT(context.RegisterFontFace(Move(textFontStorage), MakeTestAssetUuid(), "ArialMT").IsValid());

    RetainedVectorImageModel retainedImage{};
    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));

    HE_EXPECT_GT(retainedImage.GetTextDrawCount(), 0u);
    HE_EXPECT_EQ(retainedImage.GetTextDraws()[0].fontFaceIndex, 1u);
}

HE_TEST(scribe, retained_vector_image, skips_unresolved_svg_text_runs_without_failing_shape_draws)
{
    CompiledVectorImageData imageData{};
    HE_ASSERT(BuildCompiledVectorImageData(
        imageData,
        Span(reinterpret_cast<const uint8_t*>(kSvgWithShapeAndUnresolvedText), StrLen(kSvgWithShapeAndUnresolvedText)),
        0.25f));

    schema::Builder rootBuilder;
    VectorImageResource::Builder root = rootBuilder.AddStruct<VectorImageResource>();
    FillVectorImageResourceMetadata(root.GetMetadata(), imageData);
    FillVectorImageResourceFillData(root.GetFill(), imageData);
    FillVectorImageResourceStrokeData(root.GetStroke(), imageData);
    FillVectorImageResourcePaintData(root.GetPaint(), imageData);
    FillVectorImageResourceTextData(rootBuilder, root.GetText(), imageData);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
    rootBuilder.SetRoot(root);

    Vector<schema::Word> storage;
    storage = Span<const schema::Word>(rootBuilder);
    const VectorImageResourceReader image = schema::ReadRoot<VectorImageResource>(storage.Data());
    HE_ASSERT(image.IsValid());

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};

    RetainedVectorImageBuildDesc desc{};
    desc.context = &context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));
    HE_EXPECT_EQ(retainedImage.GetShapeDrawCount(), 1u);
    HE_EXPECT_EQ(retainedImage.GetTextDrawCount(), 0u);
}

HE_TEST(scribe, retained_vector_image, prepares_with_renderer_after_temporary_image_copy_expires)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    RetainedVectorImageModel retainedImage;
    ScribeContext context{};
    HE_ASSERT(BuildRetainedVectorImageFromTemporaryCopy(retainedImage, context, storage, image));
    HE_EXPECT_GT(retainedImage.GetDrawCount(), 0u);
    HE_EXPECT(retainedImage.GetImage().IsValid());
    storage.Clear();

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());
    harness.renderer.DrawImage(retainedImage);
}

HE_TEST(scribe, retained_vector_image, shares_one_fill_atlas_across_prepared_shapes)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());

    RetainedVectorImageModel retainedImage;
    RetainedVectorImageBuildDesc desc{};
    desc.context = &harness.context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));

    const GlyphResource* shape0 = nullptr;
    const GlyphResource* shape1 = nullptr;
    HE_ASSERT(retainedImage.TryGetPreparedShapeResource(
        0,
        RetainedVectorImageShapeResourceKind::CompiledShape,
        {},
        harness.renderer,
        shape0));
    HE_ASSERT(retainedImage.TryGetPreparedShapeResource(
        1,
        RetainedVectorImageShapeResourceKind::CompiledShape,
        {},
        harness.renderer,
        shape1));
    HE_ASSERT(shape0 != nullptr);
    HE_ASSERT(shape1 != nullptr);
    HE_EXPECT(shape0->atlas != nullptr);
    HE_EXPECT_EQ_PTR(shape0->atlas, shape1->atlas);
}

HE_TEST(scribe, retained_vector_image, transformed_vertex_cache_is_reused_for_same_instance)
{
    Vector<schema::Word> storage;
    VectorImageResourceReader image{};
    HE_ASSERT(BuildLoadedVectorImage(storage, image));

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());

    RetainedVectorImageModel retainedImage{};
    RetainedVectorImageBuildDesc desc{};
    desc.context = &harness.context;
    desc.image = image;
    desc.imageWords = storage;
    HE_ASSERT(retainedImage.Build(desc));

    retainedImage.SetOrigin({ 16.0f, 18.0f });
    retainedImage.SetScale(1.25f);
    retainedImage.SetTint({ 0.8f, 0.7f, 0.6f, 1.0f });

    harness.renderer.DrawImage(retainedImage);
    HE_EXPECT_GT(retainedImage.GetCachedVertexCount(), 0u);
    HE_EXPECT_GT(retainedImage.GetCachedBatchCount(), 0u);
    const uint32_t cacheGeneration = retainedImage.GetGeometryCacheGeneration();

    harness.renderer.DrawImage(retainedImage);
    HE_EXPECT_EQ(retainedImage.GetGeometryCacheGeneration(), cacheGeneration);

    retainedImage.SetScale(0.75f);
    HE_EXPECT_EQ(retainedImage.GetCachedVertexCount(), 0u);
    HE_EXPECT_EQ(retainedImage.GetCachedBatchCount(), 0u);
    harness.renderer.DrawImage(retainedImage);
    HE_EXPECT_GT(retainedImage.GetCachedVertexCount(), 0u);
    HE_EXPECT_GT(retainedImage.GetCachedBatchCount(), 0u);
    HE_EXPECT_GT(retainedImage.GetGeometryCacheGeneration(), cacheGeneration);
}

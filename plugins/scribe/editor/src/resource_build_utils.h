// Copyright Chad Engler

#pragma once

#include "font_compile_geometry.h"
#include "image_compile_geometry.h"

namespace he::scribe::editor
{
    template <typename TBuilder, typename TEntry>
    void FillCommonShapeRenderData(TBuilder dstShape, const TEntry& srcShape)
    {
        dstShape.SetBoundsMinX(srcShape.boundsMinX);
        dstShape.SetBoundsMinY(srcShape.boundsMinY);
        dstShape.SetBoundsMaxX(srcShape.boundsMaxX);
        dstShape.SetBoundsMaxY(srcShape.boundsMaxY);
        if constexpr (requires { dstShape.SetOriginX(srcShape.originX); dstShape.SetOriginY(srcShape.originY); })
        {
            dstShape.SetOriginX(srcShape.originX);
            dstShape.SetOriginY(srcShape.originY);
        }
        dstShape.SetBandScaleX(srcShape.bandScaleX);
        dstShape.SetBandScaleY(srcShape.bandScaleY);
        dstShape.SetBandOffsetX(srcShape.bandOffsetX);
        dstShape.SetBandOffsetY(srcShape.bandOffsetY);
        dstShape.SetGlyphBandLocX(srcShape.glyphBandLocX);
        dstShape.SetGlyphBandLocY(srcShape.glyphBandLocY);
        dstShape.SetBandMaxX(srcShape.bandMaxX);
        dstShape.SetBandMaxY(srcShape.bandMaxY);
        dstShape.SetFillRule(srcShape.fillRule);
        dstShape.SetFirstStrokeCommand(srcShape.firstStrokeCommand);
        dstShape.SetStrokeCommandCount(srcShape.strokeCommandCount);
    }

    template <typename TBuilder>
    void FillCompiledStrokeData(
        TBuilder stroke,
        Span<const CompiledStrokePoint> points,
        Span<const CompiledStrokeCommand> commands)
    {
        stroke.SetPointScale(CompiledStrokePointScale);

        schema::List<StrokePoint>::Builder dstPoints = stroke.InitPoints(points.Size());
        for (uint32_t pointIndex = 0; pointIndex < points.Size(); ++pointIndex)
        {
            dstPoints[pointIndex].SetX(points[pointIndex].x);
            dstPoints[pointIndex].SetY(points[pointIndex].y);
        }

        schema::List<StrokeCommand>::Builder dstCommands = stroke.InitCommands(commands.Size());
        for (uint32_t commandIndex = 0; commandIndex < commands.Size(); ++commandIndex)
        {
            dstCommands[commandIndex].SetType(commands[commandIndex].type);
            dstCommands[commandIndex].SetFirstPoint(commands[commandIndex].firstPoint);
        }
    }

    inline void FillFontFaceRuntimeMetadata(
        FontFaceRuntimeMetadata::Builder metadata,
        uint32_t glyphCount,
        uint32_t unitsPerEm,
        int32_t ascender,
        int32_t descender,
        int32_t lineHeight,
        int32_t capHeight,
        bool hasColorGlyphs)
    {
        metadata.SetGlyphCount(glyphCount);
        metadata.SetUnitsPerEm(unitsPerEm);
        metadata.SetAscender(ascender);
        metadata.SetDescender(descender);
        metadata.SetLineHeight(lineHeight);
        metadata.SetCapHeight(capHeight);
        metadata.SetHasColorGlyphs(hasColorGlyphs);
    }

    inline void FillFontFaceResourceFillData(FontFaceFillData::Builder fill, const CompiledFontRenderData& renderData)
    {
        fill.SetCurveTextureWidth(renderData.curveTextureWidth);
        fill.SetCurveTextureHeight(renderData.curveTextureHeight);
        fill.SetBandTextureWidth(renderData.bandTextureWidth);
        fill.SetBandTextureHeight(renderData.bandTextureHeight);
        fill.SetBandOverlapEpsilon(renderData.bandOverlapEpsilon);

        schema::List<FontFaceGlyphRenderData>::Builder glyphs = fill.InitGlyphs(renderData.glyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < renderData.glyphs.Size(); ++glyphIndex)
        {
            const CompiledGlyphRenderEntry& srcGlyph = renderData.glyphs[glyphIndex];
            FontFaceGlyphRenderData::Builder dstGlyph = glyphs[glyphIndex];
            FillCommonShapeRenderData(dstGlyph, srcGlyph);
            dstGlyph.SetAdvanceX(srcGlyph.advanceX);
            dstGlyph.SetAdvanceY(srcGlyph.advanceY);
            dstGlyph.SetHasGeometry(srcGlyph.hasGeometry);
            dstGlyph.SetHasColorLayers(srcGlyph.hasColorLayers);
        }
    }

    inline void FillFontFaceResourceStrokeData(FontFaceStrokeData::Builder stroke, const CompiledFontRenderData& renderData)
    {
        FillCompiledStrokeData(
            stroke,
            Span<const CompiledStrokePoint>(renderData.strokePoints.Data(), renderData.strokePoints.Size()),
            Span<const CompiledStrokeCommand>(renderData.strokeCommands.Data(), renderData.strokeCommands.Size()));
    }

    inline void FillFontFaceResourcePaintData(FontFacePaintData::Builder paint, const CompiledFontPaintData& paintData)
    {
        paint.SetDefaultPaletteIndex(paintData.defaultPaletteIndex);

        schema::List<FontFacePalette>::Builder palettes = paint.InitPalettes(paintData.palettes.Size());
        for (uint32_t paletteIndex = 0; paletteIndex < paintData.palettes.Size(); ++paletteIndex)
        {
            const CompiledFontPalette& srcPalette = paintData.palettes[paletteIndex];
            FontFacePalette::Builder dstPalette = palettes[paletteIndex];
            dstPalette.SetBackground(srcPalette.background);

            schema::List<FontFacePaletteColor>::Builder colors = dstPalette.InitColors(srcPalette.colors.Size());
            for (uint32_t colorIndex = 0; colorIndex < srcPalette.colors.Size(); ++colorIndex)
            {
                const CompiledFontPaletteColor& srcColor = srcPalette.colors[colorIndex];
                FontFacePaletteColor::Builder dstColor = colors[colorIndex];
                dstColor.SetRed(srcColor.red);
                dstColor.SetGreen(srcColor.green);
                dstColor.SetBlue(srcColor.blue);
                dstColor.SetAlpha(srcColor.alpha);
            }
        }

        schema::List<FontFaceColorGlyph>::Builder colorGlyphs = paint.InitColorGlyphs(paintData.colorGlyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < paintData.colorGlyphs.Size(); ++glyphIndex)
        {
            const CompiledColorGlyphEntry& srcColorGlyph = paintData.colorGlyphs[glyphIndex];
            FontFaceColorGlyph::Builder dstColorGlyph = colorGlyphs[glyphIndex];
            dstColorGlyph.SetFirstLayer(srcColorGlyph.firstLayer);
            dstColorGlyph.SetLayerCount(srcColorGlyph.layerCount);
        }

        schema::List<FontFaceColorGlyphLayer>::Builder layers = paint.InitLayers(paintData.layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < paintData.layers.Size(); ++layerIndex)
        {
            const CompiledColorGlyphLayerEntry& srcLayer = paintData.layers[layerIndex];
            FontFaceColorGlyphLayer::Builder dstLayer = layers[layerIndex];
            dstLayer.SetGlyphIndex(srcLayer.glyphIndex);
            dstLayer.SetPaletteEntryIndex(srcLayer.paletteEntryIndex);
            dstLayer.SetColorSource(srcLayer.colorSource);
            dstLayer.SetAlphaScale(srcLayer.alphaScale);
            dstLayer.SetTransform00(srcLayer.transform00);
            dstLayer.SetTransform01(srcLayer.transform01);
            dstLayer.SetTransform10(srcLayer.transform10);
            dstLayer.SetTransform11(srcLayer.transform11);
            dstLayer.SetTransformTx(srcLayer.transformTx);
            dstLayer.SetTransformTy(srcLayer.transformTy);
        }
    }

    inline void FillVectorImageResourceMetadata(VectorImageRuntimeMetadata::Builder metadata, const CompiledVectorImageData& imageData)
    {
        metadata.SetSourceViewBoxMinX(imageData.viewBoxMinX);
        metadata.SetSourceViewBoxMinY(imageData.viewBoxMinY);
        metadata.SetSourceViewBoxWidth(imageData.viewBoxWidth);
        metadata.SetSourceViewBoxHeight(imageData.viewBoxHeight);
        metadata.SetSourceBoundsMinX(imageData.boundsMinX);
        metadata.SetSourceBoundsMinY(imageData.boundsMinY);
        metadata.SetSourceBoundsMaxX(imageData.boundsMaxX);
        metadata.SetSourceBoundsMaxY(imageData.boundsMaxY);
    }

    inline void FillVectorImageResourceFillData(VectorImageFillData::Builder fill, const CompiledVectorImageData& imageData)
    {
        fill.SetCurveTextureWidth(imageData.curveTextureWidth);
        fill.SetCurveTextureHeight(imageData.curveTextureHeight);
        fill.SetBandTextureWidth(imageData.bandTextureWidth);
        fill.SetBandTextureHeight(imageData.bandTextureHeight);
        fill.SetBandOverlapEpsilon(imageData.bandOverlapEpsilon);

        schema::List<VectorImageShapeRenderData>::Builder shapes = fill.InitShapes(imageData.shapes.Size());
        for (uint32_t shapeIndex = 0; shapeIndex < imageData.shapes.Size(); ++shapeIndex)
        {
            const CompiledVectorShapeRenderEntry& srcShape = imageData.shapes[shapeIndex];
            VectorImageShapeRenderData::Builder dstShape = shapes[shapeIndex];
            FillCommonShapeRenderData(dstShape, srcShape);
        }
    }

    inline void FillVectorImageResourceStrokeData(VectorImageStrokeData::Builder stroke, const CompiledVectorImageData& imageData)
    {
        FillCompiledStrokeData(
            stroke,
            Span<const CompiledStrokePoint>(imageData.strokePoints.Data(), imageData.strokePoints.Size()),
            Span<const CompiledStrokeCommand>(imageData.strokeCommands.Data(), imageData.strokeCommands.Size()));
    }

    inline void FillVectorImageResourcePaintData(VectorImagePaintData::Builder paint, const CompiledVectorImageData& imageData)
    {
        schema::List<VectorImageLayer>::Builder layers = paint.InitLayers(imageData.layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < imageData.layers.Size(); ++layerIndex)
        {
            const CompiledVectorImageLayerEntry& srcLayer = imageData.layers[layerIndex];
            VectorImageLayer::Builder dstLayer = layers[layerIndex];
            dstLayer.SetShapeIndex(srcLayer.shapeIndex);
            dstLayer.SetKind(srcLayer.kind);
            dstLayer.SetRed(srcLayer.red);
            dstLayer.SetGreen(srcLayer.green);
            dstLayer.SetBlue(srcLayer.blue);
            dstLayer.SetAlpha(srcLayer.alpha);
            dstLayer.SetStrokeWidth(srcLayer.strokeWidth);
            dstLayer.SetStrokeJoin(srcLayer.strokeJoin);
            dstLayer.SetStrokeCap(srcLayer.strokeCap);
            dstLayer.SetStrokeMiterLimit(srcLayer.strokeMiterLimit);
        }
    }

    inline void FillVectorImageResourceTextData(
        schema::Builder& rootBuilder,
        ScribeImage::RuntimeResource::Text::Builder text,
        const CompiledVectorImageData& imageData)
    {
        schema::List<schema::String>::Builder fontFaces = text.InitFontFaces(imageData.fontFaces.Size());
        for (uint32_t fontIndex = 0; fontIndex < imageData.fontFaces.Size(); ++fontIndex)
        {
            const CompiledVectorImageFontFaceEntry& srcFont = imageData.fontFaces[fontIndex];
            fontFaces.Set(fontIndex, rootBuilder.AddString(StringView(srcFont.key.Data(), srcFont.key.Size())));
        }

        schema::List<ScribeImage::TextRun>::Builder runs = text.InitRuns(imageData.textRuns.Size());
        for (uint32_t runIndex = 0; runIndex < imageData.textRuns.Size(); ++runIndex)
        {
            const CompiledVectorImageTextRunEntry& srcRun = imageData.textRuns[runIndex];
            ScribeImage::TextRun::Builder dstRun = runs[runIndex];
            dstRun.SetFontFaceIndex(srcRun.fontFaceIndex);
            dstRun.SetText(rootBuilder.AddString(StringView(srcRun.text.Data(), srcRun.text.Size())));
            dstRun.SetPositionX(srcRun.position.x);
            dstRun.SetPositionY(srcRun.position.y);
            dstRun.SetFontSize(srcRun.fontSize);
            dstRun.SetTransformXX(srcRun.transformX.x);
            dstRun.SetTransformXY(srcRun.transformX.y);
            dstRun.SetTransformYX(srcRun.transformY.x);
            dstRun.SetTransformYY(srcRun.transformY.y);
            dstRun.SetTransformTx(srcRun.transformTranslation.x);
            dstRun.SetTransformTy(srcRun.transformTranslation.y);
            dstRun.SetAnchor(srcRun.anchor);
            dstRun.SetRed(srcRun.color.x);
            dstRun.SetGreen(srcRun.color.y);
            dstRun.SetBlue(srcRun.color.z);
            dstRun.SetAlpha(srcRun.color.w);
            dstRun.SetStrokeRed(srcRun.strokeColor.x);
            dstRun.SetStrokeGreen(srcRun.strokeColor.y);
            dstRun.SetStrokeBlue(srcRun.strokeColor.z);
            dstRun.SetStrokeAlpha(srcRun.strokeColor.w);
            dstRun.SetStrokeWidth(srcRun.strokeWidth);
            dstRun.SetStrokeJoin(srcRun.strokeJoin);
            dstRun.SetStrokeCap(srcRun.strokeCap);
            dstRun.SetStrokeMiterLimit(srcRun.strokeMiterLimit);
            dstRun.SetPositionUsesGlyphOriginX(srcRun.positionUsesGlyphOriginX);
        }
    }
}

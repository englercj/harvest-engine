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
        dstShape.SetBandScaleX(srcShape.bandScaleX);
        dstShape.SetBandScaleY(srcShape.bandScaleY);
        dstShape.SetBandOffsetX(srcShape.bandOffsetX);
        dstShape.SetBandOffsetY(srcShape.bandOffsetY);
        dstShape.SetGlyphBandLocX(srcShape.glyphBandLocX);
        dstShape.SetGlyphBandLocY(srcShape.glyphBandLocY);
        dstShape.SetBandMaxX(srcShape.bandMaxX);
        dstShape.SetBandMaxY(srcShape.bandMaxY);
        dstShape.SetFillRule(srcShape.fillRule);
        dstShape.SetFirstOutlineCommand(srcShape.firstOutlineCommand);
        dstShape.SetOutlineCommandCount(srcShape.outlineCommandCount);
    }

    template <typename TBuilder>
    void FillCompiledOutlineData(
        TBuilder outline,
        Span<const CompiledOutlinePoint> points,
        Span<const CompiledOutlineCommand> commands)
    {
        schema::List<OutlinePoint>::Builder dstPoints = outline.InitPoints(points.Size());
        for (uint32_t pointIndex = 0; pointIndex < points.Size(); ++pointIndex)
        {
            dstPoints[pointIndex].SetX(points[pointIndex].x);
            dstPoints[pointIndex].SetY(points[pointIndex].y);
        }

        schema::List<OutlineCommand>::Builder dstCommands = outline.InitCommands(commands.Size());
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

    inline void FillFontFaceResourceRenderData(FontFaceRenderData::Builder render, const CompiledFontRenderData& renderData)
    {
        render.SetCurveTextureWidth(renderData.curveTextureWidth);
        render.SetCurveTextureHeight(renderData.curveTextureHeight);
        render.SetBandTextureWidth(renderData.bandTextureWidth);
        render.SetBandTextureHeight(renderData.bandTextureHeight);
        render.SetBandOverlapEpsilon(renderData.bandOverlapEpsilon);

        schema::List<FontFaceGlyphRenderData>::Builder glyphs = render.InitGlyphs(renderData.glyphs.Size());
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

    inline void FillFontFaceResourceOutlineData(FontFaceOutlineData::Builder outline, const CompiledFontRenderData& renderData)
    {
        FillCompiledOutlineData(
            outline,
            Span<const CompiledOutlinePoint>(renderData.outlinePoints.Data(), renderData.outlinePoints.Size()),
            Span<const CompiledOutlineCommand>(renderData.outlineCommands.Data(), renderData.outlineCommands.Size()));
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

    inline void FillVectorImageResourceRenderData(VectorImageRenderData::Builder render, const CompiledVectorImageData& imageData)
    {
        render.SetCurveTextureWidth(imageData.curveTextureWidth);
        render.SetCurveTextureHeight(imageData.curveTextureHeight);
        render.SetBandTextureWidth(imageData.bandTextureWidth);
        render.SetBandTextureHeight(imageData.bandTextureHeight);
        render.SetBandOverlapEpsilon(imageData.bandOverlapEpsilon);

        schema::List<VectorImageShapeRenderData>::Builder shapes = render.InitShapes(imageData.shapes.Size());
        for (uint32_t shapeIndex = 0; shapeIndex < imageData.shapes.Size(); ++shapeIndex)
        {
            const CompiledVectorShapeRenderEntry& srcShape = imageData.shapes[shapeIndex];
            VectorImageShapeRenderData::Builder dstShape = shapes[shapeIndex];
            FillCommonShapeRenderData(dstShape, srcShape);
        }
    }

    inline void FillVectorImageResourceOutlineData(VectorImageOutlineData::Builder outline, const CompiledVectorImageData& imageData)
    {
        FillCompiledOutlineData(
            outline,
            Span<const CompiledOutlinePoint>(imageData.outlinePoints.Data(), imageData.outlinePoints.Size()),
            Span<const CompiledOutlineCommand>(imageData.outlineCommands.Data(), imageData.outlineCommands.Size()));
    }

    inline void FillVectorImageResourcePaintData(VectorImagePaintData::Builder paint, const CompiledVectorImageData& imageData)
    {
        schema::List<VectorImageLayer>::Builder layers = paint.InitLayers(imageData.layers.Size());
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
    }
}

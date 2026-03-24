// Copyright Chad Engler

#pragma once

#include "he/scribe/schema/scribe_assets.hsc.h"

namespace he::scribe
{
    using FontSourceFormat = ScribeFontFace::SourceFormat;

    using FontFaceMetrics = ScribeFontFace::Metrics;
    using FontFaceImportMetadata = ScribeFontFace::ImportMetadata;
    using FontFaceShapingData = ScribeFontFace::ShapingData;
    using FontFaceGlyphRenderData = ScribeFontFace::GlyphRenderData;
    using FontFacePaletteBackground = ScribeFontFace::PaletteBackground;
    using FontFacePaletteColor = ScribeFontFace::PaletteColor;
    using FontFacePalette = ScribeFontFace::Palette;
    using FontFaceColorSource = ScribeFontFace::ColorSource;
    using FontFaceColorGlyphLayer = ScribeFontFace::ColorGlyphLayer;
    using FontFaceColorGlyph = ScribeFontFace::ColorGlyph;
    using FontFaceRenderData = ScribeFontFace::RenderData;
    using FontFacePaintData = ScribeFontFace::PaintData;
    using FontFaceResource = ScribeFontFace::RuntimeResource;
    using FontFaceResourceReader = ScribeFontFace::RuntimeResource::Reader;

    using FontFamilyResource = ScribeFontFamily::RuntimeResource;
    using FontFamilyResourceReader = ScribeFontFamily::RuntimeResource::Reader;

    using VectorImageRuntimeMetadata = ScribeImage::Metadata;
    using VectorImageShapeRenderData = ScribeImage::ShapeRenderData;
    using VectorImageRenderData = ScribeImage::RenderData;
    using VectorImageLayer = ScribeImage::Layer;
    using VectorImagePaintData = ScribeImage::PaintData;
    using VectorImageResource = ScribeImage::RuntimeResource;
    using VectorImageResourceReader = ScribeImage::RuntimeResource::Reader;
}

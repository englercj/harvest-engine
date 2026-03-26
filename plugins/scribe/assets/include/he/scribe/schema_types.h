// Copyright Chad Engler

#pragma once

#include "he/scribe/schema/scribe_assets.hsc.h"

namespace he::scribe
{
    using FontSourceFormat = ScribeFontFace::SourceFormat;
    using FontFaceMetrics = ScribeFontFace::Metrics;
    using FontFaceShapingData = ScribeFontFace::RuntimeResource::Shaping;
    using FontFaceRuntimeMetadata = ScribeFontFace::RuntimeResource::Metadata;
    using FontFaceStrokeData = ScribeFontFace::RuntimeResource::Stroke;
    using FontFaceGlyphRenderData = ScribeFontFace::GlyphRenderData;
    using FontFacePaletteBackground = ScribeFontFace::PaletteBackground;
    using FontFacePaletteColor = ScribeFontFace::PaletteColor;
    using FontFacePalette = ScribeFontFace::Palette;
    using FontFaceColorSource = ScribeFontFace::ColorSource;
    using FontFaceColorGlyphLayer = ScribeFontFace::ColorGlyphLayer;
    using FontFaceColorGlyph = ScribeFontFace::ColorGlyph;
    using FontFaceFillData = ScribeFontFace::RuntimeResource::Fill;
    using FontFacePaintData = ScribeFontFace::RuntimeResource::Paint;
    using FontFaceResource = ScribeFontFace::RuntimeResource;
    using FontFaceResourceReader = ScribeFontFace::RuntimeResource::Reader;

    using FontFamilyResource = ScribeFontFamily::RuntimeResource;
    using FontFamilyResourceReader = ScribeFontFamily::RuntimeResource::Reader;

    using VectorImageRuntimeMetadata = ScribeImage::RuntimeResource::Metadata;
    using VectorImageStrokeData = ScribeImage::RuntimeResource::Stroke;
    using VectorImageShapeRenderData = ScribeImage::ShapeRenderData;
    using VectorImageFillData = ScribeImage::RuntimeResource::Fill;
    using VectorImageLayer = ScribeImage::Layer;
    using VectorImagePaintData = ScribeImage::RuntimeResource::Paint;
    using VectorImageResource = ScribeImage::RuntimeResource;
    using VectorImageResourceReader = ScribeImage::RuntimeResource::Reader;
}

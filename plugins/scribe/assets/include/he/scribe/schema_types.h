// Copyright Chad Engler

#pragma once

#include "he/scribe/schema/scribe_assets.hsc.h"

namespace he::scribe
{
    using FontSourceFormat = ScribeFontFace::SourceFormat;

    using FontFaceMetrics = ScribeFontFace::Metrics;
    using FontFaceShapingData = ScribeFontFace::RuntimeResource::Shaping;
    using FontFaceRuntimeMetadata = ScribeFontFace::RuntimeResource::Metadata;
    using FontFaceGlyphRenderData = ScribeFontFace::GlyphRenderData;
    using FontFacePaletteBackground = ScribeFontFace::PaletteBackground;
    using FontFacePaletteColor = ScribeFontFace::PaletteColor;
    using FontFacePalette = ScribeFontFace::Palette;
    using FontFaceColorSource = ScribeFontFace::ColorSource;
    using FontFaceColorGlyphLayer = ScribeFontFace::ColorGlyphLayer;
    using FontFaceColorGlyph = ScribeFontFace::ColorGlyph;
    using FontFaceRenderData = ScribeFontFace::RuntimeResource::Render;
    using FontFacePaintData = ScribeFontFace::RuntimeResource::Paint;
    using FontFaceResource = ScribeFontFace::RuntimeResource;
    using FontFaceResourceReader = ScribeFontFace::RuntimeResource::Reader;

    using FontFamilyResource = ScribeFontFamily::RuntimeResource;
    using FontFamilyResourceReader = ScribeFontFamily::RuntimeResource::Reader;

    using VectorImageRuntimeMetadata = ScribeImage::RuntimeResource::Metadata;
    using VectorImageShapeRenderData = ScribeImage::ShapeRenderData;
    using VectorImageRenderData = ScribeImage::RuntimeResource::Render;
    using VectorImageLayer = ScribeImage::Layer;
    using VectorImagePaintData = ScribeImage::RuntimeResource::Paint;
    using VectorImageResource = ScribeImage::RuntimeResource;
    using VectorImageResourceReader = ScribeImage::RuntimeResource::Reader;
}

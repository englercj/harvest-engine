// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"
#include "he/scribe/schema/runtime_blob.hsc.h"

#include "he/core/span.h"
#include "he/core/types.h"

namespace he::scribe
{
    // M5 format marker adding per-layer affine transforms for COLR v1 paint lowering.
    inline constexpr uint32_t RuntimeBlobFormatVersion = 5;

    struct LoadedFontFaceBlob
    {
        CompiledFontFaceBlob::Reader root{};
        FontFaceShapingData::Reader shaping{};
        FontFaceImportMetadata::Reader metadata{};
        FontFaceRenderData::Reader render{};
        FontFacePaintData::Reader paint{};
    };

    struct LoadedFontFamilyBlob
    {
        CompiledFontFamilyBlob::Reader root{};
        FontFamilyRuntimeData::Reader family{};
    };

    struct LoadedVectorImageBlob
    {
        CompiledVectorImageBlob::Reader root{};
        VectorImageRuntimeMetadata::Reader metadata{};
    };

    bool LoadCompiledFontFaceBlob(LoadedFontFaceBlob& out, Span<const schema::Word> data);
    bool LoadCompiledFontFamilyBlob(LoadedFontFamilyBlob& out, Span<const schema::Word> data);
    bool LoadCompiledVectorImageBlob(LoadedVectorImageBlob& out, Span<const schema::Word> data);
}

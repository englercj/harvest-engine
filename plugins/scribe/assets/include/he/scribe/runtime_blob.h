// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"
#include "he/scribe/schema/runtime_blob.hsc.h"

#include "he/core/span.h"
#include "he/core/types.h"

namespace he::scribe
{
    // M6 format marker adding compiled vector image render and paint payloads.
    inline constexpr uint32_t RuntimeBlobFormatVersion = 6;

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
        VectorImageRenderData::Reader render{};
        VectorImagePaintData::Reader paint{};
    };

    bool LoadCompiledFontFaceBlob(LoadedFontFaceBlob& out, Span<const schema::Word> data);
    bool LoadCompiledFontFamilyBlob(LoadedFontFamilyBlob& out, Span<const schema::Word> data);
    bool LoadCompiledVectorImageBlob(LoadedVectorImageBlob& out, Span<const schema::Word> data);
}

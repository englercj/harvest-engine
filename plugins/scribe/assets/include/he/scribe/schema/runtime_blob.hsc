// Copyright Chad Engler

@0x970edff4ba4b6a8c;

import "he/schema/schema.hsc";

namespace he.scribe;

enum RuntimeBlobKind
{
    Unknown @0;
    FontFace @1;
    FontFamily @2;
    VectorImage @3;
}

struct RuntimeBlobHeader
{
    formatVersion @0 :uint32;
    kind @1 :RuntimeBlobKind;
    flags @2 :uint32;
}

// M0 deliberately keeps the payload model simple: a Harvest-owned schema wrapper around the
// major runtime payload groups. Later milestones can replace individual Blob fields with more
// explicit structures without changing the high-level runtime contract.
struct CompiledFontFaceBlob
{
    const ResourceName :String = "he.scribe.font_face.runtime_blob";

    header @0 :RuntimeBlobHeader;
    shapingData @1 :Blob;
    curveData @2 :Blob;
    bandData @3 :Blob;
    paintData @4 :Blob;
    metadataData @5 :Blob;
}

struct CompiledFontFamilyBlob
{
    const ResourceName :String = "he.scribe.font_family.runtime_blob";

    header @0 :RuntimeBlobHeader;
    familyData @1 :Blob;
}

struct CompiledVectorImageBlob
{
    const ResourceName :String = "he.scribe.vector_image.runtime_blob";

    header @0 :RuntimeBlobHeader;
    curveData @1 :Blob;
    bandData @2 :Blob;
    paintData @3 :Blob;
    metadataData @4 :Blob;
}

<!-- Copyright Chad Engler -->

# Scribe Font Blob And Stroke Resource Refactor

## Summary

Refactor Scribe runtime resources so each font face stores three purpose-specific payloads
instead of carrying the full source font for everything:

- `shaping`: one compact per-face HarfBuzz blob, produced at compile time by subsetting the
  face down to layout-only tables.
- `fill`: the existing precomputed curve/band payloads, kept as the hot-path render format.
- `stroke`: one compact typed command stream for runtime stroking, stored for both fonts and
  vector images so the runtime uses one stroke implementation.

This is a schema-breaking change. Old compiled Scribe assets are not supported; the plan
assumes a full asset rebuild.

## Implementation Changes

### Schema and public type changes

- Rename `RuntimeResource.render` to `RuntimeResource.fill` for both `ScribeFontFace` and
  `ScribeImage`.
- Move `curveData` and `bandData` into the new `fill` group.
- Rename `RuntimeResource.outline` to `RuntimeResource.stroke` for both `ScribeFontFace` and
  `ScribeImage`.
- Rename the generated type aliases/helpers accordingly:
  - `FontFaceRenderData` -> `FontFaceFillData`
  - `VectorImageRenderData` -> `VectorImageFillData`
  - `FontFaceOutlineData` -> `FontFaceStrokeData`
  - `VectorImageOutlineData` -> `VectorImageStrokeData`
- Keep `RuntimeResource.shaping` as a separate group, but change its blob semantics from
  “full source font bytes” to “HarfBuzz runtime face bytes for this face only”.
- Replace the current float outline storage with compact typed stroke schema types shared by
  fonts and images:
  - `StrokeCommandType`: `MoveTo`, `LineTo`, `QuadraticTo`, `Close`
  - `StrokePoint`: signed `int32` coordinates in fixed `1/256` units
  - `StrokeCommand`: `type`, `firstPoint`
  - `stroke.pointScale`: `float32`, fixed to `1.0f / 256.0f`
- Keep per-glyph/per-shape stroke ranges on the fill-entry structs, renamed to
  `firstStrokeCommand` and `strokeCommandCount`.
- Do not store cubic stroke commands. Normalize all font/image stroke geometry to lines +
  quadratics at compile time.

### Font compiler and shaping blob generation

- Add a compile-time HarfBuzz subset step in the font compiler.
- For each imported face, produce one per-face shaping blob using HarfBuzz subsetting over
  that face only.
- Configure the shaping subset to keep all glyphs/codepoints for the face, but drop
  non-layout/non-metrics/non-runtime tables.
- Use an explicit allowlist for current Scribe horizontal layout support:
  - `cmap`, `head`, `hhea`, `hmtx`, `maxp`, `OS/2`, `name`, `GSUB`, `GPOS`, `GDEF`, `BASE`,
    `JSTF`, `kern`
- Exclude outline/color/bitmap/render tables from the shaping blob because fill and stroke
  are now carried separately.
- Keep `faceIndex` in `shaping` metadata, but the blob itself is already face-specific.
- Continue compiling fill data exactly as today, just writing it into `fill`.
- Build font stroke data at compile time from FreeType outlines:
  - flatten composite glyphs into one final glyph-local stroke command stream
  - preserve contour boundaries and closes
  - convert CFF/CFF2 cubics to quadratics using the same tolerance family already accepted
    by Scribe’s fill compiler
  - quantize stroke points to fixed `1/256` units before serialization

### Vector image compiler and runtime stroke path

- Keep SVG fill compilation behavior unchanged, but write outputs into `fill`.
- Emit stroke command streams for vector images into `stroke` using the same schema and
  normalization rules as fonts.
- Reuse one compiler-side stroke-source builder for both fonts and images so the runtime
  never has to branch on source type when stroking.
- Keep vector-image stroke commands contour-faithful, including open paths, because image
  stroking still needs cap handling.

### Runtime loading, stroking, and cache behavior

- Update `ScribeContext` and all readers/loaders to use `GetFill()` / `GetStroke()` instead
  of `GetRender()` / `GetOutline()`.
- Update HarfBuzz runtime creation to read the new shaping blob bytes instead of the full
  source font bytes.
- Keep one runtime stroke implementation that consumes the compiled `stroke` command stream
  for both fonts and images.
- Keep the existing sparse stroke cache in `ScribeContext`, keyed by:
  - font face or image handle
  - glyph index or shape index
  - outline width
  - join style
  - cap style
  - miter limit
- Keep fill resource caching unchanged except for the `fill` rename and blob location move.
- Keep retained text/vector behavior unchanged from a caller perspective, but ensure
  outline/stroke draws are sourced from the compiled `stroke` payload and never from
  offset-copy hacks.
- Remove any runtime dependency on full source font parsing for Scribe text layout or
  stroke generation.

## Tests and Acceptance Criteria

- Schema/resource tests:
  - font runtime blob loads with `shaping`, `fill`, and `stroke` groups in the new layout
  - image runtime blob loads with `fill` and `stroke` groups in the new layout
  - `curveData` and `bandData` are only read from `fill`
  - per-glyph/per-shape `firstStrokeCommand` and `strokeCommandCount` resolve correctly
- Font shaping tests:
  - text layout still succeeds from the new per-face HarfBuzz subset blob after temporary
    source bytes are discarded
  - fallback, mixed-script shaping, emoji/color glyph shaping, and existing retained-text
    preparation tests continue to pass
- Stroke runtime tests:
  - retained text outline emits one real stroke draw per glyph instead of translated copies
  - retained vector image stroke draws resolve through the same runtime stroke path
  - CPU stroker tests cover rectangle, triangle, round joins, miter limit clipping,
    open-path caps, and counters/holes
- Regression/verification commands:
  - regenerate projects
  - build `he_scribe.vcxproj`
  - build `he_test_runner.vcxproj`
  - run `he_test_runner --filter 'scribe:runtime_resource'`
  - run `he_test_runner --filter 'scribe:retained_text'`
  - run `he_test_runner --filter 'scribe:retained_vector_image'`
  - run `he_test_runner --filter 'scribe:layout_engine'`
  - run `he_test_runner --filter 'scribe:vector_image_pipeline'`

## Assumptions And Defaults

- Backward compatibility for old compiled Scribe blobs is out of scope; all assets are
  rebuilt.
- Typed schema lists are preferred for `stroke`; do not switch to an opaque packed blob in
  this refactor.
- The stroke schema is shared between fonts and images, with one runtime stroker
  implementation.
- Cubics are not preserved in the stroke schema; compile-time normalization to quadratics is
  the default.
- Variable-font-specific shaping/outline preservation is not expanded in this refactor
  beyond what current Scribe already supports.
- Per-face shaping blobs are acceptable even for TTC/OTC collections; no table-level dedup
  is planned in this pass.

## Useful Handoff Memory

- There is already an earlier stroke handoff doc at
  `plugins/scribe/docs/stroke_implementation_handoff.md`. It reflects the previous direction
  of storing font outline commands plus runtime stroking; parts of it are now obsolete for
  fonts because this refactor changes the shaping/stroke split.
- HarfBuzz subsetting support is already available in the current install:
  - `.build/installs/harfbuzz-12.2.0-.../harfbuzz-12.2.0/src/hb-subset.h`
- The current schema/runtime in the repo already has temporary `render`/`outline` additions
  from the previous stroke work. This refactor should treat that as transitional and replace
  it with the new `fill`/`stroke` layout instead of layering more changes on top.
- Current runtime still creates HarfBuzz objects from `shaping.sourceBytes` in
  `plugins/scribe/runtime/src/context.cpp`. That code will need to keep working, but against
  the new subset blob rather than the original font bytes.
- Current tests and builders that construct font/image runtime blobs directly will need broad
  rename updates because many of them still call:
  - `GetRender()`
  - `GetOutline()`
  - `SetCurveData()`
  - `SetBandData()`
- The current generated projects already include HarfBuzz and, after regeneration, can pick
  up new runtime source files normally.

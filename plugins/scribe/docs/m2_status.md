<!-- Copyright Chad Engler -->

# Milestone M2 Status

## Implemented

- `he_scribe_assets` now defines typed schema structs for:
  - `ScribeFontFace`,
  - `ScribeFontFamily`,
  - `ScribeImage`,
  - import cache payloads,
  - compiled runtime blobs,
  - typed runtime metadata and shaping payloads.
- `he_scribe_assets` now exposes typed runtime blob loaders in:
  - `he/scribe/runtime_blob.h`
  - `assets/src/runtime_blob.cpp`
- `he_scribe_editor` now registers:
  - `ScribeFontFace` with `FontImporter` and `FontFaceCompiler`,
  - `ScribeFontFamily` with `FontFamilyCompiler`,
  - `ScribeImage` with `ImageCompiler`.
- Font import now handles:
  - `.ttf`,
  - `.otf`,
  - `.ttc`,
  - `.otc`.
- Font import uses FreeType only in the editor/compiler path and writes importer cache
  resources for:
  - source bytes,
  - per-face imported metadata.
- Font compile now produces a versioned `CompiledFontFaceBlob` runtime resource that stores:
  - shaping bytes,
  - typed face metadata,
  - packed curve texture bytes,
  - packed band texture bytes,
  - per-glyph render metadata.
- The font compiler now:
  - loads outlines from importer-cached source bytes,
  - decomposes quadratic outlines directly,
  - approximates cubic segments into line-equivalent quadratics during compile,
  - chooses per-glyph band counts in the range `1..8` to reduce worst-band occupancy,
  - applies the documented overlap epsilon policy,
  - sorts horizontal and vertical band membership in descending max-axis order,
  - packs reference-style curve and band textures for runtime consumption.
- `he_scribe` now exposes a compiled-resource runtime path through:
  - `he/scribe/compiled_font.h`
  - `BuildCompiledGlyphResourceData()`
  - `Renderer::CreateCompiledGlyphResource()`
- Font family compile now produces a versioned `CompiledFontFamilyBlob`.
- Vector image assets can compile into a versioned placeholder `CompiledVectorImageBlob`.
- Unit tests now cover:
  - runtime blob loader success,
  - version rejection,
  - compiled glyph resource view generation.

## Verified

- `./hemake.ps1 generate-projects vs2026`
- `he_scribe__shaderc_1.vcxproj`
- `he_scribe_assets__schemac_1.vcxproj`
- `he_scribe_assets.vcxproj`
- `he_scribe_editor.vcxproj`
- `he_scribe_tests.vcxproj`
- `he_scribe.vcxproj`
- `he_test_runner.vcxproj`
- `he_test_runner.exe --filter "scribe:runtime_blob"`

## Milestone Result

The original roadmap success signal said a font source should import, compile, and render from
compiled resources with no direct source file dependency or FreeType dependency at runtime.

That is now true at the engine boundary for monochrome outline glyphs:

- import and compile use FreeType only in `he_scribe_editor`,
- the runtime blob is self-contained,
- `he_scribe` can build glyph render resources from compiled blob data directly,
- the renderer no longer depends on the synthetic debug glyph path for this basic font case.

## Still Deferred Beyond M2

M2 completion does not mean the broader `scribe` roadmap is finished. The remaining work moves
into later milestones:

- color glyph layers and paint data are still placeholder paths,
- SVG compilation is still placeholder-only,
- text shaping/layout through HarfBuzz is still an M5 concern,
- editor preview and on-screen UI consumption remain later milestones,
- curve and band payloads are now structurally correct, but later milestones can still add
  band dedup/subset reuse and richer color/vector data packing.

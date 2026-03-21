<!-- Copyright Chad Engler -->

# Implementation Plan

## Phase 0: Scaffold And Decide Hard Dependencies

Deliverables:

- Create `plugins/text_vector` with real modules instead of the current docs-only placeholder.
- Add `contrib/harfbuzz`.
- Choose an SVG parser dependency.
- Choose the bidi/line-break strategy.
- Bring the reference shaders into the repo with tracked provenance.
- Decide the compiled runtime blob shape, preferably using `he_schema`.

Exit criteria:

- Project generation succeeds.
- Dependency install/build succeeds on Windows.
- Shader source intake policy is documented and checked in.
- The runtime has no FreeType dependency in its module graph.

## Phase 1: Shader Intake And Render Resource Skeleton

Work:

- Copy the published Slug reference shaders into a third-party intake area.
- Port them to Slang with minimal logic changes.
- Build a tiny render test path in `he_text_vector` that can bind:
  - curve texture,
  - band texture,
  - glyph data constants,
  - per-draw transform and color.
- Create the minimal renderer API:
  - resource creation,
  - draw list recording,
  - frame submission hook.

Exit criteria:

- A synthetic test resource can render one known glyph or vector primitive through `he_rhi`.
- The Slang shaders compile through the existing `shader_compile` flow.

## Phase 2: Asset Schemas And Runtime Formats

Work:

- Define plugin-owned asset schemas for:
  - `TextVectorFontFace`,
  - `TextVectorFontFamily`,
  - `TextVectorImage`.
- Define resource IDs for:
  - imported outline cache,
  - imported shaping table cache,
  - compiled curve/band payload,
  - compiled paint payload,
  - compiled metadata payload.
- Create `he_schema`-backed serialization rules and version fields for each compiled runtime
  blob.

Exit criteria:

- Asset schemas compile.
- Runtime resource loaders can parse versioned payloads without the editor present or any raw
  font parser present.

## Phase 3: Font Importer And Compiler

Work:

- Build a font importer using FreeType.
- Extract outlines, metrics, naming, variation axes, kerning-related metadata, and color-font
  table metadata.
- Emit importer cache resources so the compiler never needs to reopen the original font.
- Build a compiler that normalizes outlines to the Slug canonical representation and packs:
  - curve texture data,
  - band texture data,
  - glyph metadata,
  - paint and layer data,
  - shaping data needed by the runtime layout engine.

Exit criteria:

- Importing `.ttf` and `.otf` produces valid `.he_asset` files plus cache resources.
- Compiling produces loadable runtime resources.
- A debug preview can draw a set of known glyphs from compiled output with no FreeType usage
  in the runtime path.

## Phase 4: SVG Importer And Compiler

Work:

- Parse SVG documents into an internal scene graph.
- Convert supported shapes and paths into the same canonical quadratic representation used by
  fonts.
- Preserve fill rules, transforms, solid colors, gradients, opacity, and stroke information.
- Reuse the same curve/band/payload packing path as fonts where practical.

Exit criteria:

- Importing a representative SVG sample set produces assets and runtime resources.
- Compiled SVG resources render through the same `he_text_vector` renderer API.

## Phase 5: Text Shaping And Layout

Work:

- Introduce `he_text_layout`.
- Build paragraph, run, and line data structures.
- Integrate HarfBuzz shaping.
- Add font fallback selection.
- Add bidi and line-break processing behind a provider interface.
- Preserve cluster mapping for cursor movement, selection, and hit testing.
- Load all shaping inputs from compiled Harvest font blobs rather than source font files.

Exit criteria:

- The engine can shape and lay out mixed-script text.
- Kerning, ligatures, combining marks, and composed sequences survive round-trip tests.
- Emoji ZWJ sequences are preserved in cluster mapping even if rendering support is partial.

## Phase 6: Color And Advanced Font Features

Work:

- Support layered color glyph rendering first.
- Add palette-aware rendering.
- Add variation-axis selection and named instances.
- Add fallback policy controls for missing glyphs and color format mismatch.

Exit criteria:

- COLR/CPAL layered emoji and icons render correctly.
- Variation fonts can be imported and shaped with axis overrides.

## Phase 7: Editor Preview And UI Consumption

Work:

- Add an editor preview path for font and vector assets.
- Expose a stable API for the new `plugins/ui` runtime.
- Build a small UI-side proof of concept using:
  - layout from `he_text_layout`,
  - rendering from `he_text_vector`.

Exit criteria:

- The editor can preview compiled text/vector assets.
- The UI library can render a basic styled text block and SVG icon set through
  `he_text_vector`.

## Phase 8: Hardening

Work:

- Add engine tests, golden-image checks, and shader regression fixtures.
- Add import/compile performance instrumentation.
- Add resource versioning and migration notes.
- Add documentation for asset authoring and runtime usage.

Exit criteria:

- Core tests cover importer, compiler, layout, and render correctness.
- Known representative fonts and SVG assets are stable across rebuilds.

## Suggested Validation Ladder

Validate in this order:

1. synthetic single-glyph renderer test,
2. monochrome Latin font asset path,
3. mixed-script shaping and fallback,
4. layered color glyphs,
5. SVG vector scene rendering,
6. UI integration.

That keeps the critical path narrow and avoids debugging all systems at once.

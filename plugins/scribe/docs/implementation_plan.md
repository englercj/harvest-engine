<!-- Copyright Chad Engler -->

# Implementation Plan

## Cross-Cutting Slug Constraints

These constraints apply to multiple phases and should be treated as implementation checks,
not optional polish:

- Keep dynamic dilation as the default path from the first real renderer onward. Do not
  fall back to a fixed author-controlled dilation constant except as a temporary debug aid.
- Preserve both nonzero and even-odd fill handling in compiled metadata and runtime draw
  paths. Fill rule support is shape data, not a postprocess.
- Keep multicolor glyph rendering as layered draws or layered submissions, not a
  pixel-shader loop over layers.
- Keep coverage evaluation separate from paint evaluation so the same geometry path can
  serve monochrome text, color glyphs, and SVG fills.
- Preserve the curve and band packing invariants required by the reference shader math.
  Compiler work must document and test sort order, offsets, band ranges, and flag packing.
- Keep core winding, coverage, and dilation math structurally close to the upstream
  reference unless a correctness issue is demonstrated and regression-tested.
- Keep shader resource bindings isolated to the entrypoint shader module so helper modules
  remain easy to compare against upstream reference logic.
- Avoid reintroducing the older supersampling or band-split variants as the default path
  unless profiling or correctness data justifies it.
- Compile all data the runtime needs into Harvest-owned blobs. Runtime text and vector code
  must not reopen source font or SVG files to recover omitted metadata.

## Phase 0: Scaffold And Decide Hard Dependencies

Deliverables:

- Create `plugins/scribe` with real modules instead of the current docs-only placeholder.
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
- Build a tiny render test path in `he_scribe` that can bind:
  - curve texture,
  - band texture,
  - glyph data constants,
  - per-draw transform and color.
- Keep helper shader logic resource-free and keep resource bindings in the entrypoint
  module only.
- Create the minimal renderer API:
  - resource creation,
  - draw list recording,
  - frame submission hook.

Exit criteria:

- A synthetic test resource can render one known glyph or vector primitive through `he_rhi`.
- The Slang shaders compile through the existing `shader_compile` flow.
- The vertex path uses dynamic dilation, not a fixed dilation constant.
- The Slang port preserves the upstream helper math structure closely enough for side-by-side
  diff review.

## Phase 2: Asset Schemas And Runtime Formats

Work:

- Define plugin-owned asset schemas for:
  - `ScribeFontFace`,
  - `ScribeFontFamily`,
  - `ScribeImage`.
- Define resource IDs for:
  - imported outline cache,
  - imported shaping table cache,
  - compiled curve/band payload,
  - compiled paint payload,
  - compiled metadata payload.
- Create `he_schema`-backed serialization rules and version fields for each compiled runtime
  blob.
- Define explicit fields for fill rules, band packing metadata, curve ordering assumptions,
  and layer/paint indirection so the runtime does not infer them from source assets.

Exit criteria:

- Asset schemas compile.
- Runtime resource loaders can parse versioned payloads without the editor present or any raw
  font parser present.
- The runtime blob contains enough information to drive winding, fill-rule, band lookup, and
  color-layer submission without reopening source assets.

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
- Validate the generated band and curve ordering against the assumptions encoded in the
  reference shader comments.

Exit criteria:

- Importing `.ttf` and `.otf` produces valid `.he_asset` files plus cache resources.
- Compiling produces loadable runtime resources.
- A debug preview can draw a set of known glyphs from compiled output with no FreeType usage
  in the runtime path.
- Known nonzero and even-odd test shapes render with the expected fill rule.

## Phase 4: SVG Importer And Compiler

Work:

- Parse SVG documents into an internal scene graph.
- Convert supported shapes and paths into the same canonical quadratic representation used by
  fonts.
- Preserve fill rules, transforms, solid colors, gradients, opacity, and stroke information.
- Reuse the same curve/band/payload packing path as fonts where practical.
- Keep coverage generation and paint evaluation separable so SVG paints do not fork the core
  coverage implementation.

Exit criteria:

- Importing a representative SVG sample set produces assets and runtime resources.
- Compiled SVG resources render through the same `he_scribe` renderer API.
- SVG even-odd and nonzero fill cases survive compile and render correctly.

## Phase 5: Text Shaping And Layout

Work:

- Introduce `he_scribe_layout`.
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
- Color glyph rendering uses layered submissions rather than a per-pixel layer loop.

## Phase 7: Editor Preview And UI Consumption

Work:

- Add an editor preview path for font and vector assets.
- Expose a stable API for the new `plugins/ui` runtime.
- Build a small UI-side proof of concept using:
  - layout from `he_scribe_layout`,
  - rendering from `he_scribe`.

Exit criteria:

- The editor can preview compiled text/vector assets.
- The UI library can render a basic styled text block and SVG icon set through `he_scribe`.

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

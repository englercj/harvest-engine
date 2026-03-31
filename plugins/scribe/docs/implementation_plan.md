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
- Preserve the reference texture storage expectations: curve data packed as RGBA16F control
  point texels and band data packed as two-channel 16-bit unsigned integer texels unless a
  later measured reason justifies a different GPU format with equivalent semantics.
- Use a small overlap epsilon during band assignment, target band counts that reduce the worst
  band occupancy, and preserve descending max-axis sort order inside each band.
- Keep band thickness uniform within a glyph and retain support for deduplicated adjacent
  bands or contiguous-subset reuse when the compiler can prove it is valid.
- Keep core winding, coverage, and dilation math structurally close to the upstream
  reference unless a correctness issue is demonstrated and regression-tested.
- Keep shader resource bindings isolated to the entrypoint shader module so helper modules
  remain easy to compare against upstream reference logic.
- Avoid reintroducing the older supersampling or band-split variants as the default path
  unless profiling or correctness data justifies it.
- Compile all data the runtime needs into Harvest-owned blobs. Runtime text and vector code
  must not reopen source font or SVG files to recover omitted metadata.
- Treat cap-height-aware pixel-grid sizing as a planned UI/layout integration detail. The
  renderer does not use hinting, so later API work should preserve room for `OS/2.sCapHeight`
  plus DPI-aware font-size snapping where that improves UI crispness.

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
- Define explicit fields for curve texture layout, band texture layout, band overlap epsilon,
  and any deduplicated-band indirection used by compiled resources.

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
- Implement band assignment with a documented epsilon policy and retain optional adjacent-band
  deduplication and contiguous-subset reuse optimizations when they reduce size without
  changing semantics.
- Preserve `OS/2.sCapHeight` or equivalent imported metrics needed for later UI font-size
  snapping guidance.

Exit criteria:

- Importing `.ttf` and `.otf` produces valid `.he_asset` files plus cache resources.
- Compiling produces loadable runtime resources.
- A debug preview can draw a set of known glyphs from compiled output with no FreeType usage
  in the runtime path.
- Known nonzero and even-odd test shapes render with the expected fill rule.
- Compiler output matches the documented curve and band packing contract, including sort order
  and overlap policy.

## Phase 4: Text Shaping And Layout

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

## Phase 5: Visual Test App

Work:

- Add a dedicated `scribe` test app that opens a real window and renders text through
  `he_scribe`.
- Seed it with representative demo cases for:
  - basic monochrome text,
  - fallback,
  - wrapping,
  - mixed-script shaping,
  - hit-test/caret visualization where useful.
- Treat the app as a standing visual testbed, not one-off scaffolding.
- Require later user-visible `scribe` features to add or update a demo case in the test app
  alongside unit tests.

Exit criteria:

- Developers can launch the test app locally and inspect real `scribe` output in a window.
- The app exercises the runtime text stack directly rather than going through editor or UI
  integration layers.
- The app is useful enough to serve as the default visual validation target for later
  renderer, layout, color-font, and SVG work.

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

## Phase 7: SVG Importer And Compiler

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

## Phase 8: Hardening

Work:

- Add engine tests, golden-image checks, and shader regression fixtures.
- Add import/compile performance instrumentation.
- Add resource versioning and migration notes.
- Add documentation for asset authoring and runtime usage.

Exit criteria:

- Core tests cover importer, compiler, layout, and render correctness.
- Known representative fonts and SVG assets are stable across rebuilds.
- Regression coverage includes band dedup/subset cases and any cap-height-aware sizing rules
  exposed to UI consumers.

## Suggested Validation Ladder

Validate in this order:

1. synthetic single-glyph renderer test,
2. monochrome Latin font asset path,
3. mixed-script shaping and fallback,
4. dedicated visual test app,
5. layered color glyphs,
6. SVG vector scene rendering.

That keeps the critical path narrow and avoids debugging all systems at once.

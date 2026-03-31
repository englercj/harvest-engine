<!-- Copyright Chad Engler -->

# Architecture Plan

## Outcome

Build a new first-party plugin, `he.scribe`, that provides:

- a reusable GPU text and vector renderer,
- a text shaping and layout engine,
- an asset pipeline for font and SVG sources,
- editor-time import/compile/preview integration,
- a stable runtime API for the new Harvest UI library to consume.

## Why A Separate Plugin

Current Harvest seams already point in this direction:

- `plugins/rhi` already owns low-level GPU abstractions and shader compilation.
- `plugins/assets` already owns importer/compiler registration and cache resources.
- `plugins/editor` currently owns the only real renderer client, but it should consume a
  reusable rendering service instead of remaining the permanent home of GPU text/vector code.
- `plugins/ui` is not yet a usable runtime integration point in this checkout.

That makes a dedicated plugin cleaner than extending `editor` or forcing `rhi` to own
algorithm-specific code.

## Proposed Module Layout

The plugin should eventually grow into four runtime/editor modules plus shader content:

1. `he_scribe_assets`
   - Owns asset schemas, runtime resource headers, and file/resource identifiers.
   - Depends on `he_assets`, `he_core`, and `he_schema`.

2. `he_scribe_editor`
   - Registers importers, compilers, and asset preview helpers.
   - Depends on `he_assets_editor`, `he_editor`, `he_scribe_assets`, `freetype`, and
     the selected SVG parser dependency.

3. `he_scribe_layout`
   - Owns shaping, font fallback, script segmentation, bidi handling, line breaking, and
     glyph run generation over compiled Harvest font data.
   - Uses HarfBuzz for shaping.
   - Depends on `he_core`, `he_math`, `he_scribe_assets`, and HarfBuzz.
   - Must not depend on FreeType or raw source font parsing at runtime.

4. `he_scribe`
   - Owns GPU resources, draw packet encoding, shader bindings, render passes, and vector/glyph
     submission APIs.
   - Depends on `he_core`, `he_math`, `he_rhi`, `he_scribe_assets`, and
     `he_scribe_layout`.

## External Dependencies

- `freetype`
  - Already present in `contrib/`.
  - Used only for importer/compiler work: face loading, outline extraction, metrics, and
    access to color-font tables.
- `harfbuzz`
  - New `contrib/` plugin required.
  - Used strictly for runtime shaping and glyph cluster production over compiled data.
- SVG parser dependency
  - New dependency still to be selected in phase 0.
  - Must expose parsed paths, paints, transforms, gradients, and stroke data.
- Unicode segmentation/bidi dependency
  - HarfBuzz shapes text but does not replace full bidi and line-break handling.
  - Decide in phase 0 whether to use ICU or a lighter split provider.

## Canonical Internal Data Model

The plugin should normalize all source content into one canonical vector representation:

- quadratic Bezier contours,
- contour winding and fill rules,
- paint commands for solid fills and gradients,
- optional stroke commands,
- glyph/vector bounds, banding, and per-shape metadata,
- shaping metadata separate from render data.

That keeps the renderer close to the Slug algorithm even when sources are:

- TrueType quadratic outlines,
- CFF cubic outlines,
- SVG path data,
- layered color glyph definitions.

### Key Decision

Canonicalize to quadratic curves at compile time.

Reasons:

- The 2017 paper is based on quadratic curves.
- The published reference shaders operate on the same banded curve representation.
- Keeping compile-time conversion outside the runtime preserves shader simplicity.

## Runtime Resource Model

Each compiled font or vector resource should emit:

- a self-contained compiled Harvest blob,
- curve texture payload,
- band texture payload,
- glyph or shape table,
- paint table,
- shaping metadata table,
- optional color-layer table,
- resource header with version, bounds, and offsets.

Prefer expressing that blob with `he_schema` so runtime loading is direct and versioned
without requiring source parsers or tool-specific readers.

At runtime, `he_scribe` should load these resources into immutable GPU objects and
expose draw handles that can be reused across frames.

## Render Pipeline

### Glyph And Vector Rendering

Use the reference Slug path as the initial implementation shape:

- draw one quad or bounding polygon per glyph/vector item,
- sample curve and band textures in the pixel shader,
- compute coverage from horizontal and vertical ray winding contributions,
- support nonzero fill immediately,
- add even-odd as a feature flag because the reference shaders already include it.

### Dynamic Dilation

Keep dynamic dilation in the vertex stage from the start.

Reasons:

- It is the main post-2017 improvement.
- It removes the need for a fixed global dilation constant.
- It matters for perspective-correct UI-in-world and any future 3D text surfaces.

### Full Color Text

Treat multicolor emoji and layered color glyphs as layered draws, not a loop inside the
pixel shader.

That follows the March 17, 2026 Slug update and avoids wasted work over empty regions.

### Full Color Vector Graphics

Use the same geometry and coverage path as text, but separate paint evaluation from
coverage evaluation:

- pass 1: coverage from Slug geometry,
- pass 2 or inline branchless path: paint evaluation from solid/gradient/stroke tables.

The exact pass structure can change, but coverage must remain decoupled from paint.

## Text Stack

Text processing should be layered as:

1. UTF-8 decode and paragraph segmentation.
2. Script run detection, direction analysis, and line-break opportunities.
3. HarfBuzz shaping per font face and feature set.
4. Font fallback resolution when glyphs are missing.
5. Cluster-to-glyph run assembly.
6. Layout into lines, inline boxes, and draw packets.
7. Submission to `he_scribe`.

The UI library should depend on `he_scribe_layout` for layout and on `he_scribe` for
rendering.

### HarfBuzz Runtime Data Strategy

The runtime must not parse raw `.ttf` or `.otf` files directly.

That means phase 0 must choose one of these two HarfBuzz-facing strategies:

- preserve the OpenType tables HarfBuzz needs inside the compiled Harvest blob and expose them
  to HarfBuzz through runtime-owned memory, or
- compile font shaping data into a Harvest-normalized representation and use HarfBuzz only
  where that still fits its data model cleanly.

Whichever route is chosen, the runtime contract stays the same: it consumes compiled asset
output only.

## Asset Pipeline Placement

Keep import and compile work in `he_scribe_editor`, but use the existing shared Harvest
asset pipeline:

- source file -> importer -> `.he_asset` + cache resources,
- asset compiler -> compiled runtime resources,
- runtime loader -> GPU resources and layout metadata.

This follows the existing `ImageImporter -> Texture2DCompiler` pattern rather than building
a separate parallel toolchain.

## Editor Integration

Editor responsibilities should stay thin:

- invoke importers and compilers through the existing asset system,
- optionally host a custom preview document for font/vector assets,
- not own permanent renderer internals.

This keeps the UI/editor path consuming the same runtime used by games and tools.

## Explicit Non-Goals For First Delivery

- Replacing ImGui text immediately.
- Supporting every SVG feature on day one.
- Solving all color emoji formats in the first milestone.
- Introducing Slug-specific behavior into `he_rhi`.

Those should come after the core asset, shaping, and renderer stack is stable.

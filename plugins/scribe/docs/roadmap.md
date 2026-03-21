<!-- Copyright Chad Engler -->

# Roadmap

## Milestone M0: Intake And Scaffolding

Scope:

- plugin/module scaffolding,
- HarfBuzz dependency,
- SVG parser decision,
- shader intake with provenance,
- compiled blob format decision.

Blocks:

- all later milestones.

Can run in parallel:

- HarfBuzz `contrib` plugin,
- SVG parser evaluation,
- shader import and Slang port stub.

## Milestone M1: Minimal Renderer

Scope:

- Slang shader build,
- curve/band resource binding,
- one debug draw path in `he_scribe`,
- dynamic dilation in the vertex path,
- entrypoint-only resource bindings with helper logic kept separate.

Success signal:

- a known synthetic glyph renders correctly through `he_rhi`.

## Milestone M2: Font Asset Path

Scope:

- `ScribeFontFace` schema,
- font importer,
- font compiler,
- runtime loader,
- explicit fill-rule and band-packing metadata,
- documented curve/band texture formats, overlap epsilon, and sort-order rules.

Success signal:

- a font source imports, compiles, and renders from compiled resources with no direct source
  file dependency or FreeType dependency at runtime.
- compiled output preserves the ordering and flag assumptions expected by the shader path.
- compiled output preserves the reference packing contract closely enough for the shader path
  and captures metrics needed for later cap-height-aware UI sizing.

## Milestone M3: Text Layout

Scope:

- HarfBuzz shaping,
- fallback,
- bidi and line breaks,
- cluster mapping and hit testing basics.

Success signal:

- mixed-script text and combining-mark cases shape and lay out correctly.

## Milestone M4: Visual Testbed

Scope:

- a dedicated test app that opens a window and renders text through `he_scribe`,
- representative demo scenes for shaping, fallback, wrapping, and glyph rendering,
- a standing rule that new user-visible `scribe` features add or update a demo case in the
  testbed alongside unit coverage.

Success signal:

- a developer can launch a `scribe` test window and visually inspect real text rendering
  behavior without relying on editor or UI integration.

## Milestone M5: Color Fonts

Scope:

- layered color glyph rendering,
- palette management,
- variation-axis support,
- no pixel-shader loop over color layers.

Success signal:

- COLR/CPAL test cases render correctly and can be shaped through the normal text stack.

## Milestone M6: SVG Asset Path

Scope:

- `ScribeImage` schema,
- SVG importer/compiler,
- shared renderer path for vector scenes,
- paint evaluation kept separable from coverage evaluation.

Success signal:

- representative icons and illustrations render through the same coverage pipeline as text.
- even-odd and nonzero SVG fill cases survive the same compiled/runtime path cleanly.

## Milestone M7: Production Hardening

Scope:

- tests,
- perf work,
- memory/caching work,
- documentation and migration notes,
- regression coverage for packing, band reuse, and sizing rules.

Success signal:

- stable golden outputs, repeatable imports, and acceptable frame cost for realistic UI text.

## Critical Path

The real critical path is:

1. dependency decisions,
2. shader intake and renderer skeleton,
3. font asset pipeline,
4. shaping/layout,
5. visual testbed.

SVG and advanced color support can progress beside shaping once the canonical runtime format is
stable.

## Recommended First Vertical Slice

The first end-to-end slice should be intentionally narrow:

- import one monochrome TTF,
- compile one face to Slug resources,
- shape a short UTF-8 string with HarfBuzz,
- draw it through `he_scribe` in the dedicated `scribe` test app.

Do not start with SVG or full emoji support. That would expand the failure surface too early.

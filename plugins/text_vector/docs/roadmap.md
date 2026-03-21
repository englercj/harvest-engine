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
- one debug draw path in `he_text_vector`.

Success signal:

- a known synthetic glyph renders correctly through `he_rhi`.

## Milestone M2: Font Asset Path

Scope:

- `TextVectorFontFace` schema,
- font importer,
- font compiler,
- runtime loader.

Success signal:

- a font source imports, compiles, and renders from compiled resources with no direct source
  file dependency or FreeType dependency at runtime.

## Milestone M3: Text Layout

Scope:

- HarfBuzz shaping,
- fallback,
- bidi and line breaks,
- cluster mapping and hit testing basics.

Success signal:

- mixed-script text and combining-mark cases shape and lay out correctly.

## Milestone M4: Color Fonts

Scope:

- layered color glyph rendering,
- palette management,
- variation-axis support.

Success signal:

- COLR/CPAL test cases render correctly and can be shaped through the normal text stack.

## Milestone M5: SVG Asset Path

Scope:

- `TextVectorImage` schema,
- SVG importer/compiler,
- shared renderer path for vector scenes.

Success signal:

- representative icons and illustrations render through the same coverage pipeline as text.

## Milestone M6: Editor And UI Adoption

Scope:

- asset preview document,
- runtime API cleanup,
- `plugins/ui` proof-of-concept integration.

Success signal:

- UI text and icons render through `he_text_vector` instead of ad hoc font-atlas plumbing.

## Milestone M7: Production Hardening

Scope:

- tests,
- perf work,
- memory/caching work,
- documentation and migration notes.

Success signal:

- stable golden outputs, repeatable imports, and acceptable frame cost for realistic UI text.

## Critical Path

The real critical path is:

1. dependency decisions,
2. shader intake and renderer skeleton,
3. font asset pipeline,
4. shaping/layout,
5. UI adoption.

SVG and advanced color support can progress beside shaping once the canonical runtime format is
stable.

## Recommended First Vertical Slice

The first end-to-end slice should be intentionally narrow:

- import one monochrome TTF,
- compile one face to Slug resources,
- shape a short UTF-8 string with HarfBuzz,
- draw it through `he_text_vector` in an editor test view.

Do not start with SVG or full emoji support. That would expand the failure surface too early.

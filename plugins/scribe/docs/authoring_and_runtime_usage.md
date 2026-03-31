<!-- Copyright Chad Engler -->

# Authoring And Runtime Usage

This document records the current intended usage model for `he.scribe` after milestone `M7`.

## Source Assets

- Commit source fonts such as `.ttf`, `.otf`, `.ttc`, and `.otc` when they are needed for
  tests or demos.
- Commit source `.svg` files when they are needed for tests or demos.
- Do not commit compiled `scribe` runtime blobs or other generated binary payloads to the repo.
- Runtime code consumes compiled Harvest-owned blobs; it does not reopen source fonts or SVGs.

## Font Authoring Notes

- Preserve shaping-critical tables in source fonts because `he_scribe_layout` depends on the
  compiled shaping data derived from them.
- Preserve metrics such as `OS/2.sCapHeight` so later UI consumers can make cap-height-aware
  sizing decisions without reimporting source fonts.
- For color-font coverage, prefer representative COLR/CPAL assets in tests and demos.

## SVG Authoring Notes

- Prefer explicit `nonzero` or `evenodd` fill rules when the authored result depends on them.
- Treat authored SVG path/shape strokes as compile-time geometry. `scribe` imports them as
  compiled stroke layers that render through the normal compiled-shape path at runtime.
- Treat runtime image stroke overrides as a separate effect path. Those restrokes are generated
  from the stored stroke command stream on demand and cached by the runtime.
- Keep SVG `<text>` on the runtime text path unless the importer explicitly grows a
  text-to-outline mode later.
- Keep representative SVG samples small and intentional so compiler regressions are easy to
  spot in unit tests and in the test app.
- Treat unsupported SVG features as compile-time work items, not silent fallbacks.

## Runtime Usage Model

- `he_scribe_assets` loads versioned runtime blobs and exposes validated views.
- `he_scribe_layout` shapes and lays out text using compiled font blobs plus HarfBuzz.
- `he_scribe` renders compiled font and vector-image payloads through the shared coverage path.
- Runtime consumers should cache loaded blobs and renderer resources instead of recompiling
  source assets on demand.

## Visual Validation Rule

- The `he_scribe_test_app` is the default visual validation surface for user-visible `scribe`
  features.
- New user-visible features should add or update a demo case in the test app alongside unit
  coverage.
- Unit tests should lock down structural invariants; the test app should make rendering issues
  obvious during local development.

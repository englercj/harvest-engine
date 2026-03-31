<!-- Copyright Chad Engler -->

# M6 Status

Milestone `M6: SVG Asset Path` is complete.

## Implemented

- `ScribeImage` source/import metadata and compiled vector-image runtime blobs.
- SVG source import through `SvgImporter`.
- In-memory SVG compilation into Scribe curve/band payloads with:
  - `viewBox` handling,
  - path parsing for `M`, `L`, `H`, `V`, `Q`, `C`, `Z`,
  - group/path transforms,
  - flat fill colors,
  - `nonzero` and `evenodd` fill rules.
- Runtime loading and validation for compiled vector-image blobs.
- Shared renderer submission path for compiled vector shapes.
- `he_scribe_test_app` SVG demo scene using committed source SVGs only.
- Unit coverage for vector-image compile/load/runtime-shape resolution.

## Validation

- `he_test_runner.exe --filter "scribe:vector_image_pipeline"`
- `he_test_runner.exe --filter "scribe:runtime_blob"`
- `he_scribe_test_app.exe`

The visual testbed now renders the SVG demo scene correctly:

- left scene shows layered filled shapes and transformed content,
- right scene shows even-odd fill behavior through the same coverage path,
- no compiled binary SVG/font outputs are committed to the repo.

## Important Fixes Landed During M6

- Vector runtime resources now use authored SVG Y-space instead of the font-specific flipped glyph convention.
- SVG band texture packing now matches the established font/Slug contract:
  - horizontal-band headers first,
  - vertical-band headers second,
  - header `y` values contain payload offsets instead of zero.

That band-header layout bug was the main cause of the earlier broken visual output in the SVG test scene.

## Deferred

The current SVG path is intentionally narrow. Later milestones can expand support for:

- additional SVG element types such as `rect`, `circle`, `ellipse`, and `polygon`,
- strokes,
- gradients and more advanced paint servers,
- broader transform/style coverage,
- deduplication and caching improvements for larger illustration sets.

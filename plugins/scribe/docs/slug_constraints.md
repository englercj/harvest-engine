<!-- Copyright Chad Engler -->

# Slug Constraints

This document exists so Slug-derived implementation details do not get lost as `he.scribe`
expands beyond the initial shader port.

## Standing Constraints

- Keep dynamic dilation in the vertex path as the normal rendering mode.
- Keep resource bindings in the entrypoint shader module and keep imported helper modules free
  of binding declarations.
- Keep winding, coverage, and dilation math structurally close to the reference shaders unless
  a tested correctness fix requires deviation.
- Preserve nonzero and even-odd fill rules through import, compile, runtime loading, and draw
  submission.
- Preserve band and curve packing assumptions expected by shader traversal. Ordering,
  addressing, band maxima, and flags must be explicit compiler outputs.
- Store curve data in the canonical reference layout: four 16-bit floating-point channels per
  texel, with `(x1, y1, x2, y2)` in one texel and the shared third control point in the next
  texel so connected quadratic curves can reuse endpoints.
- Store band data in a two-channel 16-bit unsigned integer texture layout compatible with the
  reference traversal path.
- Choose band counts to minimize the worst per-band curve count, use a small overlap epsilon
  such as `1/1024` em when assigning curves to bands, and sort each band's curves in
  descending max-`x` or max-`y` order as required by the shader early-out logic.
- Support same-thickness bands per glyph, and preserve the option to deduplicate adjacent
  identical bands or point smaller bands at contiguous subsets of larger-band data.
- Render multicolor glyphs as layered draws or layered submissions, not as a loop inside the
  pixel shader.
- Keep paint evaluation separable from coverage evaluation so SVG and text share the same
  coverage implementation.
- Do not depend on runtime source parsing to recover font, SVG, or shaping metadata omitted
  from compiled assets.
- Do not adopt older supersampling or band-split variants as the default path without data
  showing they solve a real problem for Harvest.
- Remember that this path ignores hinting. UI-facing text APIs should preserve a future option
  for cap-height-aware pixel-grid sizing using `OS/2.sCapHeight` and monitor DPI.

## Milestone Mapping

- M1: dynamic dilation, entrypoint-only bindings, and close Slang parity with upstream helper
  math.
- M2: compiled runtime blob stores fill-rule bits, band metadata, and shader-facing lookup
  data, including curve/band packing details and sort-order assumptions.
- M3: shaping/layout continues to consume compiled assets only.
- M4: color glyph support uses layered submission.
- M5: SVG paint and coverage remain decoupled while sharing the same curve/band path.
- M7: regression tests lock in fill rules, band packing invariants, band reuse cases,
  cap-height sizing behavior, and layered color output.

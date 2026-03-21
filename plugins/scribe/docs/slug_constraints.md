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
- Render multicolor glyphs as layered draws or layered submissions, not as a loop inside the
  pixel shader.
- Keep paint evaluation separable from coverage evaluation so SVG and text share the same
  coverage implementation.
- Do not depend on runtime source parsing to recover font, SVG, or shaping metadata omitted
  from compiled assets.
- Do not adopt older supersampling or band-split variants as the default path without data
  showing they solve a real problem for Harvest.

## Milestone Mapping

- M1: dynamic dilation, entrypoint-only bindings, and close Slang parity with upstream helper
  math.
- M2: compiled runtime blob stores fill-rule bits, band metadata, and shader-facing lookup
  data.
- M3: shaping/layout continues to consume compiled assets only.
- M4: color glyph support uses layered submission.
- M5: SVG paint and coverage remain decoupled while sharing the same curve/band path.
- M7: regression tests lock in fill rules, band packing invariants, and layered color output.

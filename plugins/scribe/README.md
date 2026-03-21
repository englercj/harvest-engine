<!-- Copyright Chad Engler -->

# Harvest Scribe Plugin Plan

This folder captures the implementation plan for a new Harvest text/vector rendering plugin
whose primary technical reference point is the Slug algorithm and its public reference
shaders.

The proposed working plugin name is `he.scribe`, implemented in `plugins/scribe`,
with `plugins/ui` consuming it later instead of owning text and vector rendering directly.

## Goals

- Full color GPU glyph rendering.
- Full color GPU vector rendering.
- Asset import and compile support for fonts and SVGs.
- Text shaping through HarfBuzz.
- A reusable runtime that the new Harvest UI library can call directly.
- No FreeType dependency in the runtime. Runtime code consumes compiled Harvest-owned data only.

## Document Index

- `docs/architecture.md`
  - Overall plugin architecture, module boundaries, runtime data flow, and integration seams.
- `docs/implementation_plan.md`
  - Step-by-step build plan with concrete work packages and exit criteria.
- `docs/roadmap.md`
  - Milestones, dependency order, and what can run in parallel.
- `docs/asset_pipeline.md`
  - Proposed asset types, importer/compiler flow, and compiled runtime formats.
- `docs/support_matrix.md`
  - Planned feature coverage for font formats, emoji, SVG features, and text layout.
- `docs/references_and_intake.md`
  - Source references, shader intake policy, and legal/source-tracking notes.
- `docs/slug_constraints.md`
  - Cross-cutting Slug-derived implementation reminders and gotchas to preserve across later milestones.
- `docs/m0_status.md`
  - Concrete milestone M0 decisions, dependency choices, and scaffold status.
- `docs/m2_status.md`
  - Current milestone M2 implementation status, what is complete, and what is intentionally deferred to later milestones.
- `docs/m3_status.md`
  - Current milestone M3 implementation status for shaping, fallback, wrapping, cluster mapping, and hit testing basics.

## Working Assumptions

- `he_rhi` remains generic and does not become Slug-specific.
- `he_assets` and `he_assets_editor` stay the orchestration layer for source import and
  compile.
- `plugins/ui` is treated as a downstream consumer because it is effectively a placeholder
  in this checkout.
- The reference shaders are adapted with minimal algorithmic drift and wrapped in Harvest-
  specific bindings, build rules, and resource loading.
- FreeType is acceptable in importer/compiler code, but not in the runtime text or vector
  engine.
- Compiled font and vector payloads should be self-contained Harvest blobs, likely described
  by `he_schema`, similar in role to Slug's `.slug` files but owned by Harvest.

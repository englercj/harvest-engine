<!-- Copyright Chad Engler -->

# Milestone M1 Status

This document records the concrete implementation choices made during milestone M1.

## Completed In This Milestone

- Added `shader_compile` wiring for `he_scribe`.
- Added first-party Slang ports of the imported Slug reference shader logic.
- Replaced the runtime `Renderer` placeholder with a small `he_rhi`-backed renderer.
- Added runtime glyph resource creation for:
  - packed Slug-style quad vertices,
  - curve texture payloads,
  - band texture payloads,
  - descriptor tables for `t0`/`t1`.
- Added a synthetic debug glyph resource helper that uploads a known square-outline test payload.
- Added a frame API that can:
  - begin a frame against an existing render command list and target,
  - queue glyph draws,
  - emit one render pass through `he_rhi`.

## Shader Port Strategy

M1 keeps the imported Slug logic close to upstream:

- upstream files remain in `third_party/slug_reference/`,
- first-party Slang source lives in `runtime/src/shaders/`,
- the shader logic is now compiled from local Slang files instead of directly including HLSL from `third_party/`,
- the algorithmic structure still tracks the imported reference implementation,
- `slug.slang` holds the shared Slug helper logic while `scribe.slang` owns the resource bindings and entrypoints.

This keeps provenance obvious and keeps M1 focused on build integration rather than algorithm
rewriting.

## Runtime Shape

The M1 renderer shape is intentionally narrow:

- external `rhi::Device` ownership,
- external frame command list ownership,
- plugin-owned pipeline/root-signature/vertex-format objects,
- plugin-owned glyph resources for synthetic testing.

The first path uses:

- one vertex constant block for the MVP rows plus viewport dimensions,
- one pixel descriptor table containing the curve and band textures,
- one packed quad vertex stream that matches the upstream Slug vertex contract.

## Deferred Work

M1 intentionally does not yet include:

- compiled font assets,
- compiled SVG assets,
- layout/shaping integration,
- color-font layers,
- generic batching,
- importer/editor preview wiring.

Those remain in later milestones.

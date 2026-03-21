<!-- Copyright Chad Engler -->

# Milestone M0 Status

This document records the concrete implementation choices made during milestone M0.

## Completed In This Milestone

- Replaced the docs-only `plugins/scribe` manifest with real module scaffolding.
- Added a schema-backed runtime blob stub so the runtime/data boundary exists in code.
- Added a HarfBuzz contrib plugin definition.
- Chose LunaSVG as the SVG parser direction for compiler/importer work.
- Added shader intake provenance files and a dedicated third-party staging location.
- Installed HarfBuzz through `./hemake.ps1 install-plugins`.
- Verified targeted Windows builds for:
  - `harfbuzz`
  - `he_scribe`
  - `he_scribe_layout`
  - `he_scribe_assets__schemac_1`
  - `he_scribe_assets`

## Dependency Decisions

### Text Shaping

- Library: HarfBuzz
- Harvest module name: `harfbuzz`
- Runtime policy: allowed
- FreeType bridge policy: forbidden in runtime

Rationale:

- HarfBuzz is the required shaping engine.
- The runtime can shape from compiled Harvest-owned blobs without linking FreeType.
- The MSVC build requires `/bigobj` for the amalgamated `harfbuzz.cc` translation unit.

### SVG Parsing

- Chosen direction: LunaSVG
- Runtime policy: forbidden
- Compiler/importer policy: allowed

Rationale:

- It is a much better feature fit than ultra-minimal parsers for gradients, transforms,
  styling, and general SVG import fidelity.
- The actual contrib wiring is deferred until dependency closure around `plutovg` is confirmed.

## Runtime Blob Direction

The runtime blob direction chosen in M0 is:

- Harvest-owned format,
- versioned,
- schema-backed,
- self-contained for runtime layout and rendering,
- no raw `.ttf`, `.otf`, or `.svg` parsing in runtime modules.

The first schema stub lives at:

- `assets/include/he/scribe/schema/runtime_blob.hsc`

This is intentionally a wrapper schema for payload groups, not a final optimized format.

## Intake Status

The Slug reference shader intake location is:

- `third_party/slug_reference/`

M0 tracks provenance there. Importing exact upstream shader source is attempted as part of
validation, and the upstream HLSL, notice, and MIT license files were successfully imported.

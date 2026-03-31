# Scribe Stroke Implementation Handoff

## Goal

Implement proper stroked text outlines in Scribe.

The current outline effect is not acceptable for shipping quality because it is built from repeated offset draws, which produces stepped corners and lumpy curves.

The agreed target is:

- keep the `no FreeType at runtime` goal
- do not bake multiple stroke widths into compiled assets
- store enough source outline data in the compiled font resource to build true stroked geometry at runtime
- cache stroked results in Scribe once generated

## Agreed Direction

### What we are not doing

- not scaling the fill glyph and drawing it behind the text
- not using repeated translated glyph copies as the final outline technique
- not depending on FreeType stroker at runtime
- not baking pre-stroked geometry for multiple widths into the asset blob

### What we are doing

1. Extend the compiled font resource to store a compact source-outline command stream per glyph.
2. At runtime, build stroked geometry from that outline command stream for the requested stroke settings.
3. Convert the stroked result into the same kind of curve/band render data Scribe already uses.
4. Cache the stroked result in the runtime context so repeated use of the same glyph/style combination is cheap.

## Why This Direction

Outline width is a runtime style property, so baking many widths is wasteful.

The current Scribe runtime is already moving toward a central `ScribeContext` that owns:

- registered font/image resources
- HarfBuzz font state
- runtime glyph/image caches
- renderer/layout helpers

That makes it a natural place to also own stroked-glyph caches.

## Important Current Architecture

Recent refactors changed Scribe substantially. The next agent should assume the following:

- `LayoutEngine` now lives under `plugins/scribe/runtime/src/layout_engine.cpp`
- `Renderer` implementation now lives under `plugins/scribe/runtime/src/renderer.cpp`
- `ScribeContext` is the central runtime object
- registered font/image resources are keyed by handle
- glyph/image runtime caches are sparse and lazy
- default `Renderer` / `LayoutEngine` instances are context-owned and exposed through getters

Relevant files:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/include/he/scribe/context.h`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/src/context.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/include/he/scribe/layout_engine.h`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/src/layout_engine.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/include/he/scribe/renderer.h`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/src/renderer.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/src/compiled_font.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/include/he/scribe/compiled_font.h`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\editor\src\font_compile_geometry.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\editor\src\font_compile_geometry.h`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\assets/include/he/scribe/schema/scribe_assets.hsc`

## Current Outline Behavior

Current retained text outlines are still effect draws built from extra translated copies of the base glyph.

Relevant file:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime/src/retained_text.cpp`

When replacing the current behavior, this is the area that should stop emitting offset-copy outlines and instead request/use a real stroked glyph resource.

## Recommended Design

### 1. Add source outline commands to the compiled font resource

The compiled runtime resource currently stores:

- shaping bytes
- metadata
- render data
- paint data
- packed curve texels
- packed band texels

It does not currently preserve enough contour topology to reconstruct a proper stroke.

Add a new per-glyph outline-command stream that preserves:

- contour starts / ends
- move commands
- line segments
- quadratic segments
- cubic segments if needed, or flatten/convert consistently at compile time

The representation should be compact and schema-defined.

Likely schema areas:

- `ScribeFontFace::RuntimeResource`
- glyph render/metadata child types in `scribe_assets.hsc`

Avoid introducing more opaque raw blobs when a schema struct/list can represent the data clearly.

### 2. Keep the compiler responsible for outline extraction, not stroking

The editor/compiler side should:

- load font outlines with FreeType
- extract normalized contour commands
- store them in the runtime resource

It should not:

- build stroke widths
- emit per-style stroked payloads

### 3. Build a runtime stroker inside Scribe

The runtime stroker should operate on the stored outline-command stream and support at least:

- stroke width
- join style
- miter limit
- probably butt/square/round cap support for completeness, though text glyphs are mostly closed contours

The likely first milestone is:

- closed contour glyph stroking
- round/bevel/miter joins
- no separate open-path cap complexity unless needed by vector/image reuse later

### 4. Reuse existing curve/band compilation as much as possible

Do not create a second renderer.

The best path is:

- stroke outline commands
- produce stroked contours
- feed those contours into the same curve/band packing pipeline already used for fills

That likely means extracting more shared compile/runtime geometry helpers rather than adding a parallel special-case path.

## Suggested Implementation Order

### Phase 1: Runtime resource schema extension

1. Add schema types for outline commands and per-glyph outline ranges.
2. Update the font compiler to emit those commands.
3. Add runtime-resource tests that verify the outline command stream survives compile/load.

### Phase 2: Runtime stroker prototype

1. Add a runtime stroker module under `plugins/scribe/runtime/src`.
2. Build stroked contours from the stored outline commands.
3. Add CPU-only unit tests for:
   - simple rectangle
   - simple triangle
   - round contour
   - sharp join / miter behavior
   - inner contour / counter handling

This phase should avoid any renderer integration until the geometry is trustworthy.

### Phase 3: Convert stroked contours into Scribe render data

1. Produce packed curve/band data for the stroked geometry.
2. Add a runtime cache keyed by:
   - `FontFaceHandle`
   - glyph index
   - stroke width
   - join settings
   - miter limit
   - any other style-affecting stroke parameters

This probably belongs in `ScribeContext`.

### Phase 4: Retained text integration

1. Replace the offset-copy outline effect path in `retained_text.cpp`.
2. Make outline draws request stroked glyph resources from context.
3. Keep fill and outline as separate draws, but both should use true glyph geometry.

## Cache Guidance

The recent runtime review direction strongly favors sparse/lazy caches.

So for stroked outlines:

- do not prebuild all stroked glyphs
- do not allocate dense per-glyph stroked caches
- build on demand
- cache only what has actually been used

Context is the correct cache owner.

## Testing Guidance

Add both CPU-only and runtime tests.

Recommended coverage:

- schema/resource test:
  - outline command stream loads correctly
- CPU stroker tests:
  - expected contour count / join behavior
  - counters stay valid
  - no self-intersection for common cases
- runtime integration tests:
  - retained outline draw count/path changes
  - outline is no longer emitted as N translated copies
  - basic renderer preparation succeeds after source storage lifetime ends

Relevant test files to extend:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\test\test_color_font_pipeline.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\test\test_runtime_blob.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\test\test_line_curve_utils.cpp`

Likely new test files:

- `plugins/scribe/test/test_stroker.cpp`
- `plugins/scribe/test/test_outline_command_stream.cpp`

## Things To Watch Out For

### 1. Do not let stroke geometry bypass the main renderer contract

If stroked glyphs produce different render-side assumptions than normal glyphs, maintenance cost will spike quickly.

### 2. Counter handling matters

A real outline is not just an outer offset. Interior contours need correct winding / subtraction behavior.

### 3. Join quality matters more than raw correctness

The user specifically cares about the visible quality of:

- sharp corners
- round arcs

So even if a stroker is geometrically valid, poor join approximations will still fail the visual bar.

### 4. Avoid rebuilding FreeType-like abstractions accidentally

This should be a compact Scribe-specific runtime stroker over stored commands, not a giant general-purpose font IR rewrite.

## Build / Verification Commands

Use direct MSBuild for generated vcxproj builds:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    ".build\projects\he_scribe.vcxproj" `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /m:1
```

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    ".build\projects\he_scribe_test_app.vcxproj" `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /m:1
```

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    ".build\projects\he_test_runner.vcxproj" `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /m:1
```

Focused tests:

```powershell
.build\win64-debug\bin\he_test_runner.exe --filter scribe:layout_engine
.build\win64-debug\bin\he_test_runner.exe --filter scribe:retained_text
.build\win64-debug\bin\he_test_runner.exe --filter scribe:runtime_resource
```

## Immediate Next Step

The best next step is:

1. extend the font runtime resource schema with an outline command stream
2. emit that data from the compiler
3. add a small CPU-only stroker prototype and tests before touching the retained outline rendering path

That gives a stable foundation before integrating with the renderer.

# Capital T Artifact Investigation Handoff

## Problem

There is still a visible thin vertical gray/black bar to the left of some capital `T` glyphs in the Scribe test app. The user has repeatedly confirmed that it is still visible after several compiler-side fixes.

The latest reference screenshot from the user shows:

- a detached vertical strip to the left of the `T`
- asymmetric antialiasing on the `T` stem
- the issue persists even after the forward-slash artifact was fixed

The working assumption from the user is:

1. The Slug-derived shader code is correct and should not be treated as the likely source of the bug.
2. The bug is more likely in how Scribe prepares data for the shader, or how the runtime uses that data.
3. `msbuild` must be used directly for generated `.vcxproj` files. `dotnet msbuild` is not acceptable here.

## Investigation Summary

### 1. Compiler-side line encoding was examined first

The earlier detached `W` and `/` artifacts were traced to how straight outline segments were encoded as quadratics. That class of issue was real, and a number of line-encoding changes were made over time.

Most recently, the line encoder was simplified again so line quadratics stay *exactly on the original segment* while still avoiding the exact-midpoint case:

- `plugins/scribe/editor/src/line_curve_utils.h`
- `plugins/scribe/editor/src/font_compile_geometry.cpp`

Current behavior:

- line control points are biased **along the segment tangent**
- the old extra **normal-axis pack-stability nudge** was removed
- font glyph line curves are no longer post-adjusted using contour-orientation-dependent inward nudges

The intent of this change was to remove the off-stem bow that was still present in the compiled line data.

### 2. The shared line tests were updated

`plugins/scribe/test/test_line_curve_utils.cpp` was updated to reflect the intended contract:

- vertical line controls stay on the `x` axis of the stem
- horizontal line controls stay on the `y` axis of the stem
- axis-aligned and diagonal lines are offset from the exact midpoint, not forced off-axis

Current targeted line tests:

- `scribe:line_curve_utils:keeps_horizontal_line_controls_on_the_stem_axis`
- `scribe:line_curve_utils:keeps_vertical_line_controls_on_the_stem_axis`
- `scribe:line_curve_utils:offsets_axis_aligned_and_diagonal_lines_off_the_midpoint`

These tests currently pass.

### 3. A focused compiled-glyph coverage test was used to inspect the `T`

The main diagnostic test is still:

- `plugins/scribe/test/test_color_font_pipeline.cpp`
- test name: `scribe:color_font_pipeline:compiled_capital_t_has_no_detached_left_edge_coverage`

This test is instrumented with detailed logging:

- compiled glyph bounds
- outline bounds
- horizontal/vertical band payloads
- per-curve data for the sampled band
- a coverage slice across the left edge of the glyph

After removing the off-axis line nudge, the logged left-edge `T` curve now looks like this:

- `p1 = (10, 635)`
- `p2 = (10, 675)`
- `p3 = (10, 714)`

So the line is now compiled as a perfectly vertical quadratic, which is better than the previous `p2.x = 10.5` case.

However, the test still fails.

Latest observed diagnostic output from the focused run:

- `bounds_min_x = 10`
- `bounds_min_y = 0`
- `bounds_max_x = 545`
- `bounds_max_y = 714`
- `outline_min_x = 10`
- `max_coverage_outside_left = 0.47600001096725464`
- `sample_x = 9.75`
- `sample_y = 641.484375`

The failing horizontal curve is now:

- `p1x = 10`
- `p2x = 10`
- `p3x = 10`
- `code = 256`
- `root0 = 0.024000000208616257`

The coverage slice near the left edge is still a monotonic ramp from roughly `0.308` at `x = 8` to `1.0` by roughly `x = 15.25` for the chosen diagnostic scale.

### 4. Important interpretation of that test result

That failing test is no longer strong evidence of a compiler bow. After the latest change, the sampled left edge is a straight vertical line, yet the test still reports nonzero outside-left coverage.

That means one of these is true:

1. The test is flagging expected edge antialiasing instead of the real visual artifact.
2. The real visual artifact is a **placement / transform / pixel-phase** issue in runtime rendering, not a line-curve preparation issue.
3. There is still a runtime contract mismatch between geometry placement and sample-coordinate placement, even when the compiled curve itself is correct.

By the end of the last investigation pass, the most likely next area was considered to be:

- **runtime placement / AA / transform behavior**

not shader code, and not obviously the compiler bow that had already been removed.

## Runtime Investigation Notes

The runtime path that looked most relevant was:

- `plugins/scribe/runtime/src/scribe_renderer.cpp`
- `plugins/scribe/runtime/src/shaders/scribe.slang`
- `plugins/scribe/runtime/src/shaders/slug.slang`
- `plugins/scribe/runtime/src/compiled_font.cpp`
- `plugins/scribe/runtime/src/retained_text.cpp`

### TransformVertex suspicion

`TransformVertex()` in `plugins/scribe/runtime/src/scribe_renderer.cpp` was identified as suspicious and worth deeper review:

```cpp
const float a00 = draw.size.x * draw.basisX.x;
const float a01 = draw.size.x * draw.basisY.x;
const float a10 = draw.size.y * draw.basisX.y;
const float a11 = draw.size.y * draw.basisY.y;
```

This matrix construction looks inconsistent for the off-diagonal terms. A more natural column-wise construction would usually be:

- `a00 = draw.size.x * draw.basisX.x`
- `a01 = draw.size.y * draw.basisY.x`
- `a10 = draw.size.x * draw.basisX.y`
- `a11 = draw.size.y * draw.basisY.y`

Likewise, the translation currently uses:

```cpp
offsetX = draw.position.x + (draw.size.x * draw.offset.x);
offsetY = draw.position.y + (draw.size.y * draw.offset.y);
```

which ignores the basis vectors and only works cleanly when the basis is identity-like.

That said, for plain axis-aligned uniform scaling, this bug can collapse away numerically, so it was not yet proven to explain the current `T` artifact. It remains one of the most important runtime suspects.

### Why runtime placement became the leading suspect

At the end of the last pass, the thinking was:

- the `T` line curve is now straight
- the remaining artifact still looks like a detached one-pixel column
- the user specifically noted that the `T` stem looks shifted / aliased asymmetrically

That combination is consistent with:

- geometry position and texcoord/sample position disagreeing by about a pixel
- or a fractional placement / AA phase issue
- or a bug in transformed retained text placement

## Files Changed During This Investigation

These are the main files that were touched during the latest `T` artifact pass:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\editor\src\line_curve_utils.h`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\editor\src\font_compile_geometry.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\test\test_line_curve_utils.cpp`
- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\test\test_color_font_pipeline.cpp`

There were also some earlier app-side snapping experiments in:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\test_app\scribe_test_app.cpp`

Those were not considered the real fix, and the last investigation pass had moved away from blaming the app scene itself.

## Commands / Verification Used

### Build the test runner

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    ".build\projects\he_test_runner.vcxproj" `
    /t:Build `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /p:PlatformToolset=v143 `
    /m:1
```

### Build the test app

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    ".build\projects\he_scribe_test_app.vcxproj" `
    /t:Build `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /p:PlatformToolset=v143 `
    /m:1
```

### Run the focused line tests

```powershell
& ".build\win64-debug\bin\he_test_runner.exe" --filter "scribe:line_curve_utils"
```

This currently passes.

### Run the focused `T` coverage test

```powershell
& ".build\win64-debug\bin\he_test_runner.exe" --filter "compiled_capital_t_has_no_detached_left_edge_coverage"
```

This currently still fails, with the diagnostics described above.

## Most Useful Next Steps

These were the next investigation steps that had become highest priority:

### 1. Verify whether the visual strip is geometry/sample misalignment

Focus on:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime\src\scribe_renderer.cpp`

Especially:

- `TransformVertex()`
- how `draw.position`, `draw.size`, `draw.basisX`, `draw.basisY`, and `draw.offset` are composed
- how `out.pos.xy`, `out.pos.zw`, and `out.jac` relate to the original compiled glyph quad

This is the best current lead.

### 2. Re-check retained text placement for the exact `TextSub1` scene

Focus on:

- `C:\Users\engle\source\repos\harvest-engine\plugins\scribe\runtime\src\retained_text.cpp`

Specifically:

- `draw.position`
- `draw.size`
- `draw.basisX`
- `draw.basisY`
- `draw.offset`

and how these are later consumed by `Renderer::QueueRetainedText()`.

### 3. If needed, add a renderer-side regression test or instrumentation

A useful next diagnostic would be something like:

- dump the transformed glyph quad vertices for the problematic `T`
- dump the corresponding `tex.xy` and `jac`
- compare left-edge geometry position against the intended sample-space left edge

The current compiler diagnostics are probably no longer enough by themselves.

### 4. Do not restart from the shader

The user has been explicit about this and the current evidence supports it:

- keep treating the shader as correct unless a runtime/compile contract bug proves otherwise

## Bottom Line

At the end of this session:

- the obvious compiler bow on straight lines has been removed
- the forward-slash issue was improved earlier
- the capital `T` bar is still visible to the user
- the strongest remaining lead is now **runtime placement / transform / AA phase**, not the shader and not the latest line-control math


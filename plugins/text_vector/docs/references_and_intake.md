<!-- Copyright Chad Engler -->

# References And Intake Notes

## Primary References

### 2017 JCGT Paper

- Title: `GPU-Centered Font Rendering Directly from Glyph Outlines`
- Author: Eric Lengyel
- Source: `https://jcgt.org/published/0006/02/02/paper.pdf`

Key takeaways for Harvest:

- the core method renders glyphs directly from curve data instead of texture atlases,
- robustness comes from winding-number logic that avoids floating-point artifact cases,
- the baseline representation is curve data plus band data,
- the paper already accounts for kerning, ligatures, and combining marks in processed font data.

### 2019 Dynamic Dilation Post

- Title: `Dynamic Glyph Dilation`
- Source: `https://terathon.com/blog/glyph-dilation.html`

Key takeaway:

- dilation belongs in the vertex stage and should be derived per vertex from the current MVP
  matrix and viewport dimensions, not treated as a fixed author-controlled padding value.

### March 17, 2026 Slug Update

- Title: `A Decade of Slug`
- Source: `https://terathon.com/blog/decade-slug.html`

Key takeaways:

- the Slug patent was dedicated to the public domain effective March 17, 2026,
- dynamic dilation is now the preferred path,
- supersampling and band-split details from the 2017 era are not the preferred modern shape,
- multicolor emoji should be handled as layered glyph draws rather than a pixel-shader loop,
- the author published reference vertex and pixel shaders derived from the real library code.

### Reference Shader Repository

- Source: `https://github.com/EricLengyel/Slug`
- Relevant files:
  - `SlugVertexShader.hlsl`
  - `SlugPixelShader.hlsl`

Observed notes from the published repo:

- the vertex shader file carries `SPDX-License-Identifier: MIT OR Apache-2.0`,
- the pixel shader file header says the code is available under the MIT License,
- the README states the repository contains reference shader implementations and asks for credit
  when distributed.

## Intake Policy

The implementation should not silently rewrite the algorithm while porting it. Use this policy:

1. Check in upstream reference files in a clearly labeled third-party intake area.
2. Preserve upstream notices and source URLs.
3. Port to Slang in thin wrapper files that keep algorithmic structure as close as practical to
   upstream.
4. Record the upstream commit or snapshot date used for intake.
5. Add local comments only where Harvest-specific bindings differ from upstream.

## Proposed Repo Layout For Intake

When implementation starts, add something close to:

- `plugins/text_vector/third_party/slug_reference/SlugVertexShader.hlsl`
- `plugins/text_vector/third_party/slug_reference/SlugPixelShader.hlsl`
- `plugins/text_vector/src/shaders/slug_common.slang`
- `plugins/text_vector/src/shaders/slug_vertex.slang`
- `plugins/text_vector/src/shaders/slug_pixel.slang`
- `plugins/text_vector/NOTICE`

## Binding Strategy

Because Harvest already uses Slang through `shader_compile`, the plan should be:

- keep upstream HLSL as the provenance artifact,
- adapt entry points and bindings into Slang,
- isolate Harvest-specific resource layout in wrapper code,
- avoid changing the winding, coverage, and dilation math unless a correctness issue is proven.

## Runtime Data Ownership

The runtime should consume compiled Harvest-owned data, not raw font files and not FreeType
faces.

The nearest analogy is Slug's compiled `.slug` file, but Harvest should own its own blob
format. The preferred direction is a versioned `he_schema`-backed compiled asset that stores:

- glyph outlines,
- shader optimization data,
- kerning and shaping metadata,
- color-layer data,
- vector paint data,
- any lookup tables needed by runtime layout and rendering.

## Legal Note

This document is an engineering intake note, not legal advice.

The practical rule for implementation work should be:

- treat the upstream shaders as third-party source,
- preserve the published notices,
- keep provenance attached to any copied or closely adapted files,
- prefer documenting the exact upstream snapshot used before code is imported verbatim.

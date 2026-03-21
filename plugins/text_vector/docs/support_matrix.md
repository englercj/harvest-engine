<!-- Copyright Chad Engler -->

# Support Matrix

## Fonts

### Initial Required Support

- `.ttf`
- `.otf`
- `.ttc` and `.otc` with face selection
- UTF-8 text input
- kerning
- ligatures
- combining marks
- composed and decomposed Unicode sequences
- compiled-runtime consumption only in the engine path

### Planned Early Support

- variation fonts
- COLR/CPAL layered color glyphs
- per-script fallback chains
- mixed-direction paragraphs

### Deferred Until Explicitly Implemented

- CBDT/CBLC bitmap emoji
- `sbix`
- SVG-in-OpenType glyph documents
- WOFF and WOFF2 as direct source formats

Those can be added later, but they should not be treated as part of the first production
slice unless the dependency choice and runtime budget make them cheap.

## Text Layout

### Required

- HarfBuzz shaping
- grapheme-aware cluster tracking
- bidi support
- line breaking
- hit testing
- selection and caret movement metadata

### Clarification

HarfBuzz covers shaping, but not the entire text engine. The plugin still needs a bidi and
segmentation strategy around HarfBuzz.

### Runtime Constraint

The runtime text engine must not depend on FreeType or raw source font parsing. All shaping
and render inputs must come from compiled Harvest-owned blobs.

## SVG

### Initial Required Support

- `path`
- `rect`
- `circle`
- `ellipse`
- `line`
- `polyline`
- `polygon`
- transforms
- nonzero and even-odd fill rules
- solid fills
- linear and radial gradients
- basic strokes
- opacity

### Planned Later Support

- clip paths
- masks
- patterns
- filters
- text-in-SVG
- external asset references

The initial delivery should focus on the vector graphic subset needed by the new UI library,
not the entire SVG specification.

## Color Strategy

### First Color Target

- layered vector color from COLR/CPAL and SVG paint data.

### Rendering Rule

- each color layer becomes an explicit draw item with its own bounds and paint state.

This matches the post-2019 Slug direction and is better than putting layer loops inside the
pixel shader.

## Platform Targets

### First Target

- Windows via current D3D12 `he_rhi` backend.

### Later Targets

- Linux once the `he_rhi` backend and shader pipeline are in place.

Do not design Windows-only assumptions into asset formats, text layout, or renderer public
APIs.

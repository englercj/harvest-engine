<!-- Copyright Chad Engler -->

# Asset Pipeline Plan

## Asset Types

The plugin should introduce these first asset types:

### `ScribeFontFace`

Represents one importable font face or one selected face from a collection.

Suggested fields:

- source metadata,
- family/subfamily/postscript names,
- style traits,
- units per em,
- ascent/descent/line gap,
- variation axis defaults and overrides,
- fallback tags,
- supported color formats,
- compile options.

### `ScribeFontFamily`

Represents a logical family used by layout and fallback policy.

Suggested fields:

- primary faces,
- fallback chain,
- script/language overrides,
- emoji fallback chain,
- default feature toggles.

### `ScribeImage`

Represents one imported SVG scene.

Suggested fields:

- source view box and bounds,
- authored size,
- paint and gradient metadata,
- compile options for precision and path conversion.

## Importer Outputs

Importers should create both editable assets and cache resources.

### Font Import Cache Resources

- raw face metadata,
- outline records,
- glyph metrics,
- GSUB/GPOS and other shaping-table extracts,
- color-font table extracts,
- named instances and axis metadata.

### SVG Import Cache Resources

- parsed scene graph,
- normalized path list,
- paint graph,
- gradients and stops,
- stroke metadata,
- import diagnostics.

The compiler should consume cache resources rather than reopening the original source file.

## Compiler Outputs

Compiled runtime resources should be plugin-owned Harvest blobs with explicit versioning.

Preferred direction:

- define these payloads with `he_schema`,
- serialize them as self-contained runtime blobs,
- keep enough structure that `he_scribe_layout` and `he_scribe` can load them directly
  without source-font parsing or ad hoc binary readers.

Suggested payload groups:

- `he.scribe.curves`
  - curve texture bytes and packing metadata.
  - default target layout should match the reference expectation: four 16-bit floating-point
    channels per texel storing `(x1, y1, x2, y2)`, with connected quadratics sharing the
    endpoint carried in the next texel.
- `he.scribe.bands`
  - band texture bytes and packing metadata.
  - default target layout should match the reference expectation: two 16-bit unsigned integer
    channels per texel plus metadata describing band counts, maxima, overlap epsilon, and any
    indirection used for deduplicated or subset bands.
- `he.scribe.shapes`
  - glyph/shape bounds, offsets, winding flags, and layer references.
- `he.scribe.paints`
  - solid colors, gradients, palette tables, opacity, and stroke style data.
- `he.scribe.layout`
  - face metrics, glyph advances, fallback metadata, and lookup tables needed at runtime.
- `he.scribe.shaping`
  - the shaping data HarfBuzz or the runtime shaping layer needs without touching the original
    font source.

## Canonical Compile Rules

### Fonts

- quadratic TrueType outlines pass through mostly unchanged,
- cubic outlines are approximated into quadratic segments during compile,
- color glyph layers are preserved as layered shape draws,
- shaping tables are preserved in data forms needed by `he_scribe_layout`,
- import preserves metrics such as `OS/2.sCapHeight` that may be used later for pixel-grid
  sizing guidance in UI code.

### Band Generation Rules

- Each glyph can choose its own horizontal and vertical band counts.
- Band counts should be selected to reduce the maximum curve count in any one band.
- Curve-to-band assignment should use a small overlap epsilon, with `1/1024` em as the
  baseline reference value unless testing shows Harvest needs a different default.
- Curves inside horizontal bands must be sorted in descending maximum `x`.
- Curves inside vertical bands must be sorted in descending maximum `y`.
- All bands for one glyph should have the same thickness.
- The compiler should support reusing identical adjacent-band data and contiguous subsets of
  larger-band data when that reduces size and preserves traversal semantics.

### SVG

- lines, rects, circles, ellipses, and arcs are converted to path segments,
- cubic paths are approximated into quadratic segments,
- authored path/shape strokes are expanded during compile and emitted as compiled stroke layers,
- runtime stroke command data is kept so image-level restrokes can still be generated on demand,
- transforms are baked or normalized depending on reuse opportunities,
- unsupported features fail with explicit diagnostics instead of silent corruption.

## Import Settings

Font importer settings should likely include:

- face index for TTC/OTC,
- variation axis defaults,
- compile precision profile,
- fallback classification tags,
- color-font enable/disable policy.

SVG importer settings should likely include:

- flattening tolerance for unsupported primitives,
- stroke expansion policy,
- gradient precision policy,
- unsupported-feature failure mode.

## Runtime Loading

Runtime loaders in `he_scribe_assets` should:

- load resource headers,
- validate versions,
- create CPU-side descriptors for render resources,
- hand immutable payload views to `he_scribe` and `he_scribe_layout`.

The UI runtime should never need the editor or asset database to use compiled resources.

## Invalidations And Versioning

The current shared asset system does not appear to fully persist importer/compiler version
invalidation metadata yet, so `he.scribe` should explicitly plan for:

- schema version fields,
- compiler format version fields,
- renderer compatibility checks,
- importer/compiler version tests once the shared invalidation path is strengthened.

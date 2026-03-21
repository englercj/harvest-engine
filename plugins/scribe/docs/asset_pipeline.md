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
- `he.scribe.bands`
  - band texture bytes and packing metadata.
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
- shaping tables are preserved in data forms needed by `he_scribe_layout`.

### SVG

- lines, rects, circles, ellipses, and arcs are converted to path segments,
- cubic paths are approximated into quadratic segments,
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

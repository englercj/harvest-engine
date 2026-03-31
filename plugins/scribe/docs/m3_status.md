<!-- Copyright Chad Engler -->

# Milestone M3 Status

## Implemented

- `he_scribe_layout` now exposes a real public runtime API in:
  - `he/scribe/layout_engine.h`
  - `layout/src/layout_engine.cpp`
- `LayoutEngine::LayoutText()` now:
  - shapes directly from compiled `LoadedFontFaceBlob` inputs,
  - uses HarfBuzz at runtime without any FreeType bridge,
  - selects fallback faces from the provided face span by nominal glyph coverage,
  - segments paragraphs by newline,
  - performs basic script-aware run itemization,
  - applies line wrapping on measured cluster advances,
  - preserves HarfBuzz cluster boundaries for runtime mapping,
  - records line, cluster, and glyph placement data for downstream rendering.
- `LayoutEngine::HitTest()` now provides:
  - line selection,
  - cluster selection,
  - byte-index caret mapping,
  - leading versus trailing edge resolution.
- `LayoutResult` now records:
  - shaped glyph placements,
  - text clusters,
  - lines,
  - fallback usage counts,
  - missing-glyph counts.
- `he_scribe_tests` now covers:
  - combining-mark cluster preservation,
  - fallback face selection,
  - wrapping and hit testing,
  - right-to-left line placement path.

## Verified

- `./hemake.ps1 generate-projects vs2026`
- `he_test_runner.vcxproj`
- `he_test_runner.exe --filter "scribe:layout_engine"`

## Milestone Result

The roadmap success signal for M3 was that mixed-script text and combining-mark cases should
shape and lay out correctly.

That is now true for the implemented first-pass runtime path:

- shaping is driven by HarfBuzz over compiled font blob source bytes,
- fallback can switch faces inside one layout call,
- combining-mark sequences survive as shared clusters,
- wrapped layout produces stable line, cluster, and glyph placement records,
- hit testing can map positions back to cluster byte ranges.

## Current Limits

M3 is complete for the first runtime text layout slice, but this is not the final text engine:

- bidi handling is intentionally basic and currently uses paragraph direction plus per-run
  direction instead of a full Unicode bidi implementation,
- line breaking is whitespace/newline driven and does not yet implement full Unicode line-break
  rules,
- fallback is coverage-based across explicitly supplied faces and is not yet driven by a richer
  family-policy object,
- cluster hit testing is suitable for basic caret placement, but not yet full grapheme- and
  word-navigation behavior,
- a dedicated visual test app remains the next major visual-validation milestone.

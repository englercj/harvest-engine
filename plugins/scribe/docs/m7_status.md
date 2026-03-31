<!-- Copyright Chad Engler -->

# M7 Status

Milestone `M7: Production Hardening` is complete.

## Implemented

- Shared band-packing helper logic for fonts and SVGs with reuse of identical and
  contiguous-subset band payloads.
- Compiler-side band-packing statistics recorded for compiled font and vector-image payloads.
- Compile-time instrumentation logs for font and SVG compilation, including elapsed time and
  packed-payload stats.
- Regression coverage for:
  - band reuse and subset reuse,
  - repeatable font compilation from committed source fonts,
  - repeatable SVG compilation,
  - cap-height preservation in runtime blobs.
- Authoring/runtime usage notes and runtime-blob migration guidance.

## Validation

- `./hemake.ps1 generate-projects vs2026`
- `.build/win64-debug/bin/he_test_runner.exe --filter "scribe:"`
- `.build/projects/he_scribe_test_app.vcxproj`
- visual spot-check of `.build/win64-debug/bin/he_scribe_test_app.exe`

The current visual spot-check capture is:

- `.build/scribe_test_app_capture.png`

## Notes

- The runtime blob version remains `6`; M7 did not require a format bump.
- The negative version-rejection test still logs the expected unsupported-version error during
  `scribe:runtime_blob:reject_wrong_font_face_version`.
- Runtime behavior remains source-independent: FreeType stays on the importer/compiler side
  only, and runtime code continues to consume compiled Harvest-owned blobs.

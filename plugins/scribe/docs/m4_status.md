<!-- Copyright Chad Engler -->

# M4 Status: Visual Testbed

Milestone M4 is complete.

## What Landed

- Added a dedicated windowed test app in `plugins/scribe/test_app`.
- The app loads committed source fonts from `plugins/editor/src/fonts`.
- The app compiles TTF data in memory at startup and does not depend on committed compiled
  font blobs.
- The app shapes and lays out text through `he_scribe_layout`.
- The app renders compiled glyph resources through `he_scribe`.
- The app includes built-in visual demo scenes for:
  - paragraph wrapping,
  - combining marks and fallback,
  - right-to-left paragraph flow,
  - caret hit testing.

## Notes

- The test app installs a local error handler that writes `he_scribe_test_app.error.txt` in the
  working directory if an assertion fires. This is intentional testbed instrumentation for
  faster visual-debug iteration.
- While validating the app, `he_scribe` needed a small D3D12-facing descriptor-table fix:
  glyph texture bindings now use two one-entry ranges instead of one two-entry range because
  the current D3D12 descriptor-table allocator assumes one descriptor per recorded range.

## Verification

- `./hemake.ps1 generate-projects vs2026`
- `.build/projects/he_scribe_test_app.vcxproj` built successfully in `Debug Win64`
- launching `.build/win64-debug/bin/he_scribe_test_app.exe` produced a real window titled:
  `Scribe Testbed: paragraph wrapping and compiled font rendering [NotoSans-Regular.ttf]`
- no assertion log file was emitted during the final smoke run

## Deferred

- The testbed currently focuses on text. SVG/vector demo scenes remain future work under the
  later SVG milestone.
- The visual demo content is intentionally compact and should grow as later `scribe` features
  land.

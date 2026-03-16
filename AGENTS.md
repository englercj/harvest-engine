# Repository Guidelines

## Project Structure & Module Organization
`harvest-engine` is split between engine code and HE Make.

- `plugins/`: first-party engine plugins such as `core`, `editor`, `schema`, and `window`, typically with `include/`, `src/`, and `test/`.
- `contrib/`: third-party plugins and install metadata, each usually rooted by `he_plugin.kdl`.
- `hemake/`: HE Make sources. See `hemake/AGENTS.md` for HE Make-specific conventions, generator behavior, and test expectations.
- `docs/he_project_kdl/`: the KDL spec, token rules, and per-node behavior docs. Keep node docs aligned with implementation.
- Root files: `he_project.kdl` is the real project entrypoint; it imports `hemake/project_defaults.kdl`, `he_plugin.kdl`, then `contrib/*` and `plugins/*`.

## Build, Test, and Development Commands
Run from repo root unless the path says otherwise.

- `./hemake.ps1 generate-projects vs2026`: generate `.build/Harvest Engine.slnx` and `.build/projects/*.vcxproj`.
- `./hemake.ps1 install-plugins`: download/install plugin archives declared by `fetch` nodes.
- `dotnet msbuild hemake/Harvest.Make.slnx /p:Configuration=Debug /p:Restore=false /m:1`: build HE Make.
- `dotnet test hemake/Tests/Harvest.Make.Projects.Tests/Harvest.Make.Projects.Tests.csproj -c Debug --no-build`: run HE Make project-generation tests.
- `.build/<platform-config>/bin/he_test_runner.exe`: run engine tests after building the generated projects. Use this to verify new tests and catch regressions in engine modules.
- `he_test_runner.exe --filter <pattern>`: run a subset of tests for faster iteration when you only need one suite or module.

## Coding Style & Naming Conventions
- Follow `.editorconfig`: UTF-8, LF, 4-space indentation.
- C++: C++20, Allman braces, `lower_snake_case` files, `UpperCamelCase` types.
- For C++ changes, follow `docs/styleguides/cpp_style_guide.md`. It defines required patterns beyond formatting, including preferred Harvest utilities over STL.
- C#: Microsoft conventions, `PascalCase` APIs, one type family per file when practical.
- KDL: `lower_snake_case` filenames and explicit copyright header.

## Engine Constraints
- Harvest Engine is cross-platform. Do not call platform APIs directly unless the code is explicitly the platform abstraction layer for that feature.
- Prefer `he_core` facilities over STL when practical. Reuse Harvest containers, strings, file/path helpers, formatting, and platform abstractions for consistent behavior across platforms.
- Treat warnings as errors during verification. Fix warnings; do not silence them with pragmas or warning disables unless there is an explicit repository-standard reason.

## Testing Guidelines
- Engine tests live in plugin `test/` folders.
- HE Make tests live under `hemake/Tests/`.
- When changing an engine module, add or update tests in that module to cover the new behavior or the regression being fixed. Harvest engine tests use the custom framework in `plugins/core/include/he/core/test.h`.
- When changing nodes, tokens, import behavior, `.slnx`, or `.vcxproj` generation, update both implementation tests and the matching docs in `docs/he_project_kdl/nodes/`.

## Commit & Pull Request Guidelines
- Follow `docs/CONTRIBUTING.md`: short imperative summary, blank line, wrapped details.
- PRs should include rationale, linked issues/discussions, and test evidence for generator or behavior changes.

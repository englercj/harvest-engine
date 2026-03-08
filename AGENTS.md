# Repository Guidelines

## Project Structure & Module Organization
`harvest-engine` is split between engine code and HE Make.

- `plugins/`: first-party engine plugins such as `core`, `editor`, `schema`, and `window`, typically with `include/`, `src/`, and `test/`.
- `contrib/`: third-party plugins and install metadata, each usually rooted by `he_plugin.kdl`.
- `hemake/`: HE Make sources. See `hemake/AGENTS.md` for HE Make-specific conventions, generator behavior, and test expectations.
- `docs/he_project_kdl/`: the KDL spec, token rules, and per-node behavior docs. Keep node docs aligned with implementation.
- Root files: `he_project.kdl` is the real project entrypoint; it imports `scripts2/project_defaults.kdl`, `he_plugin.kdl`, then `contrib/*` and `plugins/*`.

## Build, Test, and Development Commands
Run from repo root unless the path says otherwise.

- `./hemake.sh generate-projects vs2026`: generate `.build/Harvest Engine.slnx` and `.build/projects/*.vcxproj`.
- `./hemake.sh install-plugins`: download/install plugin archives declared by `fetch` nodes.
- `dotnet msbuild hemake/Harvest.Make.slnx /p:Configuration=Debug /p:Restore=false /m:1`: build HE Make.
- `dotnet test hemake/Tests/Harvest.Make.Projects.Tests/Harvest.Make.Projects.Tests.csproj -c Debug --no-build`: run HE Make project-generation tests.

## Coding Style & Naming Conventions
- Follow `.editorconfig`: UTF-8, LF, 4-space indentation.
- C++: C++20, Allman braces, `lower_snake_case` files, `UpperCamelCase` types.
- C#: Microsoft conventions, `PascalCase` APIs, one type family per file when practical.
- KDL: `lower_snake_case` filenames and explicit copyright header.

## Testing Guidelines
- Engine tests live in plugin `test/` folders.
- HE Make tests live under `hemake/Tests/`.
- When changing nodes, tokens, import behavior, `.slnx`, or `.vcxproj` generation, update both implementation tests and the matching docs in `docs/he_project_kdl/nodes/`.

## Commit & Pull Request Guidelines
- Follow `docs/CONTRIBUTING.md`: short imperative summary, blank line, wrapped details.
- PRs should include rationale, linked issues/discussions, and test evidence for generator or behavior changes.

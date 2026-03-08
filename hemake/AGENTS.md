# Repository Guidelines

## Structure
HE Make lives entirely under `hemake/`.

- `Common/Harvest.Common`, `Common/Harvest.Kdl`, `Common/Harvest.SourceGenerators`: shared libraries and generators.
- `Harvest.Make.CLI`: command-line entrypoint used by `hemake.sh`.
- `Harvest.Make.Projects`: KDL node model, resolution pipeline, and VS2026 project generation.
- `Tests/`: HE Make unit tests, especially `Harvest.Make.Projects.Tests`.

## Build And Test
Run from the repo root unless noted otherwise.

- `./hemake.sh generate-projects vs2026`: generate `.build/Harvest Engine.slnx` and `.build/projects/*.vcxproj`.
- `./hemake.sh install-plugins`: install archives declared by `fetch` nodes.
- `dotnet msbuild hemake/Harvest.Make.slnx /p:Configuration=Debug /p:Restore=false /m:1`: build all HE Make projects.
- `dotnet test hemake/Tests/Harvest.Make.Projects.Tests/Harvest.Make.Projects.Tests.csproj -c Debug --no-build`: run project-generation tests.

## Node And KDL Rules
- `project_file` is for existing `.csproj` files only. Any other project type should be rejected.
- `module.language` resolves as: explicit `language`, else infer from `.csproj`, else `cpp`.
- Valid module kinds are `app_console`, `app_windowed`, `content`, `custom`, `lib_header`, `lib_static`, and `lib_shared`.
- GitHub and Bitbucket `fetch` nodes require `ref`.
- Incremental linking is controlled by `link_options incremental=...`, not `toolset`.

## VS2026 Generation Notes
- `.slnx` is generated at `.build/Harvest Engine.slnx`; C++ projects go to `.build/projects/`.
- Generated `.vcxproj` files intentionally use the full Visual Studio config/platform matrix, including semantically useless cross-pairs, because the VC project system expects it.
- Modules using `project_file` should be referenced directly from the solution instead of generating a `.vcxproj`.
- For external `.csproj` mappings in `.slnx`, warn only when no exact or fuzzy config/platform match exists and HE Make must guess.

## Editing Rules
- Follow `.editorconfig`: UTF-8, LF, 4-space indentation.
- Node documentation lives in `docs/he_project_kdl/nodes/`.
- When changing node behavior, validation, defaults, tokens, or supported properties, update the matching node doc in `docs/he_project_kdl/nodes/` in the same change.
- When changing node resolution, `.slnx`, or `.vcxproj` output, add or update tests in `hemake/Tests/Harvest.Make.Projects.Tests/`.

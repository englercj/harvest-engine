---
name: harvest-build-run
description: Build, rebuild, and run Harvest Engine projects on Windows. Use when Codex needs to generate `.build/projects/*.vcxproj`, install plugin archives, build Harvest's C++ targets with Visual Studio MSBuild, build HE Make with `dotnet`, or launch engine binaries and the test runner from `.build/win64-debug/bin`. Use this skill when PowerShell quoting, DLL search paths, or MSBuild vs `dotnet msbuild` details matter.
---

# Harvest Build Run

## Work from repo root

Run build and launch commands from the repository root so relative paths under `.build/`,
`plugins/`, and `hemake/` resolve correctly.

## Refresh generated projects first when needed

Regenerate Visual Studio projects if `.build/projects/<target>.vcxproj` is missing, stale, or a
`he_plugin.kdl` / `he_project.kdl` change affects modules:

```powershell
./hemake.ps1 generate-projects vs2026
```

Install fetched plugin archives before building projects that depend on downloaded packages:

```powershell
./hemake.ps1 install-plugins
```

## Build HE Make with dotnet, not engine vcxproj files

Use `dotnet msbuild` only for the managed HE Make solution:

```powershell
dotnet msbuild hemake/Harvest.Make.slnx /p:Configuration=Debug /p:Restore=false /m:1
dotnet test hemake/Tests/Harvest.Make.Projects.Tests/Harvest.Make.Projects.Tests.csproj -c Debug --no-build
```

Do not use `dotnet msbuild` for generated `.vcxproj` C++ projects. Harvest's generated native
projects import Visual Studio C++ targets and should be built with Visual Studio MSBuild.

## Build generated C++ projects with Visual Studio MSBuild

Use the Visual Studio MSBuild executable directly:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    ".build\projects\he_test_runner.vcxproj" `
    /t:Build `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /m:1
```

General pattern for any generated native target:

```powershell
$project = "he_core"
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    ".build\projects\$project.vcxproj" `
    /t:Build `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /m:1
```

Expect binaries under:

```text
.build/win64-debug/bin/
```

Expect static libraries under:

```text
.build/win64-debug/lib/
```

## Run binaries and tests

Run the engine test runner:

```powershell
& ".build\win64-debug\bin\he_test_runner.exe"
& ".build\win64-debug\bin\he_test_runner.exe" --filter "core:range_ops"
& ".build\win64-debug\bin\he_test_runner.exe" --times
```

Run another built executable the same way:

```powershell
& ".build\win64-debug\bin\<target>.exe"
```

If the target name is unclear, inspect `.build/projects/` for the generated project name or the
plugin `he_plugin.kdl` that defines the module.

## PowerShell and pathing rules that matter here

- Use the call operator `&` whenever the executable path is quoted.
- Quote paths under `C:\Program Files\...`.
- Keep the build configuration string exactly as generated: `"Debug Win64"`.
- Keep the MSBuild platform as `x64`.
- Prefer backticks for PowerShell line continuation in multiline command examples.

## Common Windows issues seen in this repo

If MSBuild fails with `MSB6001` and a message like `Key in dictionary: 'Path'  Key being added: 'PATH'`,
normalize the process environment before invoking MSBuild:

```powershell
[System.Environment]::SetEnvironmentVariable("PATH", $env:Path, "Process")
[System.Environment]::SetEnvironmentVariable("Path", $null, "Process")
```

If a built binary exits immediately with code `-1073741515` (`0xC0000135`), suspect a missing DLL.
In this repo, adding the DirectStorage x64 runtime folder to `PATH` fixed runner startup:

```powershell
$env:PATH = (Resolve-Path ".build\installs\Microsoft.Direct3D.DirectStorage-*\native\bin\x64").Path + ";" + $env:PATH
```

Apply that `PATH` update in the same PowerShell process that launches the executable.

## Source of truth when unsure

- `he_project.kdl`
- `plugins/*/he_plugin.kdl`
- `.build/projects/*.vcxproj`
- `.build/win64-debug/bin/`
- `hemake/AGENTS.md`
- `AGENTS.md`

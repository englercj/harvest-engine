---
name: harvest-project-kdl
description: Create, edit, and review Harvest Engine project, plugin, and module definitions written in HE Make's KDL project format. Use when Codex needs to modify `he_project.kdl`, `he_plugin.kdl`, plugin module declarations, contrib package definitions, `schema_compile` or other HE Make extension nodes, or any project KDL that drives generated `.build/` outputs.
---

# Harvest Project KDL

## Read these first

- [KDL v2.0.0 spec](https://github.com/kdl-org/kdl/blob/kdl-v2/SPEC.md)
- [Project file spec](../../../docs/he_project_kdl_spec.md)
- [Project KDL path rules](../../../docs/he_project_kdl/paths.md)
- [Project KDL string tokens](../../../docs/he_project_kdl/tokens.md)
- [Project KDL node modifiers](../../../docs/he_project_kdl/node_modifiers.md)
- [Node reference directory](../../../docs/he_project_kdl/nodes/)

The `docs/he_project_kdl/nodes/*.md` files are the source of truth for individual nodes such as
`project`, `plugin`, `module`, `public`, `dependencies`, `files`, `include_dirs`, `when`,
`fetch`, `schema_compile`, `shader_compile`, and related configuration nodes.

## Never edit generated `.build/` project files directly

- Treat `.build/projects/*.vcxproj`, `.build/Harvest Engine.slnx`, and other generated `.build/`
  project files as outputs.
- Make all project-structure changes in source KDL files such as `he_project.kdl`,
  `he_plugin.kdl`, or plugin-local `he_plugin.kdl` files.
- Regenerate after source changes:

```powershell
./hemake.ps1 generate-projects vs2026
```

## Common file locations in this repo

- Root project entrypoint: [he_project.kdl](../../../he_project.kdl)
- Root import plugin: [he_plugin.kdl](../../../he_plugin.kdl)
- First-party plugin definitions: `plugins/<name>/he_plugin.kdl`
- Third-party/contrib plugin definitions: `contrib/<name>/he_plugin.kdl`
- HE Make defaults imported by the root project: `hemake/project_defaults.kdl`

## How the top-level project is organized

The root [he_project.kdl](../../../he_project.kdl) follows a simple pattern:

- declare one `project`
- import `./hemake/project_defaults.kdl`
- import the root plugin `./he_plugin.kdl`
- import first-party and contrib plugins via glob imports like `import "contrib/*"` and
  `import "plugins/*"`

Use that same style when extending the root project. Keep the root file focused on composition and
imports, not detailed module definitions.

## Common plugin and module patterns

A typical first-party plugin file:

- declares `plugin <name> version=... license=...`
- usually has an `authors { ... }` block
- defines one or more `module` blocks
- groups modules into solution folders with `group="engine/libs"`, `group="engine/tools"`,
  `group="engine/tests"`, or similar

Common module structure:

```kdl
module he_example kind=lib_static group="engine/libs" {
    files { "include/**"; "src/**" }
    include_dirs { "src" }

    dependencies { some_private_dep }

    public {
        include_dirs { "include" }
        dependencies { he_core; some_public_dep }
    }

    when platform.system=windows {
        public {
            dependencies { user32 kind=system }
        }
    }
}
```

Follow these recurring patterns:

- Put implementation-only include paths at the module level.
- Put exported include paths and exported deps in `public { ... }`.
- Use `when ... { ... }` blocks for platform, toolset, tag, or configuration-specific changes.
- Use `kind=system`, `kind=file`, `kind=include`, or `kind=link` when dependency semantics need
  to be explicit.
- Use `${...}` token interpolation rather than hardcoding generated or install paths when a token
  already exists.

## First-party plugin conventions

- First-party plugins usually live under `plugins/<plugin>/`.
- Modules are commonly named with the `he_` prefix, such as `he_core`, `he_assets`,
  `he_schema`, `he_test_runner`.
- Test modules commonly use `group="engine/tests"`.
- Tool executables commonly use `group="engine/tools"`.
- Generated companion include paths often use `${module.gen_dir}/include`.

See representative examples:

- [plugins/core/he_plugin.kdl](../../../plugins/core/he_plugin.kdl)
- [plugins/assets/he_plugin.kdl](../../../plugins/assets/he_plugin.kdl)
- [plugins/schema/he_plugin.kdl](../../../plugins/schema/he_plugin.kdl)

## Contrib plugin conventions

- Third-party packages usually live under `contrib/<package>/he_plugin.kdl`.
- Contrib plugins often use a `fetch` block to download a GitHub release, archive, or NuGet
  package into `${plugin.install_dir}`.
- Contrib modules usually use `group="engine/contrib"`.
- Public include directories usually point at `${plugin.install_dir}` or package subdirectories.

See representative examples:

- [contrib/basis_universal/he_plugin.kdl](../../../contrib/basis_universal/he_plugin.kdl)
- [contrib/DirectStorage/he_plugin.kdl](../../../contrib/DirectStorage/he_plugin.kdl)
- [contrib/mimalloc/he_plugin.kdl](../../../contrib/mimalloc/he_plugin.kdl)

## Common advanced patterns in this repo

- Root import plugin pattern:
  [he_plugin.kdl](../../../he_plugin.kdl) defines a lightweight root plugin whose job is to make
  importing the engine convenient.
- HE Make extension modules:
  modules with `kind=hemake_extension` and `project_file=...` appear in plugins like
  [plugins/schema/he_plugin.kdl](../../../plugins/schema/he_plugin.kdl) and
  [plugins/bin2c/he_plugin.kdl](../../../plugins/bin2c/he_plugin.kdl).
- Generated schema companions:
  `schema_compile "c++" ...` blocks generate companion modules and typically add
  `${module.gen_dir}/include` to public include paths.
- Test runner aggregation:
  [plugins/core/he_plugin.kdl](../../../plugins/core/he_plugin.kdl) uses a `:foreach` generator
  over `group="engine/tests"` to pull test libs into `he_test_runner`.

## Practical editing rules

- Keep module declarations cohesive: files, private includes, private deps, then `public`, then
  `when` blocks.
- Prefer modifying existing `when` blocks over duplicating near-identical platform logic.
- Match the repo's existing group names unless there is a strong reason to add a new one.
- When adding a module that generates files, make sure downstream include paths use the generated
  directory tokens rather than hardcoded `.build` paths.
- When adding fetch-backed contrib packages, keep install-path references relative to
  `${plugin.install_dir}`.

## Where to look when unsure

- [he_project.kdl](../../../he_project.kdl)
- [he_plugin.kdl](../../../he_plugin.kdl)
- [docs/he_project_kdl_spec.md](../../../docs/he_project_kdl_spec.md)
- [docs/he_project_kdl/paths.md](../../../docs/he_project_kdl/paths.md)
- [docs/he_project_kdl/tokens.md](../../../docs/he_project_kdl/tokens.md)
- [docs/he_project_kdl/node_modifiers.md](../../../docs/he_project_kdl/node_modifiers.md)
- [docs/he_project_kdl/nodes/](../../../docs/he_project_kdl/nodes/)

---
name: harvest-schema-authoring
description: Write, edit, and review Harvest Schema `.hsc` files. Use when Codex needs to define or evolve Harvest Schema structs, enums, interfaces, unions, groups, attributes, defaults, or asset type schemas, or when schema authoring decisions should account for the Harvest binary format and asset-type best practices.
---

# Harvest Schema Authoring

## Read these first

- [Language guide](../../../docs/schema_language_guide.md)
- [Binary format spec](../../../docs/schema_binary_format.md)
- [Asset schema example](../../../plugins/assets/assets/include/he/assets/asset_types.hsc)

Use the docs above as the source of truth. This skill distills the recurring rules and the asset
authoring guidance that matters most in this repo.

## File and declaration rules

- Use the `.hsc` extension for Harvest Schema files.
- Put the file's unique 64-bit ID first, before any imports, namespace, or declarations.
- Put `import` statements immediately after the file ID.
- Use at most one `namespace` per file.
- Number field and enumerator ordinals consecutively from zero with no gaps.
- Treat ordinals and field types as binary compatibility boundaries. Do not renumber fields, change
  field types, or remove fields in an existing binary-compatible schema.
- If a type may be renamed or moved later, prefer giving it an explicit ID rather than relying on
  the derived ID.

## Core language reminders

- Built-in types include `bool`, the signed and unsigned integer families, `float32`, `float64`,
  `String`, `Blob`, `AnyPointer`, `AnyStruct`, and `AnyList`.
- Use `[]` for dynamic lists and `[N]` for fixed-size arrays.
- Changing a fixed-size array length is a binary-breaking change.
- If no explicit default is provided, the default is the zeroed value for that type.
- Type aliases are language-only conveniences. Generated schema data sees only the resolved type.
- Attributes come after the declaration name or field value syntax and before any nested block.
- Unions are declared inline inside structs with `name :union { ... }`.
- Groups are declared inline with `name :group { ... }`. See `mipMapping :group` and
  `compression :group` in [asset_types.hsc](../../../plugins/assets/assets/include/he/assets/asset_types.hsc).

## Binary format consequences that should influence authoring

- Harvest is 64-bit and the binary format is word-based. Pointer-bearing structures cost pointer
  section space.
- `bool` values are bit-packed in the binary format, not byte-sized.
- Structs split data and pointer sections, so introducing pointer-heavy nesting has real layout and
  cache consequences.
- Unions are tagged and support an unset state.
- It is valid to move an existing field into a new union only when the other union members are new.

## Asset type best practices

- Prefer `:bool` fields over `uint32` bit flags for feature toggles. Bools are encoded as a single
  bit anyway, so bit flags usually reduce clarity without saving space.
- Prefer `:group` to logically group related asset fields instead of creating a separate struct
  solely for organization. A separate struct introduces pointer overhead that a group avoids.
- Use a separate `struct` instead of a `:group` when the same collection of fields is reused, when
  the pointer field should be nullable, or when you need a list of that collection of fields.
- Follow the pattern in [asset_types.hsc](../../../plugins/assets/assets/include/he/assets/asset_types.hsc):
  keep the top-level asset type readable, then use `:group` for local option clusters like
  mip-map and compression settings.
- Keep asset references explicit through schema fields and attributes such as `$AssetRef(...)`
  when that is part of the asset contract.

## Validation guidance for asset pipelines

- Put resource-data validation in the asset compiler or importer that generates the resource data.
- Throw errors before invalid resource data is emitted.
- Once a resource has been generated, assume it is valid so runtime validation is not required for
  normal correctness.
- Keep runtime checks focused on crash prevention only, such as bounds checks, null checks,
  invalid-memory avoidance, and size validation before reads or writes.

## Practical authoring checklist

- Start from [schema_language_guide.md](../../../docs/schema_language_guide.md) for syntax and
  allowed constructs.
- Use [schema_binary_format.md](../../../docs/schema_binary_format.md) when deciding whether a
  layout or evolution change is safe.
- Use [asset_types.hsc](../../../plugins/assets/assets/include/he/assets/asset_types.hsc) as the
  primary example for asset-type structure, attributes, and `:group` usage.
- Prefer explicit, readable field names and small cohesive groups.
- When evolving an existing schema, preserve ordinals and append new fields rather than reshaping
  old ones.

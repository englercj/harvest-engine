# `codegen` node

Configures the code generation behavior for the compiler.

## Arguments

1. (string) - Required. Which code generation mode to enable. Valid values are:
    * `avx` - Enable Intel Advanced Vector Extensions instructions.
    * `avx2` - Enable Intel Advanced Vector Extensions 2 instructions.
    * `avx512` - Enable Intel Advanced Vector Extensions 512 instructions.
    * `sse` - Enable SSE instructions.
    * `sse2` - Enable SSE2 instructions.
    * `sse3` - Enable SSE3 instructions.
    * `ssse3` - Enable SSSE3 instructions.
    * `sse4.1` - Enable SSE4.1 instructions.
    * `sse4.2` - Enable SSE4.2 instructions.
    * `simd128` - Enable WASM SIMD 128 isntructions.
    * `armv8.n` - Set minimum CPU requirements to ARMv8.n-A, where `n` is in the range `[0, 8]`.

## Properties

None.

## Children

None.

## Scopes

None.

## Example

```kdl
codegen avx
```

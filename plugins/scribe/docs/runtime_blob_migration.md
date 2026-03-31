<!-- Copyright Chad Engler -->

# Runtime Blob Migration

`he.scribe` runtime blobs are versioned explicitly. The current format version is `6`.

## When To Bump The Version

Bump `RuntimeBlobFormatVersion` when any change breaks binary compatibility or changes the
meaning of existing serialized fields. Typical reasons:

- adding required fields,
- removing fields,
- changing enum values or flag meaning,
- changing curve or band packing semantics,
- changing paint/layer interpretation,
- changing shaping metadata semantics in a way old readers cannot interpret safely.

## When Not To Bump The Version

Do not bump the version for implementation-only changes that preserve on-disk semantics, such
as:

- compile-time performance improvements,
- internal deduplication that still produces the same documented blob contract,
- runtime caching changes,
- new tests or docs.

## Migration Checklist

When a real format change is needed:

1. Update `RuntimeBlobFormatVersion` in `runtime_blob.h`.
2. Update blob readers and writers together.
3. Update milestone/status docs if the change affects the published pipeline contract.
4. Add or update unit coverage for version acceptance and rejection.
5. Regenerate and rebuild affected projects.

## Compatibility Policy

- Runtime readers reject mismatched blob versions instead of attempting implicit migration.
- Recompiling source assets is the expected migration path for incompatible blob changes.
- Test assets in the repo should remain source-first so recompilation is straightforward.

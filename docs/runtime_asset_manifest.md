# Some ideas for a cooked out asset manifest usable at runtime

Everything is LE encoded. Format intended to maximize version compatibility and require very little processing on load.

TODO: Encoding specifiers? Different representation than string?

Note: A feature flag that may be good is "loose files" which means storage file table is empty and assets are in loose files keyed by asset ID and resource index.

## Header:

magic - uint32
version - uint32
header size - uint32
feature flags - uint32
asset id length - uint32
lookup table offsest - uint32
lookup table entry size - uint32
lookup table entry count - uint32
lookup table bucket count - uint32
asset table offset - uint32
asset table entry size - uint32
asset table entry count - uint32
encoding specifier table offset - uint32
encoding specifier table size - uint32

## Lookup Table:

A hash table of entries keyed by an asset id
series of buckets (header -> lookup table bucket count)
each bucket holds a series of entries, sorted so they can be binary searched

### Lookup Table Bucket:

entry count - uint32
entries - array<lookup table entry>(entry count)

### Lookup Table Entry:

asset id - array<uint8>(header -> asset id length)
index - uint32

## Asset Table:

series of asset entries (header -> asset table entry count)

### Asset Table Entry:

source size - uint32
encoded size - uint32
encoding specifier offset - uint32

storage file index - uint32
storage file offset - uint64

## Encoding Specifier Table:

series of encoding specifier entries

### Encoding Specifier Table Entry:

spec - null terminated string

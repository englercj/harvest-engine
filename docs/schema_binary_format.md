# Harvest Schema - Binary Format Spec

Based on the encoding format of [Cap'n Proto](https://capnproto.org/encoding.html).

This serves as both the transmission and in-memory format for Harvest Schema objects.

## Organization

### Word Size

Harvest is a 64-bit engine, and the schema binary format is built upon an 8-byte word. All objects (structs, lists) are aligned to word boundaries, and sizes are generally expressed in terms of words (not bytes). Primitive values are aligned to a multiple of their size.

### Encoding

The binary encoding is defined as a tree of objects, the root of which is always a struct. The first word of an encoded blob is always a pointer to the root struct.

## Value Encoding

### Primitive Values

Primitives are encoded as follows:

- `void` - Not encoded. There is only one possible value.
- `bool` - One bit where `1` is true and `0` is false.
- Integer - Little-endian format. Signed integers use two's complement.
- Floating-point - Little-ending IEEE-754 format.

Primitive types are aligned to a multiple of their size. Boolean values are bit-packed, unlike C++ where each bool is a single byte in size.

## Object Encoding

### Structs

A struct is encoded as a pointer to its content. The content is split into two sections: data and pointers. The pointer section is encoded directly after the data section.

A struct pointer is one word (64-bits), and has the following layout:

```
lsb                      struct pointer                       msb
+-+-----------------------------+---------------+---------------+
|A|             B               |       C       |       D       |
+-+-----------------------------+---------------+---------------+

A (2 bits) = 0, to indicate that this is a struct pointer.
B (30 bits) = Offset, in words, from the end of the pointer to the
    start of the struct's data section. Signed.
C (16 bits) = Size of the struct's data section, in words.
D (16 bits) = Size of the struct's pointer section, in words.
```

Notes about the positioning of fields in the data and pointer sections:

- The field metadata words are included in the data section size. See below for details about this metadata.
- A field's position is affected only by lower-numbered fields, never higher-numbered fields. This ensured backwards-compatibility for newly added fields.
- Fields in the data section may be separated by padding. Later fields may be positioned into the padding of an earlier field, if alignment rules allow. This ensures minimal waste due to padding.

#### Field Metadata

The data section of a struct starts with a word that stores additional information about the data fields in the struct. It has the following layout:

```
lsb                  struct field metadata                    msb
+---------------+---------------+-------------------------------+
|       A       |       B       |               C               |
+---------------+---------------+-------------------------------+

A (16 bits) = Number of data fields in the struct. Always greater
    than zero.
B (16 bits) = Reserved for future use. Always zero.
C (32 bits) = Bit set representing if each field is "set".
    The field ordinal is the index into the bitset, which starts
    at the least-significant bit.
```

If the number of data fields is larger than 32, then an additional word is added for each 64 fields above that value. Each word is equivalent to `C`, but 64-bits in size.

#### Default Values

Default structs are always all-zeroes. This means the field metadata bitset is set to zero so all data fields will return default values, and all pointers are set to zero (null) so accessors for will return a pointer to a defaulted struct. This ensures allocation is fast and simple, and that any applied compression can take advantage of sequences of zero-value bytes.

#### Zero-Sized Structs

A pointer to a struct of zero-size is stored as an all-zero pointer, with the exception of the offset which is set to `-1`. This allows you to distinguish between a null pointer and a pointer to a struct with no members. Since a struct with no members has no data or pointer sections the offset just points back to the pointer word itself.

#### Field Unions

Fields in a union are encoded just as they otherwise would be in addition to a "tag" value which represents which union field is set. Note that since structs have data and pointer sections unions that contain both primitive and pointer types will have their values encodes across both sections. The tag is always stored in the data section of the struct, before the second member of the union.

For example, consider this struct:

```
struct S
{
    union
    {
        foo @0 :int32;
        bar @1 :int32;
        qux @3 :int64;
    }
    baz @2 :int32;
}
```

1. `foo` is placed at offset 0
2. The tag is placed at offset 4
3. `bar` is placed at offset 0, since it can fit within a previous member's space
4. `baz` is placed at offset 8, since the tag ends at offset 6 but `baz` is 4-byte aligned
5. `qux` is placed at offset 16, since it cannot fit within a previous member's space and `baz` ends at offset 12 but `qux` is 8-byte aligned.

The reason for this complexity is to support schema evolution of the union. This encoding scheme allows adding new fields to the union without breaking compatibility. It also supports converting a field into a new union because the tag isn't encoded until before the second member.

### Lists

A list pointer is one word (64-bits), and has the following layout:

```
lsb                       list pointer                        msb
+-+-----------------------------+--+----------------------------+
|A|             B               |C |             D              |
+-+-----------------------------+--+----------------------------+

A (2 bits) = 1, to indicate that this is a list pointer.
B (30 bits) = Offset, in words, from the end of the pointer to the
    start of the first element of the list. Signed.
C (3 bits) = Size of each element:
    0 = 0 (e.g. List<void>)
    1 = 1 bit
    2 = 1 byte
    3 = 2 bytes
    4 = 4 bytes
    5 = 8 bytes (non-pointer)
    6 = 8 bytes (pointer)
    7 = composite (see below)
D (29 bits) = Size of the list:
    when C < 7: Number of elements in the list.
    when C = 7: Number of words in the list, not counting the tag word
    (see below).
```

The elements of the list are tightly-packed. For example, `bool`s are packed bit-by-bit in little-endian order (the first bool is the least-significant bit of the first byte).

#### Composite Elements

When `C = 7`, the elements of the list are fixed-width composite values –- usually, structs. In this case, the list content is prefixed by a "tag" word that describes the elements. It has the following layout:

```
lsb                    list composite tag                     msb
+-+-----------------------------+---------------+---------------+
|A|             B               |       C       |       D       |
+-+-----------------------------+---------------+---------------+

A (2 bits) = 0, to indicate that elements are structs.
B (30 bits) = Number of elements in the list.
C (16 bits) = Size of the struct's data section, in words.
D (16 bits) = Size of the struct's pointer section, in words.
```

The list's pointer stores a word count rather than an element count to ensure that the extents of the list's location can always be determined by inspecting the pointer alone, without having to look at the tag; this may allow more-efficient prefetching in some use cases. The reason we don't store struct lists as a list of pointers is because doing so would take significantly more space (an extra pointer per element) and may be less cache-friendly.

#### Strings

Strings are encoded the same as a list of bytes (`List<uint8>`), with some restrictions:

- All bytes must be valid UTF-8
- The last byte must be zero

The list size includes the null terminating byte in the encoded format, however the library will report the string size without counting this byte.

## Schema Evolution

TODO: What is valid when evolving the schema?

Union note:

You can move an existing field into a new union without breaking compatibility with existing data, as long as all of the other fields in the union are new. Since the existing field is necessarily the lowest-numbered in the union, it will be the union's default field.

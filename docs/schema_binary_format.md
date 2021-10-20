# Harvest Schema - Binary Format

A binary file contains a header followed by a series of serialized structures.

- All values are little-endian encoded.
- All offsets are relative to the offset value itself (relative pointers).

```
/--------+-------------+-----+-------------\
| header | structure 1 | ... | structure N |
\--------+-------------+-----+-------------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| header    | 12            | Header of the file. |
| structure | variable      | Structure data as defined by the schema. |

## Header:

```
/-----------+---------+----------+-------------\
| signature | version | reserved | root offset |
\-----------+---------+----------+-------------/
```

| Element       | Size (bytes)  | Description |
| ------------- | ------------: | ----------- |
| signature     | 4             | Signature identifier for the file. |
| version       | 2             | Version of the file. Always equal to 1. (uint16) |
| reserved      | 2             | Reserved bytes, always zero. |
| root offset   | 4             | Offset to the root structure of the file. (uint32) |

## Structure:

```
/---------+-----+---------+---------\
| field 1 | ... | field N | v-table |
\---------+-----+---------+---------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| field     | variable      | Each field of the structure tightly packed. |
| v-table   | variable      | Indirection table mapping field IDs to offsets. |

### V-Table

```
/-------------+------------+----------------+-----+------------+----------------\
| field count | field id 1 | field offset 1 | ... | field id N | field offset N |
\-------------+------------+----------------+-----+------------+----------------/
```

| Element       | Size (bytes)  | Description |
| ------------- | ------------: | ----------- |
| field count   | 2             | Number of id/offset pairs in the v-table. (uint16) |
| field id      | 4             | An FNV1a hash of the field name. (uint32) |
| field offset  | 4             | Offset to the field. (uint32) |

### Fields

#### Scalars

Scalar values are stored inline using little-endian encoding.

```
/-------\
| value |
\-------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| `bool`    | 1             | Boolean stored as zero for false, and one for true. |
| `int8`    | 1             | 8-bit two's complement signed integer. |
| `int16`   | 2             | 16-bit two's complement signed integer. |
| `int32`   | 4             | 32-bit two's complement signed integer. |
| `int64`   | 8             | 64-bit two's complement signed integer. |
| `uint8`   | 1             | 8-bit unsigned integer. |
| `uint16`  | 2             | 16-bit unsigned integer. |
| `uint32`  | 4             | 32-bit unsigned integer. |
| `uint64`  | 8             | 64-bit unsigned integer. |
| `float32` | 4             | 32-bit IEEE-754 floating point value. |
| `float64` | 8             | 64-bit IEEE-754 floating point value. |

#### Array, List, Set, and Vector

These containers are stored as a sequence of elements, prefixed by a 4-byte unsigned integer length. Scalar values are stored inline, all other values store a byte offset (uint32) to the object.

```
/--------+-----------+-----+-----------\
| length | element 1 | ... | element N |
\--------+-----------+-----+-----------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| length    | 4             | Number of elements in the sequence. (uint32) |
| element   | variable      | Scalars are stored inline, other types are stored as offsets. |

#### String

Strings are stored as a sequence of 1-byte characters, prefiex by a 4-byte unsigned integer length and terminated with a null byte. The length value does not include the null terminator.

```
/--------+--------+-----+--------+------\
| length | char 1 | ... | char N | null |
\--------+--------+-----+--------+------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| length    | 4             | Number of characters in the string, not including the null terminator. (uint32) |
| char      | 1             | UTF-8 encoded character of the string. |
| null      | 1             | Null terminator character, always zero. |

#### Map

Maps are stored as a sequence of key-value pairs. Scalar keys and values are stored inline, all other key and value types store a byte offset (uint32) to the object.

```
/--------+-------+---------+-----+-------+---------\
| length | key 1 | value 1 | ... | key N | value N |
\--------+-------+---------+-----+-------+---------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| length    | 4             | Number of key-value pairs in the map. (uint32) |
| key       | variable      | Scalars are stored inline, other types are stored as offsets. |
| value     | variable      | Scalars are stored inline, other types are stored as offsets. |

#### Union

Unions are stored as a field ID of which value is set, followed by the value itself. The value is stored according to the rules of that type.

TODO: Currently non-scalar values are stored as an offset, but they can be stored inline and remove the indirect. Need to figure out what the API should look like to build a union that way.

```
/----------+-------\
| field id | value |
\----------+-------/
```

| Element   | Size (bytes)  | Description |
| --------- | ------------: | ----------- |
| field id  | 4             | FNV1a hash of the field's name. (uint32) |
| value     | variable      | The value encoded inline. |

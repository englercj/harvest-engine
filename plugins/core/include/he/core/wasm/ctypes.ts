// Copyright Chad Engler

// Some c-like types to make intentions more clear for functions that are meant to interface
// with C code. This allows us more strict type checking and better intellisense.

/// An 8-bit signed integer: `int8_t`
export type i8 = number & { __brand__: 'i8' };

/// An 16-bit signed integer: `int16_t`
export type i16 = number & { __brand__: 'i16' };

/// An 32-bit signed integer: `int32_t`
export type i32 = number & { __brand__: 'i32' };

/// An 64-bit signed integer: `int64_t`
export type i64 = bigint & { __brand__: 'i64' };

/// An 8-bit unsigned integer: `uint8_t`
export type u8 = number & { __brand__: 'u8' };

/// An 16-bit unsigned integer: `uint16_t`
export type u16 = number & { __brand__: 'u16' };

/// An 32-bit unsigned integer: `uint32_t`
export type u32 = number & { __brand__: 'u32' };

/// An 64-bit unsigned integer: `uint64_t`
export type u64 = bigint & { __brand__: 'u64' };

/// An 32-bit floating point number: `float`
export type f32 = number & { __brand__: 'f32' };

/// An 64-bit floating point number: `double`
export type f64 = number & { __brand__: 'f64' };

/// An 8-bit character: `char`
export type char = number & { __brand__: 'char' };

/// A pointer to a value: `T*`
export type ptr<T> = number & { __brand__: 'ptr', __ptr__: T };

/// A pointer to a constant value: `const T*`
export type const_ptr<T> = number & { __brand__: 'const_ptr', __ptr__: T };

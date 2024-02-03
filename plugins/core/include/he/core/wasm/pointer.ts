// Copyright Chad Engler

import { const_ptr, char, f32, f64, i8, i16, i32, i64, ptr, u8, u16, u32, u64 } from './ctypes';
import { Heap } from './heap';

export class Pointer
{
    /// Returns the number of bytes required to store the given string as a UTF-8 byte array.
    ///
    /// \param[in] str The JavaScript string to measure.
    /// \return The number of bytes required to store the string as a UTF-8 byte array.
    static utf8StrLen(str: string): number
    {
        let len = 0;
        for (let i = 0; i < str.length; ++i)
        {
            var c = str.charCodeAt(i);
            if (c <= 0x7f)
            {
                len++;
            }
            else if (c <= 0x7ff)
            {
                len += 2;
            }
            else if (c >= 0xd800 && c <= 0xdfff)
            {
                len += 4;
                ++i;
            }
            else
            {
                len += 3;
            }
        }
        return len;
    };

    /// Reads a string from the heap at the given pointer.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \param[in] len The number of bytes to read. If len is -1, the string is assumed to be null-terminated.
    static readString(ptr: const_ptr<char>, len: number)
    {
        if (len == 0 || !ptr)
            return '';

        if (len < 0)
        {
            len = 0;
            while (Heap.u8[ptr + len])
                ++len;
        }

        // TextDecoder.decode() doesn't work with a view of a ShredArrayBuffer, so make a copy.
        // See: https://github.com/whatwg/encoding/issues/172
        const array = Heap.buffer instanceof SharedArrayBuffer ? Heap.u8.slice(ptr, ptr + len) : Heap.u8.subarray(ptr, ptr + len);
        const decoder = new TextDecoder('utf-8');
        return decoder.decode(array);
    };

    /// Writes a string to the heap at the given pointer.
    ///
    /// \param[in] str The JavaScript string to copy from.
    /// \param[in] dst The destination buffer to write to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \return The number of bytes written to the destination buffer.
    static writeString(str: string, dst: ptr<char>, dstLen: u32)
    {
        if (dstLen <= 0)
            return 0;

        const start = dst;

        if (Heap.buffer instanceof SharedArrayBuffer)
        {
            // TextEncoder.encodeInto() doesn't work with a view of a ShredArrayBuffer, so encode directly.
            // See: https://github.com/whatwg/encoding/issues/172
            let end = dst + dstLen;
            for (let i = 0; i < str.length; ++i)
            {
                var c = str.charCodeAt(i);
                if (c <= 0x7f)
                {
                    if (dst >= end)
                        break;
                    Heap.u8[dst++] = c;
                }
                else if (c <= 0x7ff)
                {
                    if (dst + 1 >= end)
                        break;
                    Heap.u8[dst++] = 0xc0 | (c >> 6);
                    Heap.u8[dst++] = 0x80 | (c & 0x3f);
                }
                else if (c >= 0xd800 && c <= 0xdfff)
                {
                    if (dst + 3 >= end)
                        break;
                    c = ((c - 0xd800) << 10) + (str.charCodeAt(++i) - 0xdc00) + 0x10000;
                    Heap.u8[dst++] = 0xf0 | (c >> 18);
                    Heap.u8[dst++] = 0x80 | ((c >> 12) & 0x3f);
                    Heap.u8[dst++] = 0x80 | ((c >> 6) & 0x3f);
                    Heap.u8[dst++] = 0x80 | (c & 0x3f);
                }
                else
                {
                    if (dst + 2 >= end)
                        break;
                    Heap.u8[dst++] = 0xe0 | (c >> 12);
                    Heap.u8[dst++] = 0x80 | ((c >> 6) & 0x3f);
                    Heap.u8[dst++] = 0x80 | (c & 0x3f);
                }
            }
            Heap.u8[dst] = 0;
            return dst - start;
        }

        const encoder = new TextEncoder();
        const result = encoder.encodeInto(str, Heap.u8.subarray(dst, dst + dstLen));
        return result.written;
    };

    /// Reads a boolean value from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readBool(ptr: ptr<boolean> | const_ptr<boolean>): boolean { return !!Heap.u8[ptr]; };

    /// Writes a JavaScript boolean value to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeBool(ptr: ptr<boolean>, value: boolean) { Heap.u8[ptr] = value ? 1 : 0; };

    /// Reads an 8-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readInt8(ptr: ptr<i8> | const_ptr<i8>): i8 { return Heap.i8[ptr] as i8; };

    /// Writes a JavaScript number value as an 8-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeInt8(ptr: ptr<i8>, value: i8) { Heap.i8[ptr] = value; };

    /// Reads a 16-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readInt16(ptr: ptr<i16> | const_ptr<i16>): i16 { return Heap.i16[ptr] as i16; };

    /// Writes a JavaScript number value as a 16-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeInt16(ptr: ptr<i16>, value: i16) { Heap.i16[ptr] = value; };

    /// Reads a 32-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readInt32(ptr: ptr<i32> | const_ptr<i32>): i32 { return Heap.i32[ptr] as i32; };

    /// Writes a JavaScript number value as a 32-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeInt32(ptr: ptr<i32>, value: i32) { Heap.i32[ptr] = value; };

    /// Reads a 64-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readInt64(ptr: ptr<i64> | const_ptr<i64>): i64 { return Heap.i64[ptr] as i64; };

    /// Writes a JavaScript BigInt value as a 64-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeInt64(ptr: ptr<i32>, value: i64) { Heap.i64[ptr] = value; };

    /// Reads an 8-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readUint8(ptr: ptr<u8> | const_ptr<u8>): u8 { return Heap.u8[ptr] as u8; };

    /// Writes a JavaScript number value as an 8-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeUint8(ptr: ptr<u8>, value: u8) { Heap.u8[ptr] = value; };

    /// Reads a 16-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readUint16(ptr: ptr<u16> | const_ptr<u16>): u16 { return Heap.u16[ptr] as u16; };

    /// Writes a JavaScript number value as a 16-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeUint16(ptr: ptr<u16>, value: u16) { Heap.u16[ptr] = value; };

    /// Reads a 32-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readUint32(ptr: ptr<u32> | const_ptr<u32>): u32 { return Heap.u32[ptr] as u32; };

    /// Writes a JavaScript number value as a 32-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeUint32(ptr: ptr<u32>, value: u32) { Heap.u32[ptr] = value; };

    /// Reads a 64-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readUint64(ptr: ptr<u64> | const_ptr<u64>): u64 { return Heap.u64[ptr] as u64; };

    /// Writes a JavaScript BigUint value as a 64-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeUint64(ptr: ptr<u64>, value: u64) { Heap.u64[ptr] = value; };

    /// Reads a 32-bit floating point number from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readFloat32(ptr: ptr<f32> | const_ptr<f32>): f32 { return Heap.f32[ptr] as f32; };

    /// Writes a JavaScript number value as a 32-bit floating point number to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeFloat32(ptr: ptr<f32>, value: f32) { Heap.f32[ptr] = value; };

    /// Reads a 64-bit floating point number from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    static readFloat64(ptr: ptr<f64> | const_ptr<f64>): f64 { return Heap.f64[ptr] as f64; };

    /// Writes a JavaScript number value as a 64-bit floating point number to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    static writeFloat64(ptr: ptr<f64>, value: f64) { Heap.f64[ptr] = value; };
}

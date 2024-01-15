// Copyright Chad Engler

(function()
{
    "use strict";
    window.HE = window.HE || {};

    // --------------------------------------------------------------------------------------------
    // Pointers library
    HE.ptr = HE.ptr || {};

    /// Returns the number of bytes required to store the given string as a UTF-8 byte array.
    ///
    /// \param[in] str The JavaScript string to measure.
    /// \return The number of bytes required to store the string as a UTF-8 byte array.
    HE.ptr.utf8StrLen = function (str)
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
    HE.ptr.readString = function (ptr, len)
    {
        if (len == 0 || !ptr)
            return "";

        const heap = HE.heap.u8;

        if (len < 0)
        {
            len = 0;
            while (heap[ptr + len])
                ++len;
        }

        // TextDecoder.decode() doesn't work with a view of a ShredArrayBuffer, so make a copy.
        // See: https://github.com/whatwg/encoding/issues/172
        const array = heap.buffer instanceof SharedArrayBuffer ? heap.slice(ptr, ptr + len) : heap.subarray(ptr, ptr + len);
        const decoder = new TextDecoder('utf8');
        return decoder.decode(array);
    };

    /// Writes a string to the heap at the given pointer.
    ///
    /// \param[in] str The JavaScript string to copy from.
    /// \param[in] dst The destination buffer to write to.
    /// \param[in] dstLen The length of the destination buffer.
    /// \return The number of bytes written to the destination buffer.
    HE.ptr.writeString = function (str, dst, dstLen)
    {
        if (dstLen <= 0)
            return 0;

        const heap = HE.heap.u8;
        const start = dst;

        if (heap.buffer instanceof SharedArrayBuffer)
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
                    heap[dst++] = c;
                }
                else if (c <= 0x7ff)
                {
                    if (dst + 1 >= end)
                        break;
                    heap[dst++] = 0xc0 | (c >> 6);
                    heap[dst++] = 0x80 | (c & 0x3f);
                }
                else if (c >= 0xd800 && c <= 0xdfff)
                {
                    if (dst + 3 >= end)
                        break;
                    c = ((c - 0xd800) << 10) + (str.charCodeAt(++i) - 0xdc00) + 0x10000;
                    heap[dst++] = 0xf0 | (c >> 18);
                    heap[dst++] = 0x80 | ((c >> 12) & 0x3f);
                    heap[dst++] = 0x80 | ((c >> 6) & 0x3f);
                    heap[dst++] = 0x80 | (c & 0x3f);
                }
                else
                {
                    if (dst + 2 >= end)
                        break;
                    heap[dst++] = 0xe0 | (c >> 12);
                    heap[dst++] = 0x80 | ((c >> 6) & 0x3f);
                    heap[dst++] = 0x80 | (c & 0x3f);
                }
            }
            heap[dst] = 0;
            return dst - start;
        }

        const encoder = new TextEncoder('utf8');
        const result = encoder.encodeInto(str, heap.subarray(dst, dst + dstLen));
        return result.written;
    };

    /// Reads a boolean value from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readBool = function (ptr) { return !!HE.heap.u8[ptr]; };

    /// Writes a JavaScript boolean value to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeBool = function (ptr, value) { HE.heap.u8[ptr] = value; };

    /// Reads an 8-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readInt8 = function (ptr) { return HE.heap.i8[ptr]; };

    /// Writes a JavaScript number value as an 8-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeInt8 = function (ptr, value) { HE.heap.i8[ptr] = value; };

    /// Reads a 16-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readInt16 = function (ptr) { return HE.heap.i16[ptr]; };

    /// Writes a JavaScript number value as a 16-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeInt16 = function (ptr, value) { HE.heap.i16[ptr] = value; };

    /// Reads a 32-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readInt32 = function (ptr) { return HE.heap.i32[ptr]; };

    /// Writes a JavaScript number value as a 32-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeInt32 = function (ptr, value) { HE.heap.i32[ptr] = value; };

    /// Reads a 64-bit signed integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readInt64 = function (ptr) { return HE.heap.i64[ptr]; };

    /// Writes a JavaScript BigInt value as a 64-bit signed integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeInt64 = function (ptr, value) { HE.heap.i64[ptr] = value; };

    /// Reads an 8-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readUint8 = function (ptr) { return HE.heap.u8[ptr]; };

    /// Writes a JavaScript number value as an 8-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeUint8 = function (ptr, value) { HE.heap.u8[ptr] = value; };

    /// Reads a 16-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readUint16 = function (ptr) { return HE.heap.u16[ptr]; };

    /// Writes a JavaScript number value as a 16-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeUint16 = function (ptr, value) { HE.heap.u16[ptr] = value; };

    /// Reads a 32-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readUint32 = function (ptr) { return HE.heap.u32[ptr]; };

    /// Writes a JavaScript number value as a 32-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeUint32 = function (ptr, value) { HE.heap.u32[ptr] = value; };

    /// Reads a 64-bit unsigned integer from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readUint64 = function (ptr) { return HE.heap.u64[ptr]; };

    /// Writes a JavaScript BigUint value as a 64-bit unsigned integer to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeUint64 = function (ptr, value) { HE.heap.u64[ptr] = value; };

    /// Reads a 32-bit floating point number from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readFloat32 = function (ptr) { return HE.heap.f32[ptr]; };

    /// Writes a JavaScript number value as a 32-bit floating point number to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeFloat32 = function (ptr, value) { HE.heap.f32[ptr] = value; };

    /// Reads a 64-bit floating point number from the heap.
    ///
    /// \param[in] ptr The pointer to read from.
    /// \return The value read from the heap.
    HE.ptr.readFloat64 = function (ptr) { return HE.heap.f64[ptr]; };

    /// Writes a JavaScript number value as a 64-bit floating point number to the heap.
    ///
    /// \param[in] ptr The pointer to write to.
    /// \param[in] value The value to write to the heap.
    HE.ptr.writeFloat64 = function (ptr, value) { HE.heap.f64[ptr] = value; };

    // --------------------------------------------------------------------------------------------
    // Heap library
    HE.heap = HE.heap || {};

    HE.heap.create = function (size)
    {
        if (HE.heap.u8)
            throw "HE.heap.create() called more than once";

        // TODO: Shared memory between workers.
        const memory = new WebAssembly.Memory({
            initial: wasmMemInitial >> 16,
            shared: true,
            // TODO: Memory64
            // index: 'i64',
        });
        HE.heap.memory = memory;
        HE.heap.i8 = new Int8Array(memory.buffer);
        HE.heap.i16 = new Int16Array(memory.buffer);
        HE.heap.i32 = new Int32Array(memory.buffer);
        HE.heap.i64 = new BigInt64Array(memory.buffer);
        HE.heap.u8 = new Uint8Array(memory.buffer);
        HE.heap.u16 = new Uint16Array(memory.buffer);
        HE.heap.u32 = new Uint32Array(memory.buffer);
        HE.heap.u64 = new BigUint64Array(memory.buffer);
        HE.heap.f32 = new Float32Array(memory.buffer);
        HE.heap.f64 = new Float64Array(memory.buffer);
        return heap;
    };

    // --------------------------------------------------------------------------------------------
    // Env functions (imports)
    HE.env = HE.env || {};

    let batteryManager = null;
    navigator.getBattery().then(function (b) { batteryManager = b; });

    HE.env.heWASM_Alert = function (msg)
    {
        const str = HE.ptr.readString(msg);
        alert(str);
    };

    HE.env.heWASM_ConsoleLog = function (level, msg)
    {
        const str = HE.ptr.readString(msg);
        switch (level)
        {
            case 1: // Debug
                console.debug(str);
                break;
            case 2: // Info
                console.info(str);
                break;
            case 3: // Warn
                console.warn(str);
                break;
            case 4: // Error
                console.error(str);
                break;
            case 0: // Log
            default:
                console.log(str);
                break;
        }
    };

    HE.env.heWASM_Debugger = function ()
    {
        debugger;
    };

    HE.env.heWASM_GetDateNow = function ()
    {
        return Date.now();
    };

    HE.env.heWASM_GetDateTzOffset = function ()
    {
        return new Date().getTimezoneOffset();
    };

    HE.env.heWASM_IsDaylightSavingTimeActive = function ()
    {
        const today = new Date();
        const jan = new Date(today.getFullYear(), 0, 1);
        const jul = new Date(today.getFullYear(), 6, 1);
        const std = Math.max(jan.getTimezoneOffset(), jul.getTimezoneOffset());
        return today.getTimezoneOffset() < std;
    };

    HE.env.heWASM_GetPerformanceNow = function ()
    {
        return performance.now();
    };

    HE.env.heWASM_GetHardwareConcurrency = function ()
    {
        return navigator.hardwareConcurrency;
    };

    HE.env.heWASM_GetUserAgentLength = function ()
    {
        return HE.ptr.utf8StrLen(navigator.userAgent.length);
    };

    HE.env.heWASM_GetUserAgent = function (dst, dstLen)
    {
        HE.ptr.writeString(navigator.userAgent, dst, dstLen);
    };

    HE.env.heWASM_GetBatteryStatus = function (chargingTime, dischargingTime, level, charging)
    {
        if (!batteryManager)
            return false;

        HE.ptr.writeFloat64(chargingTime, batteryManager.chargingTime);
        HE.ptr.writeFloat64(dischargingTime, batteryManager.dischargingTime);
        HE.ptr.writeFloat64(level, batteryManager.level);
        HE.ptr.writeBool(charging, batteryManager.charging);
        return true;
    };
})();

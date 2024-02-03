// Copyright Chad Engler

import { lib } from './lib';

export class Heap
{
    static memory: WebAssembly.Memory;
    static buffer: ArrayBuffer | SharedArrayBuffer;
    static i8: Int8Array;
    static i16: Int16Array;
    static i32: Int32Array;
    static i64: BigInt64Array;
    static u8: Uint8Array;
    static u16: Uint16Array;
    static u32: Uint32Array;
    static u64: BigUint64Array;
    static f32: Float32Array;
    static f64: Float64Array;

    static create(desc: WebAssembly.MemoryDescriptor)
    {
        Heap.memory = new WebAssembly.Memory(desc);
        Heap._init();
    }

    static set(memory: WebAssembly.Memory)
    {
        Heap.memory = memory;
        Heap._init();
    }

    private static _init()
    {
        Heap.buffer = Heap.memory.buffer;
        Heap.i8 = new Int8Array(Heap.buffer);
        Heap.i16 = new Int16Array(Heap.buffer);
        Heap.i32 = new Int32Array(Heap.buffer);
        Heap.i64 = new BigInt64Array(Heap.buffer);
        Heap.u8 = new Uint8Array(Heap.buffer);
        Heap.u16 = new Uint16Array(Heap.buffer);
        Heap.u32 = new Uint32Array(Heap.buffer);
        Heap.u64 = new BigUint64Array(Heap.buffer);
        Heap.f32 = new Float32Array(Heap.buffer);
        Heap.f64 = new Float64Array(Heap.buffer);

        lib.addImports('env', { memory: Heap.memory });
    }
}

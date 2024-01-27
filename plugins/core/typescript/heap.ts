// Copyright Chad Engler

export class Heap
{
    memory: WebAssembly.Memory;
    buffer: ArrayBuffer | SharedArrayBuffer;
    i8: Int8Array;
    i16: Int16Array;
    i32: Int32Array;
    i64: BigInt64Array;
    u8: Uint8Array;
    u16: Uint16Array;
    u32: Uint32Array;
    u64: BigUint64Array;
    f32: Float32Array;
    f64: Float64Array;

    constructor(desc: WebAssembly.MemoryDescriptor)
    {
        this.memory = new WebAssembly.Memory(desc);
        this.buffer = this.memory.buffer;
        this.i8 = new Int8Array(this.buffer);
        this.i16 = new Int16Array(this.buffer);
        this.i32 = new Int32Array(this.buffer);
        this.i64 = new BigInt64Array(this.buffer);
        this.u8 = new Uint8Array(this.buffer);
        this.u16 = new Uint16Array(this.buffer);
        this.u32 = new Uint32Array(this.buffer);
        this.u64 = new BigUint64Array(this.buffer);
        this.f32 = new Float32Array(this.buffer);
        this.f64 = new Float64Array(this.buffer);
    }
}

let heap: Heap | null = null;

export function getDefaultHeap(): Heap
{
    if (!heap)
    {
        throw new Error('Heap not initialized.');
    }

    return heap;
}

export function createDefaultHeap(desc: WebAssembly.MemoryDescriptor)
{
    heap = new Heap(desc);
}

// Copyright Chad Engler

import { Heap } from "./heap";
import { lib } from "./lib";

class _BufferReader
{
    private _u8: Uint8Array;
    private _index: number;

    constructor(buffer: ArrayBuffer, startIndex: number = 0)
    {
        this._u8 = new Uint8Array(buffer);
        this._index = startIndex;
    }

    get length(): number
    {
        return this._u8.length;
    }

    // reads a single byte
    readU8(): number
    {
        return this._u8[this._index++];
    }

    // reads a variable-length LEB128 number
    readLEB128(): number
    {
        let ret = 0;
        let next = 128;
        for (const start = this._index; next & 128; ++this._index)
        {
            next = this._u8[this._index];
            ret |= (next & 127) << ((this._index - start) * 7);
        }
        return ret;
    }
}

export class Loader
{
    static async loadModule(source: URL | string): Promise<WebAssembly.Instance>
    {
        const res = await fetch(source);
        const buffer = await res.arrayBuffer();
        const reader = new _BufferReader(buffer);

        // Find the start point of the heap to calculate the initial memory requirements.
        // To do this we scan through the wasm binary looking for the 'Globals' section.
        // LLVM places the heap base pointer into the first value of that section.
        // See: https://webassembly.org/docs/binary-encoding/
        let heapBase = 65536;
        for (let i = 8, sectionEnd; i < reader.length; i = sectionEnd)
        {
            const type = reader.readLEB128();
            const length = reader.readLEB128();
            sectionEnd = i + length;

            if (type < 0 || type > 11 || length <= 0 || sectionEnd > reader.length)
                break;

            if (type == 6)
            {
                // Section 6 'Globals', llvm places the heap base pointer into the first value here
                const count = reader.readLEB128();
                const gtype = reader.readU8();
                const mutable = reader.readU8();
                const opcode = reader.readLEB128();
                const offset = reader.readLEB128();
                const endcode = reader.readLEB128();
                heapBase = offset;
                break;
            }
        }

        // Create the default heap used by the application.
        const initialDataSize = ((heapBase + 65535) >> 16) << 16;
        const initialHeapSize = 64 * 1024 * 1024;
        const maximumHeapSize = 3 * 1024 * 1024 * 1024;
        Heap.create({
            initial: (initialDataSize + initialHeapSize) >> 16,
            maximum: maximumHeapSize >> 16,
            shared: true,
        });

        const module = await WebAssembly.instantiate(buffer, lib.imports);

        lib.exports = module.instance.exports;
        lib.module = module.module;
        return module.instance;
    }

    static async instantiateModule(source: WebAssembly.Module): Promise<WebAssembly.Instance>
    {
        const instance = await WebAssembly.instantiate(source, lib.imports);

        lib.exports = instance.exports;
        return instance;
    }
}

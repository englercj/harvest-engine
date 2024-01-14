// Fetch the .wasm file and store its bytes into the byte array wasmBytes
fetch('output.wasm')
    .then(res => res.arrayBuffer())
    .then(function(wasmBytes)
    {
        wasmBytes = new Uint8Array(wasmBytes);

        // Find the start point of the heap to calculate the initial memory requirements
        var wasmHeapBase = 65536;

        // This code goes through the wasm file sections according the binary encoding description
        //     https://webassembly.org/docs/binary-encoding/
        for (let i = 8, sectionEnd, type, length; i < wasmBytes.length; i = sectionEnd)
        {
            // Get() gets the next single byte, GetLEB() gets a LEB128 variable-length number
            function Get() { return wasmBytes[i++]; }
            function GetLEB() { for (var s=i,r=0,n=128; n&128; i++) r|=((n=wasmBytes[i])&127)<<((i-s)*7); return r; }
            type = GetLEB(), length = GetLEB(), sectionEnd = i + length;
            if (type < 0 || type > 11 || length <= 0 || sectionEnd > wasmBytes.length) break;
            if (type == 6)
            {
                //Section 6 'Globals', llvm places the heap base pointer into the first value here
                let count = GetLEB(), gtype = Get(), mutable = Get(), opcode = GetLEB(), offset = GetLEB(), endcode = GetLEB();
                wasmHeapBase = offset;
                break;
            }
        }

        // Set the wasm memory size to [DATA] + [STACK] + [256KB HEAP]
        // (This loader does not support memory growing so it stays at this size)
        var wasmMemInitial = ((wasmHeapBase+65535)>>16<<16) + (256 * 1024);
        var env = { memory: new WebAssembly.Memory({initial: wasmMemInitial>>16 }) };

        // Instantiate the wasm module by passing the prepared environment
        WebAssembly.instantiate(wasmBytes, {env:env}).then(function (output)
        {
            // Store the list of the functions exported by the wasm module in WA.asm
            WA.asm = output.instance.exports;

            // If function '__wasm_call_ctors' (global C++ constructors) exists, call it
            if (WA.asm.__wasm_call_ctors) WA.asm.__wasm_call_ctors();

            // If function 'main' exists, call it with dummy arguments
            if (WA.asm.main) WA.asm.main(0, 0);

            // If the outer HTML file supplied a 'started' callback, call it
            if (WA.started) WA.started();
        })
        .catch(function (err)
        {
            // On an exception, if the err is 'abort' the error was already processed in the abort function above
            if (err !== 'abort') abort('BOOT', 'WASM instiantate error: ' + err + (err.stack ? "\n" + err.stack : ''));
        });
    });

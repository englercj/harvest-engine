// Copyright Chad Engler

import { i32 } from './ctypes';

export class Library
{
    get imports(): Readonly<WebAssembly.Imports> { return this._imports; }

    get exports(): Readonly<WebAssembly.Exports> { return this._exports; }
    set exports(ex: WebAssembly.Exports) { this._exports = ex; }

    module: WebAssembly.Module | null = null;
    exitCode: i32 = 0 as i32;

    addImports(module: string, imports: WebAssembly.ModuleImports)
    {
        if (!this._imports[module])
            this._imports[module] = {};

        Object.assign(this._imports[module], imports);
    }

    private _imports: WebAssembly.Imports = {};
    private _exports: WebAssembly.Exports = {};
}

export const lib = new Library();

// Copyright Chad Engler

export class Library
{
    addImports(module: string, imports: WebAssembly.ModuleImports)
    {
        if (!this._imports[module])
            this._imports[module] = {};

        Object.assign(this._imports[module], imports);
    }

    setExports(exports: WebAssembly.Exports)
    {
        this._exports = exports;
    }

    setModule(module: WebAssembly.Module)
    {
        this._module = module;
    }

    get imports(): Readonly<WebAssembly.Imports>
    {
        return this._imports;
    }

    get exports(): Readonly<WebAssembly.Exports>
    {
        return this._exports;
    }

    get module(): WebAssembly.Module | null
    {
        return this._module;
    }

    private _imports: WebAssembly.Imports = {};
    private _exports: WebAssembly.Exports = {};
    private _module: WebAssembly.Module | null = null;
}

export const lib = new Library();

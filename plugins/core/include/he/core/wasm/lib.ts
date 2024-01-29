// Copyright Chad Engler

export class Library
{
    readonly imports: WebAssembly.Imports = {};

    addImports(module: string, imports: WebAssembly.ModuleImports)
    {
        if (!this.imports[module])
            this.imports[module] = {};

        Object.assign(this.imports[module], imports);
    }
}

export const lib = new Library();

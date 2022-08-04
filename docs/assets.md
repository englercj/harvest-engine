# Harvest Assets

## Asset Processors

There are two kinds of asset processors:

1. Asset Compilers, which convert a single source asset into N game-ready resources.
2. Asset Importers, which convert a single source content into N editable assets and M cache resources.

The flow generally looks like:

```
|--------|                 |-------|                 |----------|
| Source | -- Importer --> | Asset | -- Compiler --> | Resource |
|--------|                 |-------|                 |----------|
```

Keep in mind that an importer can emit resources as well as assets, and the compiler may take resources as input in addition to outputting them.

Consider a Photoshop Document (psd) file. The importer may parse this file and create multiple assets: one for each layer. But when the compiler converts those assets into game-ready data we don't want it to have to dig back into the PSD. So, the importer also creates a resource for each layer that is the extracted from the PSD. The compiler
can then utilize that data instead of parsing the PSD.

This also has the benefit of isolating DCC dependencies into the Importer. A machine that imports a PSD file may need access to Photoshop, but the compiler does not. Combine this with a shared resource cache and you have have machines without Photoshop compile assets that were originally imported from PSD files!

# Plugin File Spec

All keys are optional unless otherwise specified.

All paths can contain globs and are relative to the he_plugin file, or the install directory if the plugin is installed from a remote source. That is, if you use "install.github", "install.bitbucket", "install.nuget", or "install.archive" to install the plugin then paths are relative to the extracted files location.

## Plugin Keys

|      Key      |       Value Type      | Description |
| ------------- | --------------------- | ----------- |
| id            | String                | Required. Globally unique identifier for the plugin. |
| name          | String                | Friendly name of the plugin meant for humans. |
| description   | String                | Short description fo the plugin meant for humans. |
| version       | String                | An arbitrary version string identifying the version of the plugin. |
| author        | String|Array<String>  | The plugin's author name and email, in the format "Name <email>". |
| license       | String                | An SPDX license identifier (https://spdx.org/licenses/), or "UNLICENSED", or "SEE LICENSE IN <filename>". If this is not specified it is treated as "UNLICENSED". |
| warnings      | String                | Desired level of warning: "Off", "Default", or "Extra". The default is "Extra" |
| tags          | Array<String>         | An array of string identifiers used as search tags. |
| modules       | Array<Module>         | An array of modules that this plugin provides. See the Module Keys section. |
| install       | Install               | Description of how to install the plugin. |

## Install Keys

|      Key      |   Value Type  | Description |
| ------------- | ------------- | ----------- |
| exec          | String        | Path to a lua file that returns a function for execution when the plugin is imported. Use as an entry point for build system extension. |
| valid_targets | Array<String> | List of target system tags this plugin is valid to be installed on. |
| valid_hosts   | Array<String> | List of host system tags this plugin is valid to be installed on. |
| archive       | mixed         | Either a string URL of the archive to download, or an object where the key is a system name and the value is a string URL of the archive to download for that system. |
| index_archive_by_host | Boolean | Tells the installer to index the archive object by *host* system rather than *target* system. This can be useful for plugins that provide only executables and should be downloaded based on the host system that will run them, rather than the target system being built for. |
| source        | String        | Path relative to the install directory to the plugin. This is useful if the plugin's contents are in a different location than the he_plugin file. |
| github        | String        | A string repository specifier in the form "<user>/<repo>#<commit-ish>" |
| bitbucket     | String        | A string repository specifier in the form "<user>/<repo>#<commit-ish>" |
| nuget         | String        | A string with the package name and version in the form "<package>#<version>" |
| basepath      | String        | A path relative to the install directory to append to the install path returned by any of the other install sources. |

## Module Keys

|            Key            |     Value Type    | Description |
| ------------------------- | ----------------- | ----------- |
| name                      | String            | Required. Globally unique name of the module. Also used as the project name. |
| type                      | String            | See the Module Types section for valid values. |
| group                     | String            | Name of the group this module belongs to. This will be a virtual folder in the solution tree. |
| language                  | String            | Language of the source to be compiled in the module project. Default is "C++" if not specified. |
| files                     | Array<String>     | File paths to include in the module project. |
| post_build_commands       | Array<String>     | Commands to run after the module is built. |
| pre_build_commands        | Array<String>     | Commands to run before the module is built. |
| dependson_runtime         | Array<String>     | Module names this module will use at runtime, but are not required to build. See the Module Dependencies section for more details. |
| variants                  | Array<Variant>    | Variations of the module's properties activated by a filter. See the Variant Keys section. |
| exec                      | String            | Path to a lua file that return a function for execution in the context of the module's project. Use as a last resort for advanced module project functionality. |

### Prefixed Module Keys

The following keys must be prefixed with "public" or "private". The former means to propagate that value to any modules depending on this module, and the later means it only affects this module's project. For example, "public_defines" and "private_defines".

| *_defines           | Array<String>     | Symbols to define. Use the syntax "SYMBOL=X" to define the symbol with a value. |
| *_dependson         | Array<String>     | Module names this module requires to build. See the Module Dependencies section for more details. |
| *_dependson_include | Array<String>     | Module names this module requires include-only access to. See the Module Dependencies section for more details. |
| *_includedirs       | Array<String>     | Include paths required for this module to build. |

### Module Variant Keys

Variants can include any Module Key which will only be applied when that variant is active (based on the "condition"). The only exceptions are "name", "type", and "variants" which cannot be overridden by a variant.

Variants are treated as additive and will have the effect of adding new keys to the module when they are active.

|      Key      |     Value Type    | Description |
| ------------- | ----------------- | ----------- |
| conditions    | Array<String>     | Conditions that must be true for the variant to be active. The values here are passed directly to premake's [filter](https://premake.github.io/docs/filter). |

## Module Types

| Type | Description |
| ---- | ----------- |
| default | The default module type. Built as a hot-reloadable shared library during dynamic (internal) builds and as a static library during static (shipping) builds. |
| static | Always built as a static library. |
| shared | Always built as a shared library (dll/so). |
| custom | Custom build steps like copying prebuilt binaries. Depending on a module of this type doesn't generate any link commands. |
| header | Header-only module that does not generate any symbols to be linked. |
| console_app | A console application |
| windowed_app | A windowed application |

## Module Dependencies

When depending on a module the actual effect of that dependency on the generated project depends on a few factors:

- The type of the build being generated (dynamic or static)
- The type of the module that has the dependency
- The type of the module that is being depended on

For example, a "default" module depending on a "static" module in a dynamic build will link the static library and inherit the public include paths. During a static build it will only inherit the public include paths and mark the dependency so any application using the module links both.

The "public_dependson" and "public_dependson_include" keys will propagate those values through the module dependency tree so that other modules inherit those values when their projects are generated. The "private_dependson" and "private_dependson_include" keys do not propagate and are used only in that module's project generation.

### Dependency Prefixes

When a dependency is specified with no prefix, for example "he_core" it is assumed the value refers to a module. However, there are also a few supported prefixes that change how the dependency name is interpreted:

| Prefix | Description |
| ------ | ----------- |
| module: | The name refers to a module. This is the default, and is not required. |
| sys: | The name refers to a system library. Do not include any prefix or file extension, the system will deduce those based on the target system. |
| file: | The name refers to a file path to a library. The path is used exactly as-is so you will need to include any prefix or file extension information. |

Example: `"public_dependson": ["he_core", "module:he_platform", "sys:user32", "file:mylib.lib"]`

## Module Keys Extension

You can extend the supported keys by calling the `he.add_module_key` function in a lua function specified in your plugin's `install/exec` key.

For example:

`game/tools/plugin/he_plugin.toml`
```toml
id = "game.tools.plugin"
name = "My Tool Plugin"

[install]
    exec = "my_tool.lua"
```

`game/tools/plugin/my_tool.lua`
```lua
return function (plugin)
    -- plugin = the parsed he_plugin file

    he.add_module_key {
        key = "mytool",
        scope = "private",
        type = "string",
        desc = "a string path to the files to run mytool on, globs are allowed",
        handler = function (ctx, value)
            -- ctx = the module that used the "mytool" key
            -- value = the value of the "mytool" key in the module

            -- The project for the module that used the "mytool" key is currently in scope
            -- Paths are relative to the he_plugin file that contained the module.
            files { value }
            dependson { "mytool" }

            -- It is important to use filter_push_combine as that will push a filter on the stack
            -- that combines the files filter with the current active filter. Not doing this will
            -- have incorrect results when your tool is used in a variant.
            he.filter_push_combine { "files:" .. value }
                buildmessage "Running mytool on file %{file.abspath}"
                buildcommands { he.target_bin_dir .. "/mytool %{file.abspath} -o " .. he.file_gen_dir }
                buildoutputs { he.file_gen_dir .. "/%{file.name}.h" }
            he.filter_pop()
        end,
    }
end
```

Now another module can use the new `mytool` key and it will execute the `handler` function.

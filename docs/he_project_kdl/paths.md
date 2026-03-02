# Paths

Paths in HE Make are separated by a forward slash (`/`), regardless of the platform being configured. The system will automatically normalize paths as necessary.

## Relative paths

All relative paths are assumed to be relative to the `*.kdl` file they are written in. You can use [tokens](tokens.md) to build complex paths if needed.

## Glob Patterns

All keys that accept paths to files can contain globs patterns. An asterisk (`*`) character represents a wildcard that matches against files in the current directory. A double asterisk (`**`) represents a wildcard that matches recursively through directories.

## Example

```kdl
module my_module type=static group="engine/libs" {
    files {
        // Path to a file that lives next to the 'he_plugin.kdl' file.
        "user_config.h"

        // Path to the 'library.cpp' file, relative to the plugin's install
        // directory within the build directory.
        "${plugin.install_dir}/library.cpp"

        // Matches all files that end with `.cpp` in the `src` directory
        "src/*.cpp"

        // Matches all files that end with `.cpp` in the `src` directory, and any child directories
        "src/**.cpp"

        // Matches all files in any directory that have `.win32.` in the name
        "**.win32.*"
    }
}
```

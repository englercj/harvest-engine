# Paths

Paths in HE Make are separated by a forward slash (`/`), regardless of the platform being configured. The system will automatically normalize paths as necessary.

## Relative paths

Paths in a plugin or module scope are specified relative to the plugin's install location. For plugins with an `install {}` block, the install location is the extracted files location in the `build/` directory. For plugins without an `install {}` block, this is the same directory where the `he_plugin.kdl` file is located.

## Glob Patterns

All keys that accept paths to files can contain globs patterns. An asterisk (`*`) character represents a wildcard that matches against files in the current directory. A double asterisk (`**`) represents a wildcard that matches recursively through directories.

## Example

```kdl
module my_module type=static group="engine/libs" {
    files {
        // Path to a file that lives next to the 'he_plugin.kdl' file.
        "${plugin.path:dirname}/user_config.h"

        // Path to the 'library.cpp' file, relative to the plugin's install
        // directory within the build directory.
        "library.cpp"

        // Matches all files that end with `.cpp` in the `src` directory
        "src/*.cpp"

        // Matches all files that end with `.cpp` in the `src` directory, and any child directories
        "src/**.cpp"

        // Matches all files in any directory that have `.win32.` in the name
        "**.win32.*"
    }
}
```

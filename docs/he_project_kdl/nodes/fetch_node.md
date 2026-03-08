# `fetch` node

Defines how to fetch install data for a plugin, and what the base directory for those installed files are. Each fetch is extracted under `${project.installs_dir}` into a folder named `<prefix>-<sha256>`, where the prefix is derived from the archive source (for example a repo or package name).

## Arguments

1. (string) - Required. The method to use for fetching the data. Valid values are:
    - `archive` - Download and extract an archive file from a URL.
    - `bitbucket` - Download a version of a bitbucket repo.
    - `github` - Download a version of a github repo.
    - `nuget` - Download a version of a nuget package.

## Properties

- `base_dir` (string) - Optional. The base path to append to the extracted directory where downloaded files are placed. By default a base directory is guessed based on the fetch method.
- `install_dir_priority` (int) - Optional. The priority of this fetch node when a plugin is determining its install directory. higher priority values are selected over lower ones.
    * In most cases there is only a single fetch node active per configuration, and this property isn't needed. That is, usually there is a only one per-system or one per-host.
    * This is only necessary in rare cases where a plugin has multiple fetch nodes that are active for a single configuration, and the plugin needs to disambiguate which one defines the "install directory" of the parent plugin.

Additional properties are defined depending on the fetch method specified as the first argument to the node. Those properties are detailed below.

### `archive` method

The `archive` method supports the following properties:

- `url` (string) - Required. The full URL to download the archive from.
- `archive_format` (string) - Optional. The format of the archive file. By default this is deduced from the URL by checking the extension of the file name, and `zip` is assumed if the extension is not recognized. If those deduction rules do not work for your URL, use this property to specify it explicitly. Valid values are:
    * `zip` - A ZIP archive file.
    * `tar` - An uncompressed tarball file.
    * `tar.gz` - A GZIP compressed tarball file.

### `bitbucket` method

The `bitbucket` method supports the following properties:

- `user` (string) - Required. The name of the user or organization that owns the repository.
- `repo` (string) - Required. The name of the repository.
- `ref` (string) - Required. A commit sha, tag name, or branch name.

### `github` method

The `github` method supports the following properties:

- `user` (string) - Required. The name of the user or organization that owns the repository.
- `repo` (string) - Required. The name of the repository.
- `ref` (string) - Required. A commit sha, tag name, or branch name.

### `nuget` method

The `nuget` method supports the following properties:

- `package` (string) - Required. The name of the package.
- `version` (string) - Required. The version of the package.

## Children

None.

## Scopes

- [`install`](install_node.md)

## Example

```kdl
// Fetch sqlite from an archive URL.
// With a raw archive Harvest can't guess the structure of the extracted zip, so an explicit base_dir is needed.
fetch archive url="https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip" base_dir="sqlite-amalgamation-3420000"

// Fetch a version of a bitbucket repository.
// Since bitbucket has a consistent archive format Harvest can guess what a good base_dir is without an explicit dirname.
fetch bitbucket user=ocornut repo=imgui ref=823a1385a269d923d35b82b2f470f3ae1fa8b5a3

// Fetch a version of a github repository.
// Since github has a consistent archive format Harvest can guess what a good base_dir is without an explicit dirname.
fetch github user=ocornut repo=imgui ref=823a1385a269d923d35b82b2f470f3ae1fa8b5a3

// Fetch a version of a nuget package.
// Since nuget has a consistent archive format Harvest can guess what a good base_dir is without an explicit dirname.
fetch nuget Microsoft.Direct3D.DirectStorage version="1.2.1"
```

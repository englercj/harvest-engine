# `install` node

Defines how a plugin is to be installed. It is common for contrib libraries to only commit a plugin file to source control and to have their source or binaries downloaded when the project is initialized.

## Arguments

None.

## Properties

None.

## Children

- [`fetch`](fetch_node.md)
- [`when`](when_node.md)

## Scopes

- [`plugin`](plugin_node.md)

## Example

```kdl
// install sqlite from archive URL
install {
    fetch archive url="https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip" base_dir="sqlite-amalgamation-3420000"
}

// install imgui from github
install {
    fetch github user=ocornut repo=imgui ref=823a1385a269d923d35b82b2f470f3ae1fa8b5a3
}

// install DirectStorage from nuget, only if there is a windows build target
install {
    when system=windows {
        fetch nuget Microsoft.Direct3D.DirectStorage version="1.2.1"
    }
}

// install Slang from archive URL, only if there is a windows or linux build target
install {
    when system=windows {
        fetch archive url="https://github.com/shader-slang/slang/releases/download/v0.28.2/slang-0.28.2-win64.zip"
    }

    when system=linux {
        fetch archive url="https://github.com/shader-slang/slang/releases/download/v0.28.2/slang-0.28.2-linux-x86_64.zip"
    }
}

// install node from archive URL, only when on a windows or linux host
install {
    when host=windows {
        fetch archive url="https://nodejs.org/dist/v20.11.0/node-v20.11.0-win-x64.zip" base_dir="node-v20.11.0-win-x64/node-v20.11.0-win-x64"
    }

    when host=linux {
        fetch archive url="https://nodejs.org/dist/v20.11.0/node-v20.11.0-linux-x64.tar.xz" base_dir="node-v20.11.0-linux-x64/node-v20.11.0-linux-x64"
    }
}
```

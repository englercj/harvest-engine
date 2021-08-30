# Getting Started

## Setup Your Workspace

The recommended file structure is:

```
my_game/
    assets/
    plugins/
    premake5.lua
harvest-engine/
    plugins/
    scripts/
```

Where `my_game` is your project's source, and `harvest-engine` is the engine source or a binary distribution. See Managing Source Control for recommendations on storage.

In the root of your project folder create a `premake5.lua` file that imports the engine and your game:

```lua
-- Copyright Chad Engler

include "../harvest-engine/scripts/_setup.lua"

he_workspace()
    startproject "editor"

    import_plugins({
        "../harvest-engine",    -- Import the engine plugins
        "plugins/*",            -- Import your game's plugins
    }, {
        -- Don't include engine tests in your workspace unless you want to build & run them
        exclude_groups = { "engine/tests" },
    })
```

## Generate Your Project

Harvest includes a `bootstrap.sh` script that fetches premake, any necessary plugin dependencies, and generates project files. It only fetches things that are out of date, so don't be afraid to run it often.

You'll want to run this script from within your project's directory. It uses the current working directory to know where to output build files.

```
cd my_game
../harvest-engine/bootstrap.sh
```

By default this will generate project files for the OS you're developing on, if you want to generate for a specific OS you can specify the project file type and OS.

For example, if you're on windows but want to generate the linux makefiles:

```
../harvest-engine/bootstrap.sh gmake2 --os=linux
```

Once done you can find project files in the `my_game/build/` directory. You'll need to rerun the bootstrap file to regenerate projects anytime anyone adds or removes a source code file, plugin, updates the engine, or modifies a premake script. It recommended to run it every time you get latest (git pull, p4 sync, etc).

## Managing Source Control

### Using Git

The recommended way to use git is to separately clone the engine repository and your game's repository and link them via your `premake.lua` file as in the example given in the Setup Your Workspace section.

Be sure to add the `build/` directory to your `.gitignore` file in your project. All generated files are output to this folder and are not intended to be committed.

Use of git lfs is recommended for managing large binary files and providing sparse checkout. That is, only the large binary files the user needs are actually present on their machine.

The Harvest Editor can integrate with git lfs to properly manage large binary assets in the project's assets folder. It will download them only when they are needed, lock them when necessary for editing, etc.

Here is a `.gitattributes` file to get you started:

```
* text=auto eol=lf
*.bat text eol=crlf
# TODO: Binary file types!
```

### Using Perforce/Plastic

TODO.

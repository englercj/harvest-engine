# Harvest Contributing Guide

This document contains a set of guidelines for contributing to Harvest and its various plugins.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [I just have a question!](#i-just-have-a-question)
- [Basics](#basics)
    * [Projects, Plugins, and Modules](#projects-plugins-and-modules)
    * [Design Philosophy](#design-philosophy)
- [How to Contribute](#how-to-contribute)
    * [Reporting Bugs](#reporting-bugs)
    * [Suggesting Enhancements](#suggesting-enhancements)
    * [Getting Support](#getting-support)
    * [Making Code Contributions](#making-code-contributions)
- [Style Guides](#style-guides)
    * [Commit Messages](#commit-messages)
    * [C++ Style Guide](#c++-style-guide)
    * [C# Style Guide](#c#-style-guide)
    * [KDL Style Guide](#kdl-style-guide)
    * [Code Organization](#code-organization)
- [Git Workflow](#git-workflow)
    * [Branches and PRs](#branches-and-prs)
    * [Commit Messages](#commit-messages)
    * [Recommended Configuration](#recommended-configuration)
    * [Useful Aliases](#useful-aliases)

## Code of Conduct

This project and everyone participating in it is governed by the [Harvest Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to adhere to this code. Please report unacceptable behavior to [@englercj](https://github.com/englercj).

## I just have a question!

For questions and support requests please post in the [Discussion Forums](https://github.com/englercj/harvest-engine/discussions).

For bug reports please post an [Issue](https://github.com/englercj/harvest-engine/issues).

## Basics

### Projects, Plugins, and Modules

Harvest is a modular game engine specifically designed to be extensible. Harvest operates on a project, which contains plugins, which contain modules. For details you can check out the [Harvest Project Spec](he_project_kdl_spec.md).

### Design Philosophy

#### Keep it Consistent

The number one rule when developing Harvest is "When in Rome". That is, follow the style and patterns that already exist in any area of the code that you are modifying. Keeping code as consistent as possible is more important that using the "correct" style or pattern. The goal is for the entire engine to feel as if it was written by a single person.

#### Keep it Simple

- Prefer simple types (`const char*`) where able.
- Complexity should be justified by a well-defined use-case and documentation.
- Code should be easy to change.

#### Keep it Explicit

- Explicit is better than implicit.
- Not everyone knows everything, help future readers by being explicit.

#### Keep it Performant

- Performance is important, keep it top of mind when doing anything.
- Performance doesn't need to be #1 goal all the time. Balance this requirement with maintenance costs, future reader comprehension, etc.

#### Keep it Extensible

- All code must be in a module in a plugin.
- Consider how other modules can extend your functionality.
    * For example, `he_core` abstracts platforms; how does another plugin extend platform support?
    * For example, `he_editor` has a lot of UI; how does another plugin extend that UI?

#### Keep it Readable

- Optimize for reading comprehension.

## How to Contribute

### Reporting Bugs

For bug reports please post an [Issue](https://github.com/englercj/harvest-engine/issues) with as much detail as possible. A template is provided to explain the structure of a good bug report.

Your bug may have already been reported, so be sure to search first before posting.

### Suggesting Enhancements

For suggestions and feature requests please open a new discussion in the [Discussion Forums](https://github.com/englercj/harvest-engine/discussions).

Your suggestion or feature requests may have already been posted, so be sure to search first before posting.

### Getting Support

For questions and support requests please open a new discussion in the [Discussion Forums](https://github.com/englercj/harvest-engine/discussions).

Your question may have already been asked, so be sure to search first before posting.

### Making Code Contributions

Code contributions should be made via [Pull Requests](https://github.com/englercj/harvest-engine/pulls). Please be sure to follow the style guides and fill in the PR template with as much detail as you can.

## Style Guides

### Commit Messages

TODO.

### C++ Style Guide

Please review the [C++ Style Guide](styleguides/cpp_style_guide.md) document.

### C# Style Guide

Please review the [C# Style Guide](styleguides/cs_style_guide.md) document.

### KDL Style Guide

Please review the [KDL Style Guide](styleguides/kdl_style_guide.md) document.

### Code Organization

#### Plugins and Modules

- Plugin and module folders should use a short `snake_case` identifier.
- Modules defined in an `he_plugin.kdl` module should use the same name as their folder.
    * Note: Only modules provided by Harvest should use the `he_` prefix in their name.
- All code should be in a module folder within a plugin.

#### When should I use `RegisterApi`?

TODO.

- When should module functionality should be exposed via RegisterApi?
    * When should I include a header and use something vs using RegisterApi?
    * Is it only for dynamic/optional dependencies?
    * Registered apis are singletons.
    * Registered apis are IoC, so plugins can be optionally loaded to provide them.
    * RegisterApi works cross-dll without exporting, includes don't.
- File layout for plugin with one module:
    * `plugin_name/he_plugin.toml`
    * `plugin_name/include/module_name/`
    * `plugin_name/src/`
    * `plugin_name/test/`
- File layout for plugin with multiple module:
    * `plugin_name/he_plugin.toml`
    * `plugin_name/module_name1/include/module_name1/`
    * `plugin_name/module_name1/src/`
    * `plugin_name/module_name1/test/`
    * `plugin_name/module_name2/include/module_name2/`
    * `plugin_name/module_name2/src/`
    * `plugin_name/module_name2/test/`

## Git Workflow

Generally speaking the development of Harvest follows the [releaseflow](http://releaseflow.org/) strategy. This document describes additional rules. For any rules that conflict between this document and the [releaseflow](http://releaseflow.org/) documents, the rules in this document take precedence.

### Branches and PRs

- `main` is the latest development branch
- `release/<major.minor>` is the release branch for a particular major/minor release
- `feature/<name>` is for *short-lived* feature development
    * Once merged into `main` or a release branch, the feature branch should be deleted
- `bugfix/<name>` is for *short-lived* bug fixes
    * Once merged into `main` or a release branch, the bug fix branch should be deleted
- PRs should be merged using the "Squash & Merge" strategy.
- Prefer rebase to merge.
    * Try to keep the commit line as straight as possible, no octopus merging.
    * Default to rebase for `git pull` either by using the recommended config below or specifying `git pull --rebase`

### Commit Messages

- First line is a summary and should be 50 characters or less
    * Do not use markdown in your summary line
- Separate details from the summary with a blank line
    * Details should be wrapped at 80 characters
    * Details can contain markdown formatting
- Content should be descriptive and augment what can be seen in code
    * Don't just mention which files were touched, describe why changes were made

Good guide: https://cbea.ms/git-commit/

Example:

```txt
This is the summary line, it is 50 characters long

This is the detail text. It can go up to eighty characters long before it
should be wrapped. Markdown `formatting` is also reasonable here.

- Bullets and such
- Are totally fine
```

### Recommended Configuration

Here is a configuration file that is recommended when you get started. Remember to replace the `[user]` block with your information:

```ini
[user]
	name = Your Name
	email = your_email@domain.com
[init]
	defaultBranch = main
[pull]
	rebase = true
[fetch]
	prune = true
[color]
	ui = auto
```

### Useful Aliases

Here are some aliases that can be useful when working with command-line git:

```ini
[alias]
	co = checkout
	ls = status
	lol = log --graph --decorate --pretty=oneline --abbrev-commit --date=relative --format=format:'%C(red)%h%C(reset) - %C(green)(%ar)%C(reset) %s - %C(dim cyan)%an%C(reset) %C(auto)%d%C(reset)'
	gc-branches = "!LANG=en_US git branch -vv | awk '/: gone]/{print $1}' | xargs git branch -D"
```

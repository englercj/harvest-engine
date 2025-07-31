# `authors` node

A set of author names and their metadata.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add matched items to the set, or update it if it already exists. This is the default behavior.
    * `remove` - Remove matched the items from the set.
    * `update` - Update properties of matched items if they exist in the set.

## Properties

None.

## Children

- `author-name` - Required. Name of an author.
    * Author name nodes may have an `email` property to specify their email.

## Scopes

- [`plugin`](plugin_node.md)

## Example

```kdl
authors { "Chad Engler" email=englercj@live.com }
```

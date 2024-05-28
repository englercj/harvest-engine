# `authors` node

A set of author names and their metadata.

## Arguments

1. (string) - Optional. How to treat the set of items. Valid values are:
    * `add` - Add the items to the set. This is the default behavior.
    * `remove` - Remove the items from the set.
    * `match` - Do not modify the set of items. Only update properties of matched items.

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

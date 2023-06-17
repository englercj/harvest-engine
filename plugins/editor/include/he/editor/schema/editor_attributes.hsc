// Copyright Chad Engler

@0x83501c02c6723aa5;

import "he/schema/schema.hsc";

namespace he.editor;

// ------------------------------------------------------------------------------------------------
// Display Attributes

// A collection of attributes that modify how fields are treated when on display, most often when
// shown in the editor property grid.
//
// This is a strange place for these to live, but it is a compromise to avoid strange dependencies.
// We need asset types to live in the non-editor assets module because some of the types defined in
// here are used at runtime. Specifically, there are asset types that are just passed through as a
// runtime resource. Because of that, this module cannot have any dependencies on the editor.
// Hence, the strange relationship of defining these here but implementing code that cares about them
// in editor modules.
struct Display
{
    // hides an asset type from the list of types that the editor can create directly.
    attribute ImportOnly(struct) :void;

    // Hides the field from display. The data is still persisted and available to code.
    // The editor property grids will skip fields with this attribute.
    attribute Hidden(field) :void;

    // Shows the field, but does not allow it to be edited. The data can still be changed by code.
    attribute ReadOnly(field) :void;

    // Human-friendly display name for the target.
    // The editor uses this value as an override for the name during display.
    attribute Name(enum, enumerator, field, struct) :String;

    // Human-friendly description text for the target.
    // The editor will use this value as a tooltip.
    attribute Description(enum, enumerator, field, struct) :String;

    // Allows for multi-line editing of a String field.
    attribute Multiline(field) :void;

    // Promotes fields of the type to be displayed as if they are fields of the parent.
    attribute Promote(field) :void;

    // Displays a slider edit control for this field.
    attribute Slider(field) :ScalarRange;

    // Sets a minimum and maximum value that can be selected in the UI for an integer value.
    attribute Clamp(field) :ScalarRange;
}

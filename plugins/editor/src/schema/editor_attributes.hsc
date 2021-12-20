// Copyright Chad Engler

@0xbcd5ee1c74ae4bdb;

namespace he.editor.schema;

// Hides the field from editor views so that it cannot be changed by the user. The data is
// still persisted and available to code.
attribute EditorHidden(*) :void;

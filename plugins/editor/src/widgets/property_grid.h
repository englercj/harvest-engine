// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/schema/dynamic.h"

#include "imgui.h"

namespace he::editor
{
    class TypeEditUIService;
    struct AssetEdit;

    // --------------------------------------------------------------------------------------------

    /// Begins a property grid block.
    ///
    /// \return True if the grid is being rendered.
    bool BeginPropertyGrid(uint32_t id);

    /// Ends a property grid block.
    ///
    /// \note Only call if \ref BeginPropertyGrid returns true.
    void EndPropertyGrid();

    /// Begins the property grid header block.
    ///
    /// \return True if the header is begin rendered.
    bool BeginPropertyGridHeader(String* searchText = nullptr);

    /// Ends the property grid header block.
    ///
    /// \note Only call if \ref BeginPropertyGridHeader returns true.
    void EndPropertyGridHeader();

    /// Pushes a property grid section onto the stack.
    ///
    /// \param[in] label The label for this section.
    /// \return True if the section is open and rendered.
    bool BeginPropertyGridSection(const char* label);

    /// Pops the last property grid section off the stack.
    ///
    /// \note Only call if \ref BeginPropertyGridSection returns true.
    void EndPropertyGridSection();

    /// Pushes a property grid table onto the stack.
    ///
    /// \return True if the row is begin rendered.
    bool BeginPropertyGridTable();

    /// Pops the last property grid table off the stack.
    ///
    /// \note Only call if \ref BeginPropertyGridTable returns true.
    void EndPropertyGridTable();

    /// Pushes a property grid row onto the stack.
    ///
    /// \return True if the row is begin rendered.
    bool BeginPropertyGridRow();

    /// Pops the last property grid row off the stack.
    ///
    /// \note Only call if \ref BeginPropertyGridRow returns true.
    void EndPropertyGridRow();

    // --------------------------------------------------------------------------------------------

    /// Simple interface for creating a property grid for a single structure.
    ///
    /// \param[in] data The structure to build a property grid for.
    /// \param[in] declInfo The decaration info that describes the structure in `data`.
    /// \param[out] edit The edit to adds changes into.
    void PropertyGrid(const schema::DynamicStruct::Reader& data, TypeEditUIService& service, AssetEdit& edit);
}

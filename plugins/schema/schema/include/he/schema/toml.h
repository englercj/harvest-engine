// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/schema/schema.h"

namespace he { class String; }

namespace he::schema
{
    /// Serializes a schema buffer structure into a TOML string.
    ///
    /// \param[in] dst The destination string to write to.
    /// \param[in] data The struct reader to read data from.
    /// \param[in] info The declaration info for the struct's schema.
    void ToToml(he::String& dst, StructReader data, const DeclInfo& info);

    /// Serializes a schema buffer structure into a TOML string.
    ///
    /// \tparam The type of schema structure to serialize.
    /// \param[in] dst The destination string to write to.
    /// \param[in] data The reader to read data from.
    template <typename T>
    void ToToml(he::String& dst, typename T::Reader data) { ToToml(dst, data, T::DeclInfo); }

    /// Deserializes a schema buffer structure from a TOML string.
    ///
    /// \param[in] dst The destination buffer to write the structure to.
    /// \param[in] data The TOML string to read from.
    /// \param[in] info The declaration info for the struct's schema.
    bool FromToml(Builder& dst, StringView data, const DeclInfo& info);

    /// Deserializes a schema buffer structure from a TOML string.
    ///
    /// \tparam The type of schema structure to deserialize.
    /// \param[in] dst The destination buffer to write the structure to.
    /// \param[in] data The TOML string to read from.
    template <typename T>
    bool FromToml(Builder& dst, StringView data) { return FromToml(dst, data, T::DeclInfo); }

    /// Deserializes a schema buffer structure from a TOML string.
    ///
    /// \tparam The type of schema structure to deserialize.
    /// \param[in] dst The destination buffer to write the structure to.
    /// \param[in] data The TOML string to read from.
    template <typename T>
    bool FromToml(TypedBuilder<T>& dst, StringView data) { return FromToml<T>(dst.builder, data); }
}

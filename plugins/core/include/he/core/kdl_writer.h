// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/string_writer.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    class KdlValue;

    /// Formats for writing unsigned integral numbers in KDL.
    enum class KdlIntFormat : uint8_t
    {
        Decimal,
        Hex,
        Octal,
        Binary,
    };

    /// Formats for writing floating-point numbers in KDL.
    enum class KdlFloatFormat : uint8_t
    {
        Default,
        General,
        Fixed,
        Exponent,
    };

    /// A writer for the KDL format.
    class KdlWriter
    {
    public:
        /// Constructs a KDL writer with the given destination string.
        explicit KdlWriter(String& dst) noexcept
            : m_writer(dst)
        {}

        /// Clears the destination string and resets the writer.
        void Clear();

        /// Reserves space in the destination string.
        void Reserve(uint32_t size) { m_writer.Reserve(size); }

    public:
        /// Writes a single-line comment.
        ///
        /// \param[in] value The comment to write.
        void Comment(StringView value);

        /// Writes the start of a multi-line comment.
        void StartComment();

        /// Writes the end of a multi-line comment.
        void EndComment();

        /// Writes a node tag with an optional type annotation.
        ///
        /// \param[in] name The name of the node.
        /// \param[in] type Optional. The type annotation for the node.
        void Node(StringView name, const StringView* type = nullptr);

        /// Writes a node tag with a type annotation.
        ///
        /// \param[in] name The name of the node.
        /// \param[in] type The type annotation for the node.
        void Node(StringView name, StringView type) { Node(name, &type); }

        /// Writes the start of a node block (`node {`).
        void StartNodeChildren();

        /// Writes the end of a node block (`}`).
        void EndNodeChildren();

        /// Writes an integral argument value with an optional type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        /// \param[in] format Optional. The integer format to use when writing the value.
        template <Integral T> requires(!IsSame<T, bool>)
        void Argument(T value, const StringView* type = nullptr, KdlIntFormat format = KdlIntFormat::Decimal);

        /// Writes an integral argument value with a type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        /// \param[in] format Optional. The integer format to use when writing the value.
        template <Integral T> requires(!IsSame<T, bool>)
        void Argument(T value, StringView type, KdlIntFormat format = KdlIntFormat::Decimal) { Argument<T>(value, &type, format); }

        /// Writes a boolean argument value with an optional type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        void Argument(bool value, const StringView* type = nullptr);

        /// Writes a boolean argument value with a type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        void Argument(bool value, StringView type) { Argument(value, &type); }

        /// Writes a floating-point argument value with an optional type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] precision Optional. The number of decimal places to write.
        void Argument(double value, const StringView* type = nullptr, KdlFloatFormat format = KdlFloatFormat::Default, int32_t precision = -1);

        /// Writes a floating-point argument value with a type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] precision Optional. The number of decimal places to write.
        void Argument(double value, StringView type, KdlFloatFormat format = KdlFloatFormat::Default, int32_t precision = -1) { Argument(value, &type, format, precision); }

        /// Writes a string argument value with an optional type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Argument(StringView value, const StringView* type = nullptr, bool multiline = false, uint32_t rawDelimiterCount = 0);

        /// Writes a string argument value with a type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Argument(StringView value, StringView type, bool multiline = false, uint32_t rawDelimiterCount = 0) { Argument(value, &type, multiline, rawDelimiterCount); }

        /// Writes a string argument value with an optional type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Argument(const char* value, const StringView* type = nullptr, bool multiline = false, uint32_t rawDelimiterCount = 0);

        /// Writes a string argument value with a type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Argument(const char* value, StringView type, bool multiline = false, uint32_t rawDelimiterCount = 0) { Argument(value, &type, multiline, rawDelimiterCount); }

        /// Writes a null argument value with an optional type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        void Argument(nullptr_t value, const StringView* type = nullptr);

        /// Writes a null argument value with a type annotation.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type The type annotation for the argument value.
        void Argument(nullptr_t value, StringView type) { Argument(value, &type); }

        /// Writes a property name and integral value with an optional type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        template <Integral T> requires(!IsSame<T, bool>)
        void Property(StringView name, T value, const StringView* type = nullptr, KdlIntFormat format = KdlIntFormat::Decimal);

        /// Writes a property name and integral value with a type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        template <Integral T> requires(!IsSame<T, bool>)
        void Property(StringView name, T value, StringView type, KdlIntFormat format = KdlIntFormat::Decimal) { Property<T>(name, value, &type, format); }

        /// Writes a property name and boolean value with an optional type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        void Property(StringView name, bool value, const StringView* type = nullptr);

        /// Writes a property name and boolean value with a type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type The type annotation for the property value.
        void Property(StringView name, bool value, StringView type) { Property(name, value, &type); }

        /// Writes a property name and floating-point value with an optional type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] precision Optional. The number of decimal places to write.
        void Property(StringView name, double value, const StringView* type = nullptr, KdlFloatFormat format = KdlFloatFormat::Default, int32_t precision = -1);

        /// Writes a property name and floating-point value with a type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] precision Optional. The number of decimal places to write.
        void Property(StringView name, double value, StringView type, KdlFloatFormat format = KdlFloatFormat::Default, int32_t precision = -1) { Property(name, value, &type, format, precision); }

        /// Writes a property name and string value with an optional type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Property(StringView name, StringView value, const StringView* type = nullptr, bool multiline = false, uint32_t rawDelimiterCount = 0);

        /// Writes a property name and string value with a type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type The type annotation for the property value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Property(StringView name, StringView value, StringView type, bool multiline = false, uint32_t rawDelimiterCount = 0) { Property(name, value, &type, multiline, rawDelimiterCount); }

        /// Writes a property name and string value with an optional type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Property(StringView name, const char* value, const StringView* type = nullptr, bool multiline = false, uint32_t rawDelimiterCount = 0);

        /// Writes a property name and string value with a type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type The type annotation for the property value.
        /// \param[in] multiline Optional. True to write the value as a multi-line string.
        /// \param[in] rawDelimiterCount Optional. When zero (default) the value is written as an
        ///     escaped string. When greater than zero, the value is written as a raw string using
        ///     this many '#' characters as delimiters.
        void Property(StringView name, const char* value, StringView type, bool multiline = false, uint32_t rawDelimiterCount = 0) { Property(name, value, &type, multiline, rawDelimiterCount); }

        /// Writes a property name and null value with an optional type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        void Property(StringView name, nullptr_t value, const StringView* type = nullptr);

        /// Writes a property name and null value with a type annotation.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type The type annotation for the property value.
        void Property(StringView name, nullptr_t value, StringView type) { Property(name, value, &type); }

    private:
        template <typename T, typename... Args>
        void WriteArgOrProp(const StringView* name, T value, const StringView* type, Args&&... args);

    private:
        StringWriter m_writer;
        uint32_t m_nodeDepth{ 0 };
        bool m_startOfLine{ true };
        bool m_inNode{ false };
    };
}

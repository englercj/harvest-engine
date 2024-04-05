// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/string_writer.h"
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
        General,
        Fixed,
        Exponent,
    };

    /// Formats for writing strings in KDL.
    enum class KdlStringFormat : uint8_t
    {
        Escaped,
        Raw,
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

        /// Writes a node tag, with an optional type.
        ///
        /// \param[in] name The name of the node.
        /// \param[in] type Optional. The type annotation for the node.
        void Node(StringView name, StringView type = {});

        /// Writes the start of a node block (`node {`).
        void StartNodeChildren();

        /// Writes the end of a node block (`}`).
        void EndNodeChildren();

        /// Writes an argument value to a node.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        void Argument(bool value, StringView type = {});

        /// Writes an argument value to a node.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        void Argument(signed char value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntArg(value, type, format); }

        /// \copydoc Argument(signed char, StringView)
        void Argument(short value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntArg(value, type, format); }

        /// \copydoc Argument(signed char, StringView)
        void Argument(int value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntArg(value, type, format); }

        /// \copydoc Argument(signed char, StringView)
        void Argument(long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntArg(value, type, format); }

        /// \copydoc Argument(signed char, StringView)
        void Argument(long long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntArg(value, type, format); }

        /// Writes an argument value to a node.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        /// \param[in] format Optional. The format to use when writing the value.
        void Argument(unsigned char value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintArg(value, type, format); }

        /// \copydoc Argument(unsigned char, StringView, KdlIntFormat)
        void Argument(unsigned short value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintArg(value, type, format); }

        /// \copydoc Argument(unsigned char, StringView, KdlIntFormat)
        void Argument(unsigned int value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintArg(value, type, format); }

        /// \copydoc Argument(unsigned char, StringView, KdlIntFormat)
        void Argument(unsigned long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintArg(value, type, format); }

        /// \copydoc Argument(unsigned char, StringView, KdlIntFormat)
        void Argument(unsigned long long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintArg(value, type, format); }

        /// Writes an argument value to a node.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] precision Optional. The number of decimal places to write.
        void Argument(double value, StringView type = {}, KdlFloatFormat format = KdlFloatFormat::General, int32_t precision = -1);

        /// Writes an argument value to a node.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] rawDelimiterCount Optional. The number of '#' characters to use as
        ///     delimiters when using the \ref KdlStringFormat::Raw format.
        void Argument(StringView value, StringView type = {}, KdlStringFormat format = KdlStringFormat::Escaped, uint32_t rawDelimiterCount = 1);

        /// Writes an argument value to a node.
        ///
        /// \param[in] value The argument value.
        /// \param[in] type Optional. The type annotation for the argument value.
        void Argument(nullptr_t, StringView type = {});

        /// Writes a property name and value to a node.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        void Property(StringView name, bool value, StringView type = {});

        /// Writes a property name and value to a node.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        void Property(StringView name, signed char value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntProp(name, value, type, format); }

        /// \copydoc Property(StringView, signed char, StringView)
        void Property(StringView name, short value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntProp(name, value, type, format); }

        /// \copydoc Property(StringView, signed char, StringView)
        void Property(StringView name, int value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntProp(name, value, type, format); }

        /// \copydoc Property(StringView, signed char, StringView)
        void Property(StringView name, long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntProp(name, value, type, format); }

        /// \copydoc Property(StringView, signed char, StringView)
        void Property(StringView name, long long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return IntProp(name, value, type, format); }

        /// Writes a property name and value to a node.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        void Property(StringView name, unsigned char value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintProp(name, value, type, format); }

        /// \copydoc Property(StringView, unsigned char, StringView, KdlIntFormat)
        void Property(StringView name, unsigned short value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintProp(name, value, type, format); }

        /// \copydoc Property(StringView, unsigned char, StringView, KdlIntFormat)
        void Property(StringView name, unsigned int value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintProp(name, value, type, format); }

        /// \copydoc Property(StringView, unsigned char, StringView, KdlIntFormat)
        void Property(StringView name, unsigned long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintProp(name, value, type, format); }

        /// \copydoc Property(StringView, unsigned char, StringView, KdlIntFormat)
        void Property(StringView name, unsigned long long value, StringView type = {}, KdlIntFormat format = KdlIntFormat::Decimal) { return UintProp(name, value, type, format); }

        /// Writes a property name and value to a node.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] precision Optional. The number of decimal places to write.
        void Property(StringView name, double value, StringView type = {}, KdlFloatFormat format = KdlFloatFormat::General, int32_t precision = -1);

        /// Writes a property name and value to a node.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        /// \param[in] format Optional. The format to use when writing the value.
        /// \param[in] rawDelimiterCount Optional. The number of '#' characters to use as
        ///     delimiters when using the \ref KdlStringFormat::Raw format.
        void Property(StringView name, StringView value, StringView type = {}, KdlStringFormat format = KdlStringFormat::Escaped, uint32_t rawDelimiterCount = 1);

        /// Writes a property name and value to a node.
        ///
        /// \param[in] name The name of the property.
        /// \param[in] value The value of the property.
        /// \param[in] type Optional. The type annotation for the property value.
        void Property(StringView name, nullptr_t, StringView type = {});

    private:
        void IntArg(long long value, StringView type, KdlIntFormat format);
        void UintArg(unsigned long long value, StringView type, KdlIntFormat format);

        void IntProp(StringView name, long long value, StringView type, KdlIntFormat format);
        void UintProp(StringView name, unsigned long long value, StringView type, KdlIntFormat format);

    private:
        StringWriter m_writer;
        uint32_t m_nodeDepth{ 0 };
        bool m_startOfLine{ true };
        bool m_inNode{ false };
    };
}

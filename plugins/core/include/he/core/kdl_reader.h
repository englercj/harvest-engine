// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    /// An error that can occur while reading a KDL document.
    enum class KdlReadError : uint8_t
    {
        None,                   ///< No error.
        Cancelled,              ///< The read was cancelled by the handler.
        UnexpectedEof,          ///< The input ended unexpectedly.
        DisallowedUtf8,         ///< The utf-8 sequence is disallowed. For example, various control sequences.
        InvalidBom,             ///< The utf-8 Byte Order Mark (BOM) of the file is not valid.
        InvalidUtf8,            ///< An invalid utf-8 sequence was encountered.
        InvalidEscapeSequence,  ///< An escape sequence (`\n`, `\t`, `\"`, etc) was invalid.
        InvalidControlChar,     ///< A control character was encountered somewhere it wasn't expected (like in a string).
        InvalidIdentifier,      ///< An identifier string contains invalid characters.
        InvalidNumber,          ///< Format of a number is invalid. For example, a negative hex number.
        InvalidToken,           ///< Encountered an unexpected token in the file.
        InvalidDocument,        ///< The document is semantically invalid.
    };

    /// The result of reading a KDL document.
    struct KdlReadResult
    {
        /// The error that occurred while reading the document.
        KdlReadError error = KdlReadError::None;

        /// The line where the error occurred.
        uint32_t line{ 0 };

        /// The column where the error occurred.
        uint32_t column{ 0 };

        /// When error is \ref KdlReadError::InvalidToken, this was the character that was
        /// expected instead. Otherwise this value is just '\0'.
        char expected{ '\0' };

        /// Checks if the read was successful.
        ///
        /// \return True if the read was successful, false otherwise.
        [[nodiscard]] bool IsValid() const { return error == KdlReadError::None; }

        /// Checks if the read was successful.
        ///
        /// \return True if the read was successful, false otherwise.
        [[nodiscard]] explicit operator bool() const { return error == KdlReadError::None; }
    };

    /// A reader for the KDL file format.
    class KdlReader
    {
    public:
        /// Constructs a KDL reader with the given allocator.
        ///
        /// \param[in] allocator The allocator to use for memory allocation.
        explicit KdlReader(Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// The handler interface for the KDL reader.
        /// This class defines the interface of functions that will be invoked by the KDL reader
        /// as it reads the document.
        class Handler
        {
        public:
            /// Destructor.
            virtual ~Handler() = default;

            /// Called at the start of reading the document.
            ///
            /// \return True to continue reading, false to stop.
            virtual bool StartDocument() = 0;

            /// Called at the end of reading the document.
            ///
            /// \return True to continue reading, false to stop.
            virtual bool EndDocument() = 0;

            /// Called when a single-line (`//`) or multi-line (`/**/`) comment is encountered.
            ///
            /// \param[in] value The comment value.
            /// \return True to continue reading, false to stop.
            virtual bool Comment(StringView value) = 0;

            /// Called when a slashdash comment starts. This may be followed by any number of
            /// calls to \ref StartNode, \ref EndNode, \ref Argument, or \ref Property which
            /// should all be treated as "commented out" until \ref EndComment is called.
            ///
            /// \return True to continue reading, false to stop.
            virtual bool StartComment() = 0;

            /// Called when the components of a slashdash comment have been read.
            ///
            /// \return True to continue reading, false to stop.
            virtual bool EndComment() = 0;

            /// Called when a node is encountered. Additional calls to \ref StartNode before
            /// \ref EndNode are children of this node.
            ///
            /// \param[in] name The name of the node.
            /// \param[in] type The type annotation, if any, on the node.
            /// \return True to continue reading, false to stop.
            virtual bool StartNode(StringView name, StringView type) = 0;

            /// Called when the end of a node is encountered.
            ///
            /// \return True to continue reading, false to stop.
            virtual bool EndNode() = 0;

            /// Called when an argument to a node is encountered.
            ///
            /// \param[in] value The argument value.
            /// \param[in] type The type annotation, if any, on the argument value.
            /// \return True to continue reading, false to stop.
            virtual bool Argument(bool value, StringView type) = 0;

            /// \copydoc Argument(bool)
            virtual bool Argument(int64_t value, StringView type) = 0;

            /// \copydoc Argument(bool)
            virtual bool Argument(uint64_t value, StringView type) = 0;

            /// \copydoc Argument(bool)
            virtual bool Argument(double value, StringView type) = 0;

            /// \copydoc Argument(bool)
            virtual bool Argument(StringView value, StringView type) = 0;

            /// \copydoc Argument(bool)
            virtual bool Argument(nullptr_t, StringView type) = 0;

            /// Called when a property is encountered.
            ///
            /// \param[in] name The name of the property.
            /// \param[in] value The value of the property.
            /// \param[in] type The type annotation, if any, on the property value.
            virtual bool Property(StringView name, bool value, StringView type) = 0;

            /// \copydoc Property(StringView, bool)
            virtual bool Property(StringView name, int64_t value, StringView type) = 0;

            /// \copydoc Property(StringView, bool)
            virtual bool Property(StringView name, uint64_t value, StringView type) = 0;

            /// \copydoc Property(StringView, bool)
            virtual bool Property(StringView name, double value, StringView type) = 0;

            /// \copydoc Property(StringView, bool)
            virtual bool Property(StringView name, StringView value, StringView type) = 0;

            /// \copydoc Property(StringView, bool)
            virtual bool Property(StringView name, nullptr_t value, StringView type) = 0;
        };

    public:
        /// Reads a KDL document from the given data and calls the handler for each element
        /// encountered during parsing.
        ///
        /// \param[in] data The KDL document string data.
        /// \param[in] handler The handler to call for each element.
        /// \return The result of the read.
        KdlReadResult Read(StringView data, Handler& handler);

    private:
        Allocator& m_allocator;
    };
}

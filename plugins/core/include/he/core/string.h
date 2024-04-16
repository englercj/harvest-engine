// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/string_ops.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace he
{
    class String final
    {
    public:
        // ----------------------------------------------------------------------------------------
        // Constants

        /// The type of elements in the string.
        using ElementType = char;

        /// The maximum number of characters that can be stored inline in the object.
        static constexpr uint32_t MaxEmbedCharacters = 31;

        /// The maximum number of characters that can be stored on the heap.
        static constexpr uint32_t MaxHeapCharacters = 0x7ffffffe;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty string.
        ///
        /// \param allocator Optional. The allocator to use.
        explicit String(Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct a string by copying from the null terminated string `str`.
        ///
        /// \param str The string to copy from.
        /// \param allocator Optional. The allocator to use.
        String(const char* str, Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct a string by copying `len` characters from the string `str`.
        /// This does not stop early if it encounters a null terminator before `len` characters.
        ///
        /// \param str The string to copy from.
        /// \param len The number of characters to copy.
        /// \param allocator Optional. The allocator to use.
        String(const char* str, uint32_t len, Allocator& allocator = Allocator::GetDefault()) noexcept;

        /// Construct a string from an object that provides a Harvest-style contiguous range of
        /// characters. That is, it has `.Data()` and `.Size()` members.
        ///
        /// \param range The object that provides the range.
        /// \param allocator Optional. The allocator to use.
        template <ContiguousRangeOf<const char> R> requires(!IsSame<R, String>)
        String(const R& range, Allocator& allocator = Allocator::GetDefault()) noexcept
            : String(range.Data(), range.Size(), allocator)
        {}

        /// Construct a string by copying `x`, and using `allocator` for this string's allocations.
        ///
        /// \param x The string to copy from.
        /// \param allocator The allocator to use for any allocations.
        String(const String& x, Allocator& allocator) noexcept;

        /// Construct a string by moving `x`, and using `allocator` for this string's allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The string to move from.
        /// \param allocator The allocator to use for any allocations.
        String(String&& x, Allocator& allocator) noexcept;

        /// Construct a string by copying `x`, using the allocator from `x`.
        ///
        /// \param x The string to copy from.
        String(const String& x) noexcept;

        /// Construct a string by moving `x`, using the allocator from `x`.
        ///
        /// \param x The string to move from.
        String(String&& x) noexcept;

        /// Destructs the string, freeing any memory allocations.
        ~String() noexcept;

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the string `x` into this string.
        ///
        /// \param x The string to copy from.
        String& operator=(const String& x) noexcept;

        /// Move the string `x` into this string.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The string to move from.
        String& operator=(String&& x) noexcept;

        /// Replaces the contents of this string with a copy of the null terminated `str`.
        ///
        /// \param str The string source to copy from.
        String& operator=(const char* str) noexcept { Assign(str); return *this; }

        /// Replaces the contents of this string with a copy of the characters in `range`.
        ///
        /// \param str The string source to copy from.
        template <ContiguousRangeOf<const char> R> requires(!IsSame<R, String>)
        String& operator=(const R& range) noexcept { Assign(range.Data(), range.Size()); return *this; }

        /// Gets a reference to the character at `index`. Asserts if `index` is not less
        /// than \see Size().
        ///
        /// \param index The index of the character to return.
        /// \return A reference to the character at `index`.
        [[nodiscard]] const char& operator[](uint32_t index) const;

        /// \copydoc operator[](uint32_t)
        [[nodiscard]] char& operator[](uint32_t index) { return const_cast<char&>(const_cast<const String&>(*this)[index]); }

        /// Appends the null terminated string to the end of this string.
        ///
        /// \param str The string to append.
        String& operator+=(const char* str) { Insert(Size(), str, StrLen(str)); return *this; }

        /// Appends the character to the end of this string.
        ///
        /// \param c The character to append.
        String& operator+=(char c) { Insert(Size(), &c, 1); return *this; }

        /// Appends a series of characters from an object that provides a Harvest-style
        /// contiguous range. That is, it has `.Data()` and `.Size()` members.
        ///
        /// \param range The object that provides the range.
        template <ContiguousRangeOf<const char> R>
        String& operator+=(const R& range) { Insert(Size(), range.Data(), range.Size()); return *this; }

        /// Creates a new string that is the concatenation of this string and the null terminated
        /// string parameter.
        ///
        /// \param str The string to concatenate.
        String operator+(const char* str) const { String copy(*this); copy += str; return copy; }

        /// Creates a new string that is the concatenation of this string and the character
        /// parameter.
        ///
        /// \param c The character to concatenate.
        String operator+(char c) const { String copy(*this); copy += c; return copy; }

        /// Creates a new string that is the concatenation of this string and a series of
        /// characters from an object that provides a Harvest-style contiguous range.
        /// That is, it has `.Data()` and `.Size()` members.
        ///
        /// \param range The object that provides the range.
        template <ContiguousRangeOf<const char> R>
        String operator+(const R& range) const { String copy(*this); copy += range; return copy; }

        /// Checks if this string is equal to the null terminated string `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are equal, false otherwise.
        [[nodiscard]] bool operator==(const char* x) const { return CompareTo(x) == 0; }

        /// Checks if this string is equal to a character range.
        ///
        /// \param range The characters to check against.
        /// \return True if the strings are equal, false otherwise.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] bool operator==(const R& range) const { return range.Size() == Size() && CompareTo(range) == 0; }

        /// Checks if this string is less than the null terminated string `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than `x`, false otherwise.
        [[nodiscard]] bool operator<(const char* x) const { return CompareTo(x) < 0; }

        /// Checks if this string is less than a character range.
        ///
        /// \param range The characters to check against.
        /// \return True if the string is less than `range`, false otherwise.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] bool operator<(const R& range) const { return CompareTo(range) < 0; }

        /// Checks if this string is less than or equal to the null terminated string `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than or equal to `x`, false otherwise.
        [[nodiscard]] bool operator<=(const char* x) const { return CompareTo(x) <= 0; }

        /// Checks if this string is less than or equal to a character range.
        ///
        /// \param range The characters to check against.
        /// \return True if the string is less than or equal to `range`, false otherwise.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] bool operator<=(const R& range) const { return CompareTo(range) <= 0; }

        /// Checks if this string is greater than the null terminated string `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than `x`, false otherwise.
        [[nodiscard]] bool operator>(const char* x) const { return CompareTo(x) > 0; }

        /// Checks if this string is greater than a character range.
        ///
        /// \param range The characters to check against.
        /// \return True if the string is greater than `range`, false otherwise.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] bool operator>(const R& range) const { return CompareTo(range) > 0; }

        /// Checks if this string is greater than or equal to the null terminated string `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than or equal to `x`, false otherwise.
        [[nodiscard]] bool operator>=(const char* x) const { return CompareTo(x) >= 0; }

        /// Checks if this string is greater than or equal to a character range.
        ///
        /// \param range The characters to check against.
        /// \return True if the string is greater than or equal to `range`, false otherwise.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] bool operator>=(const R& range) const { return CompareTo(range) >= 0; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this string is using embedded storage for the string data.
        ///
        /// \return Returns true if no heap allocation is being used.
        [[nodiscard]] bool IsEmbedded() const { return m_embed[EmbedSize - 1] != HeapFlag; }

        /// Checks if this string is empty.
        ///
        /// \return Returns true if this is an empty string.
        [[nodiscard]] bool IsEmpty() const { return Size() == 0; }

        /// The capacity the string has for characters.
        ///
        /// Note: This is not the size of the allocation, but rather the number of total
        /// characters the string can hold before having to reallocate.
        ///
        /// \return Number of total chracters this string can store.
        [[nodiscard]] uint32_t Capacity() const { return (IsEmbedded() ? EmbedSize : m_heap.capacity) - 1; }

        /// The length of the string that is currently stored.
        ///
        /// \return Number of characters in the string, not including the null terminator.
        [[nodiscard]] uint32_t Size() const { return IsEmbedded() ? GetSizeEmbed() : m_heap.size; }

        /// Reserves capacity for `len` characters. \see Capacity() is guaranteed to return
        /// at least `len` after this operation.
        ///
        /// \param len The length of characters to reserve capacity for.
        void Reserve(uint32_t len);

        /// Resizes the string to be `len` characters long. If this size is longer than the
        /// current length then the new characters are default initialized.
        ///
        /// \param len The length to make the string.
        void Resize(uint32_t len, DefaultInitTag);

        /// Resizes the string to be `len` characters long. If this size is longer than the
        /// current length then `c` is used to fill in the new characters.
        ///
        /// \param len The length to make the string.
        /// \param c The character to use to copy to new entries in the string.
        void Resize(uint32_t len, char c = '\0');

        /// Resizes the string to be `len` characters longer. The new characters are default
        /// initialized.
        ///
        /// \note This is different than `Resize(Size() + len)` in that it will use normal growth
        /// rules to expand the storage. There will likely be slack in the capacity after calling
        /// `Expand(len)`, a property that `Resize(Size() + len)` does not have.
        ///
        /// \param len The number of characters to expand the string size by.
        void Expand(uint32_t len, DefaultInitTag);

        /// Resizes the string to be `len` characters longer. The new characters are set to `c`.
        ///
        /// \note This is different than `Resize(Size() + len)` in that it will use normal growth
        /// rules to expand the storage. There will likely be slack in the capacity after calling
        /// `Expand(len)`, a property that `Resize(Size() + len)` does not have.
        ///
        /// \param len The number of characters to expand the string size by.
        /// \param c The character to use to copy to new entries in the string.
        void Expand(uint32_t len, char c = '\0');

        /// Shrinks the memory allocation to fit the current size of the string. If the string
        /// is allocated on the heap and shrinking would let it fit into the embedded data it
        /// will copy the string into the embedded data and deallocate the heap string.
        void ShrinkToFit();

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the string's character buffer.
        ///
        /// \return A pointer to the character buffer.
        [[nodiscard]] const char* Data() const { return IsEmbedded() ? m_embed : m_heap.data; }

        /// \copydoc Data()
        [[nodiscard]] char* Data() { return IsEmbedded() ? m_embed : m_heap.data; }

        /// Gets a reference to the string's first character. The string must not be empty.
        ///
        /// \return A reference to the first character.
        [[nodiscard]] const char& Front() const;

        /// \copydoc Front()
        [[nodiscard]] char& Front() { return const_cast<char&>(const_cast<const String*>(this)->Front()); }

        /// Gets a reference to the string's last character. The string must not be empty.
        ///
        /// \return A reference to the last character.
        [[nodiscard]] const char& Back() const;

        /// \copydoc Back()
        [[nodiscard]] char& Back() { return const_cast<char&>(const_cast<const String*>(this)->Back()); }

        /// Returns a reference to the allocator object used by the string.
        ///
        /// \return The allocator object this string uses.
        [[nodiscard]] Allocator& GetAllocator() const { return m_allocator; }

        /// Returns a non-cryptographic hash of the string contents.
        ///
        /// \return The hash value.
        [[nodiscard]] uint64_t HashCode() const noexcept;

        /// Create a new string with a copy of a character range from this string.
        /// The range starts at `offset` and continues to the end of the string.
        ///
        /// \note Asserts if `offset` is greater than, or equal to, \ref Size().
        ///
        /// \param[in] offset The offset to start the new string at.
        /// \param[in] allocator The allocator to use for the new string.
        /// \return The requested substring.
        [[nodiscard]] String Substring(uint32_t offset, Allocator& allocator) const;

        /// Create a new string with a copy of a character range from this string.
        /// The range starts at `offset` and continues to the end of the string. The newly
        /// created string will use the same allocator as this string.
        ///
        /// \note Asserts if `offset` is greater than, or equal to, \ref Size().
        ///
        /// \param[in] offset The offset to start the new string at.
        /// \return The requested substring.
        [[nodiscard]] String Substring(uint32_t offset) const { return Substring(offset, m_allocator); }

        /// Create a new string with a copy of a character range from this string.
        /// The range starts at `offset` and continues to the end of the string.
        ///
        /// \note Asserts if `offset` is greater than, or equal to, \ref Size() or if `count` is
        /// greater than `Size() - offset`.
        ///
        /// \param[in] offset The offset to start the new string at.
        /// \param[in] count The number of characters to include in the new string.
        /// \param[in] allocator The allocator to use for the new string.
        /// \return The requested substring.
        [[nodiscard]] String Substring(uint32_t offset, uint32_t count, Allocator& allocator) const;

        /// Create a new string with a copy of a character range from this string.
        /// The range starts at `offset` and continues to the end of the string. The newly
        /// created string will use the same allocator as this string.
        ///
        /// \note Asserts if `offset` is greater than, or equal to, \ref Size() or if `count` is
        /// greater than `Size() - offset`.
        ///
        /// \param[in] offset The offset to start the new string at.
        /// \param[in] count The number of characters to include in the new string.
        /// \return The requested substring.
        [[nodiscard]] String Substring(uint32_t offset, uint32_t count) const { return Substring(offset, count, m_allocator); }

        /// Searches this string for a character.
        ///
        /// \param[in] search The character to search for.
        /// \return A pointer to the found character, or nullptr if not found.
        [[nodiscard]] const char* Find(char search) const { return StrFindN(Data(), Size(), search); }

        /// Searches this string for a null-terminated string.
        ///
        /// \param[in] search The null-terminated string to search for.
        /// \return A pointer to the start of the found string, or nullptr if not found.
        [[nodiscard]] const char* Find(const char* search) const { return StrFindN(Data(), Size(), search); }

        /// Searches this string for a substring.
        ///
        /// \param[in] search The string to search for.
        /// \param[in] len The number of characters in the `search` string.
        /// \return A pointer to the start of the found string, or nullptr if not found.
        [[nodiscard]] const char* Find(const char* search, uint32_t len) const { return StrFindN(Data(), Size(), search, len); }

        /// Searches this string for a range of characters.
        ///
        /// \param[in] search The range of characters to search for.
        /// \return A pointer to the start of the found string, or nullptr if not found.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] const char* Find(const R& search) const { return StrFindN(Data(), Size(), search.Data(), search.Size()); }

        /// Searches this string for a character.
        ///
        /// \param[in] search The character to search for.
        /// \return The index of the found character, or `uint32_t(-1)` if not found.
        [[nodiscard]] uint32_t FindIndex(char search) const { const char* p = Find(search); return p ? static_cast<uint32_t>(p - Data()) : static_cast<uint32_t>(-1); }

        /// Searches this string for a null-terminated string.
        ///
        /// \param[in] search The null-terminated string to search for.
        /// \return The index of the start of the found string, or `uint32_t(-1)` if not found.
        [[nodiscard]] uint32_t FindIndex(const char* search) const { const char* p = Find(search); return p ? static_cast<uint32_t>(p - Data()) : static_cast<uint32_t>(-1); }

        /// Searches this string for a substring.
        ///
        /// \param[in] search The string to search for.
        /// \param[in] len The number of characters in the `search` string.
        /// \return The index of the start of the found string, or `uint32_t(-1)` if not found.
        [[nodiscard]] uint32_t FindIndex(const char* search, uint32_t len) const { const char* p = Find(search, len); return p ? static_cast<uint32_t>(p - Data()) : static_cast<uint32_t>(-1); }

        /// Searches this string for a range of characters.
        ///
        /// \param[in] search The range of characters to search for.
        /// \return The index of pointer to the start of the found string, or nullptr if not found.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] uint32_t FindIndex(const R& search) const { const char* p = Find(search); return p ? static_cast<uint32_t>(p - Data()) : static_cast<uint32_t>(-1); }

        /// Checks if this string contains a character.
        ///
        /// \param[in] search The character to search for.
        /// \return True if the character is found, false otherwise.
        [[nodiscard]] bool Contains(char search) const { return Find(search) != nullptr; }

        /// Checks if this string contains a null-terminated string.
        ///
        /// \param[in] search The null-terminated string to search for.
        /// \return True if the string is found, false otherwise.
        [[nodiscard]] bool Contains(const char* search) const { return Find(search) != nullptr; }

        /// Checks if this string contains a substring.
        ///
        /// \param[in] search The string to search for.
        /// \param[in] len The number of characters in the `search` string.
        /// \return True if the string is found, false otherwise.
        [[nodiscard]] bool Contains(const char* search, uint32_t len) const { return Find(search, len) != nullptr; }

        /// Checks if this string contains a range of characters.
        ///
        /// \param[in] search The range of characters to search for.
        /// \return True if the string is found, false otherwise.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] bool Contains(const R& search) const { return Find(search) != nullptr; }

        // ----------------------------------------------------------------------------------------
        // Converters

        /// Parses the string into a integral value.
        /// If successful, an integer value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, a range error
        /// occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is
        /// returned. If no conversion can be performed, zero is returned.
        ///
        /// \param[in] base Optional. The numerical base of the value being parsed.
        /// \return The parsed number.
        template <typename T>
        [[nodiscard]] T ToInteger(int32_t base = 10) const
        {
            const char* end = End();
            return StrToInt<T>(Begin(), &end, base);
        }

        /// Parses the string into a floating point value.
        /// If successful, a floating point value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, a range error
        /// occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is
        /// returned. If no conversion can be performed, zero is returned.
        ///
        /// \return The parsed number.
        template <typename T = float>
        [[nodiscard]] T ToFloat() const
        {
            const char* end = End();
            return StrToFloat<T>(Begin(), &end);
        }

        // ----------------------------------------------------------------------------------------
        // Comparison

        /// Compares this string to `len` characters of `str` and returns the result of the comparison.
        ///
        /// \param str The string to compare against.
        /// \param len The maximum number of characters to compare. Must be less than or equal to
        ///     the string length of `str`.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        [[nodiscard]] int32_t CompareTo(const char* str, uint32_t len) const;

        /// Compares this string to the null terminated string `str` and returns the result of the comparison.
        ///
        /// \param str The string to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        [[nodiscard]] int32_t CompareTo(const char* str) const { return CompareTo(str, StrLen(str)); }

        /// Compares this string to a range of characters.
        ///
        /// \param range The range of characters to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] int32_t CompareTo(const R& range) const { return CompareTo(range.Data(), range.Size()); }

        /// Compares this string to `len` characters of `str` and returns the result of the comparison.
        ///
        /// \param str The string to compare against.
        /// \param len The maximum number of characters to compare. Must be less than or equal to
        ///     the string length of `str`.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        [[nodiscard]] int32_t CompareToI(const char* str, uint32_t len) const;

        /// Compares this string to the null terminated string `str` and returns the result of the comparison.
        ///
        /// \param str The string to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        [[nodiscard]] int32_t CompareToI(const char* str) const { return CompareToI(str, StrLen(str)); }

        /// Compares this string to a range of characters.
        ///
        /// \param range The range of characters to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        template <ContiguousRangeOf<const char> R>
        [[nodiscard]] int32_t CompareToI(const R& range) const { return CompareToI(range.Data(), range.Size()); }

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first character in the string.
        ///
        /// \return A pointer to the first character.
        [[nodiscard]] const char* Begin() const { return Data(); }

        /// \copydoc Begin()
        [[nodiscard]] char* Begin() { return Data(); }

        /// Gets a pointer to one past the last character in the string.
        /// This always points to the null terminator.
        ///
        /// \return A pointer to one past the last character.
        [[nodiscard]] const char* End() const { return Data() + Size(); }

        /// \copydoc End()
        [[nodiscard]] char* End() { return Data() + Size(); }

        /// \copydoc Begin()
        [[nodiscard]] const char* begin() const { return Begin(); }

        /// \copydoc Begin()
        [[nodiscard]] char* begin() { return Begin(); }

        /// \copydoc End()
        [[nodiscard]] const char* end() const { return End(); }

        /// \copydoc End()
        [[nodiscard]] char* end() { return End(); }

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Sets the size of the string to zero. Does not affect memory allocation.
        void Clear() { SetSize(0); }

        /// Inserts the character into the string at index.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the string to insert at.
        /// \param c The character to insert.
        void Insert(uint32_t index, char c) { Insert(index, &c, 1); }

        /// Inserts the null terminated string into the string at index.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the string to insert at.
        /// \param str The string to insert.
        void Insert(uint32_t index, const char* str) { Insert(index, str, StrLen(str)); }

        /// Inserts the characters in the range `[begin, end)` into the string at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the string to insert at.
        /// \param begin The beginning, inclusive, of the character range to copy from.
        /// \param end The end, exclusive, of the character range to copy from.
        void Insert(uint32_t index, const char* begin, const char* end);

        /// Inserts `len` characters of a string into the string at index.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the string to insert at.
        /// \param str The string to insert.
        /// \param len The number of characters to copy into the string.
        void Insert(uint32_t index, const char* str, uint32_t len);

        /// Erases `count` characters from the string starting at `index`.
        /// Asserts if `index` is out of range.
        ///
        /// \param index The index in the string to insert at.
        /// \param count The number of characters to remove.
        void Erase(uint32_t index, uint32_t count);

        /// Appends the character `c` to the end of the string.
        ///
        /// \param c The character to append to the string.
        void PushBack(char c) { Insert(Size(), &c, 1); }

        /// Prepends the character `c` to the start of the string.
        ///
        /// \param c The character to prepend to the string.
        void PushFront(char c) { Insert(0, &c, 1); }

        /// Removes the last character from the end of the string.
        ///
        /// \return Returns the character that was removed.
        char PopBack();

        /// Removes the first character from the end of the string.
        ///
        /// \return Returns the character that was removed.
        char PopFront();

        /// Appends the character to the end of this string.
        ///
        /// \param c The character to append.
        void Append(char c) { Insert(Size(), &c, 1); }

        /// Appends the null terminated string to the end of this string.
        ///
        /// \param str The string to append.
        void Append(const char* str) { Insert(Size(), str, StrLen(str)); }

        /// Appends the characters in the range `[begin, end)` to the end of this string.
        ///
        /// \param begin The beginning, inclusive, of the character range to append.
        /// \param end The end, exclusive, of the character range to append.
        void Append(const char* begin, const char* end) { Insert(Size(), begin, end); }

        /// Appends `len` characters of the string to the end of this string.
        ///
        /// \param str The string to append.
        /// \param len The number of characters to copy into the string.
        void Append(const char* str, uint32_t len) { Insert(Size(), str, len); }

        /// Appends the string object to the end of this string.
        ///
        /// \param str The string to append.
        void Append(const String& str) { Insert(Size(), str.Data(), str.Size()); }

        /// Replaces the contents of this string with a copy of the null terminated `str`.
        ///
        /// \param str The string source to copy from.
        void Assign(const char* str) { Clear(); Append(str); }

        /// Replaces the contents of this string with a copy of the characters in the range
        /// `[begin, end)`.
        ///
        /// \param begin The beginning, inclusive, of the character range to copy from.
        /// \param end The end, exclusive, of the character range to copy from.
        void Assign(const char* begin, const char* end) { Clear(); Append(begin, end); }

        /// Replaces the contents of this string with a copy of `len` characters of the string `str`.
        ///
        /// \param str The string source to copy from.
        void Assign(const char* str, uint32_t len) { Clear(); Append(str, len); }

        /// Replaces the contents of this string with `str`.
        ///
        /// \param str The string source to copy from.
        void Assign(const String& str) { Clear(); Append(str); }

        /// Replace all occurrences of the string `search` with the string `replacement`.
        ///
        /// \param search The string to search for.
        /// \param searchLen The number of characters in the `search` string.
        /// \param replacement The string to replace `search` with.
        /// \param replacementLen The number of characters in the `replacement` string.
        /// \return The number of replacements made.
        uint32_t Replace(const char* search, uint32_t searchLen, const char* replacement, uint32_t replacementLen);

        /// Replace all occurrences of the null-terminated string `search` with the
        /// null-terminated string `replacement`.
        ///
        /// \param search The null-terminated string to search for.
        /// \param replacement The null-terminated string to replace `search` with.
        /// \return The number of replacements made.
        uint32_t Replace(const char* search, const char* replacement) { return Replace(search, StrLen(search), replacement, StrLen(replacement)); }

        /// Replace all occurrences of the range of characters `search` with the null-terminated
        /// string `replacement`.
        ///
        /// \param search The range of characters to search for.
        /// \param replacement The null-terminated string to replace `search` with.
        /// \return The number of replacements made.
        template <ContiguousRangeOf<const char> R>
        uint32_t Replace(const R& search, const char* replacement) { return Replace(search.Data(), search.Size(), replacement, StrLen(replacement)); }

        /// Replace all occurrences of the null-terminated string `search` with the range of
        /// characters `replacement`.
        ///
        /// \param search The null-terminated string to search for.
        /// \param replacement The range of characters to replace `search` with.
        /// \return The number of replacements made.
        template <ContiguousRangeOf<const char> R>
        uint32_t Replace(const char* search, const R& replacement) { return Replace(search, StrLen(search), replacement.Data(), replacement.Size()); }

        /// Replace all occurrences of the range of characters `search` with the range of
        /// characters `replacement`.
        ///
        /// \param search The range of characters to search for.
        /// \param replacement The range of characters to replace `search` with.
        /// \return The number of replacements made.
        template <ContiguousRangeOf<const char> R1, ContiguousRangeOf<const char> R2>
        uint32_t Replace(const R1& search, const R2& replacement) { return Replace(search.Data(), search.Size(), replacement.Data(), replacement.Size()); }

    private:
        // Grows the internal capacity to make space for `len` elements.
        void GrowBy(uint32_t len);

        // Calculate geometric growth that will be necessary to include `len` additional elements.
        uint32_t CalculateGrowth(uint32_t len, uint32_t size, uint32_t capacity) const;

        // Sets the size of the string object, and writes the null terminator.
        void SetSize(uint32_t size);

        // Sets the size of the embedded string, and writes the null terminator.
        void SetSizeEmbed(uint32_t size);

        // Sets the size of the heap allocated string, and writes the null terminator.
        void SetSizeHeap(uint32_t size);

        // Gets the size of the embedded string.
        uint32_t GetSizeEmbed() const { return (EmbedSize - 1) - (static_cast<uint32_t>(m_embed[EmbedSize - 1])); }

        // Copy the given string.
        void CopyFrom(const String& x);

        // Move from the given string into this object.
        void MoveFrom(String&& x);

    private:
        friend class StringTestAttorney;

        static constexpr char HeapFlag = char(-1);
        static constexpr uint32_t EmbedSize = MaxEmbedCharacters + 1; // +1 for remaining count & heap flag

        // Structure for tracking the string when it lives on the heap.
        struct Heap
        {
            char* data;
            uint32_t size;
            uint32_t capacity;
        };

        // Need at least 1 byte beyond Heap to store the heap flag.
        static_assert(sizeof(Heap) < EmbedSize);

        Allocator& m_allocator;
        union
        {
            // The last byte encodes if we're using heap or embed. It also stores the number
            // of available characters remaining in the embed array. When the string is full
            // the last byte will be zero, so it acts as the null terminator too.
            char m_embed[EmbedSize];
            Heap m_heap;
        };
    };

    /// User-defined literal that creates a String object from a string literal.
    ///
    /// Example:
    /// ```cpp
    /// const String value1 = "abc\0\0def";     // "abc" (size = 3)
    /// const String value2 = "abc\0\0def"_s;   // "abc\0\0def" (size = 8)
    /// ```
    ///
    /// \param[in] str A pointer to the string literal.
    /// \param[in] len The length of the string literal, not including the null terminator.
    /// \return A constructed String object holding an equivalent value to the literal.
    [[nodiscard]] inline String operator"" _s(const char* str, size_t len) noexcept
    {
        return String(str, static_cast<uint32_t>(len));
    }
}

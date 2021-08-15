// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/types.h"

namespace he
{
    class String final
    {
    public:
        /// The type of elements in the string.
        using ElementType = char;

        /// The maximum number of characters that can be stored inline in the object.
        static constexpr uint32_t MaxEmbedCharacters = 63;

        /// The maximum number of characters that can be stored on the heap.
        static constexpr uint32_t MaxHeapCharacters = 0xfffffffe;

        // ----------------------------------------------------------------------------------------
        // Raw String Algorithms

        /// Checks if a string is nullptr or the first character is a null terminator.
        ///
        /// \param s The string to check.
        /// \return True when `s` is null or empty.
        static constexpr bool IsEmpty(const char* s);

        /// Gets the length of a null terminated string using a constexpr algorithm. This may be
        /// slower than \see Length(const char*) for cases that are not calculated at compile time.
        ///
        /// \param s The string to get the length of.
        /// \return The length of the string.
        static constexpr uint32_t LengthConst(const char* s);

        /// Gets the length of a null terminated string.
        ///
        /// \param s The string to get the length of.
        /// \return The length of the string.
        static uint32_t Length(const char* s);

        /// Gets the length of a null terminated string, up to `len`.
        ///
        /// \param s The string to get the length of.
        /// \param len The maximum length of the string to check.
        /// \return The length of the string.
        static uint32_t LengthN(const char* s, uint32_t len);

        /// Compares the null terminated strings and returns the result of the comparison.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        static int32_t Compare(const char* a, const char* b);

        /// Compares the null terminated strings, up to `len`, and returns the result of the
        /// comparison.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \param len The maximum length of the string to check.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        static int32_t CompareN(const char* a, const char* b, uint32_t len);

        /// Compares the null terminated strings in a case-insensative manner and returns the
        /// result of the comparison.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        static int32_t CompareI(const char* a, const char* b);

        /// Compares the null terminated strings, up to `len`, in a case-insensative manner and
        /// returns the result of the comparison.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \param len The maximum length of the string to check.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        static int32_t CompareNI(const char* a, const char* b, uint32_t len);

        /// Compares the null terminated strings and returns true if they are equal.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return True if the strings are equal, false otherwise.
        static bool Equal(const char* a, const char* b) { return Compare(a, b) == 0; }

        /// Compares the null terminated strings, up to `len`, and returns true if they are equal.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return True if the strings are equal, false otherwise.
        static bool EqualN(const char* a, const char* b, uint32_t len) { return CompareN(a, b, len) == 0; }

        /// Compares the null terminated strings in a case-insensative manner and
        /// returns true if they are equal.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return True if the strings are equal, false otherwise.
        static bool EqualI(const char* a, const char* b) { return CompareI(a, b) == 0; }

        /// Compares the null terminated strings, up to `len`, in a case-insensative manner and
        /// returns true if they are equal.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return True if the strings are equal, false otherwise.
        static bool EqualNI(const char* a, const char* b, uint32_t len) { return CompareNI(a, b, len) == 0; }

        /// Compares the null terminated strings and returns true if `a` is less than `b`.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return True if `a` is less than `b`, false otherwise.
        static bool Less(const char* a, const char* b) { return Compare(a, b) < 0; }

        /// Compares the null terminated strings, up to `len`, and returns true if `a` is less than `b`.
        ///
        /// \param a The left-hand side of the comparison operation.
        /// \param b The right-hand side of the comparison operation.
        /// \return True if `a` is less than `b`, false otherwise.
        static bool LessN(const char* a, const char* b, uint32_t len) { return CompareN(a, b, len) < 0; }

        /// Copies the source string into the destination buffer, including the null terminator.
        /// The destination buffer is garuanteed to be null terminated if `dstLen > 0`.
        ///
        /// \param dst The destination buffer to copy into.
        /// \param dstLen The size of the destination buffer.
        /// \param src The string to copy from.
        /// \return The number of characters copied into `dst`, excluding the null terminator.
        static uint32_t Copy(char* dst, uint32_t dstLen, const char* src);

        /// \copydoc Copy(char*, uint32_t, const char*)
        template <uint32_t N>
        static uint32_t Copy(char (&dst)[N], const char* src) { return Copy(dst, N, src); }

        /// Copies up to `srcLen` characters from the source string into the destination buffer,
        /// including the null terminator. The destination buffer is garuanteed to be null
        /// terminated if `dstLen > 0`.
        ///
        /// \param dst The destination buffer to copy into.
        /// \param dstLen The size of the destination buffer.
        /// \param src The string to copy from.
        /// \param srcLen The maximum number of characters to copy.
        /// \return The number of characters copied into `dst`, excluding the null terminator.
        static uint32_t CopyN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen);

        /// \copydoc CopyN(char*, uint32_t, const char*, uint32_t)
        template <uint32_t N>
        static uint32_t CopyN(char (&dst)[N], const char* src, uint32_t srcLen) { return CopyN(dst, N, src, srcLen); }

        /// Copies the source string to the end of the destination string.
        /// The destination buffer is garuanteed to be null terminated if `dstLen > 0`.
        ///
        /// \param dst The destination buffer to copy into.
        /// \param dstLen The size of the destination buffer.
        /// \param src The string to copy from.
        /// \return The length of the `dst` string after the concatenation completes.
        static uint32_t Cat(char* dst, uint32_t dstLen, const char* src);

        /// \copydoc Cat(char*, uint32_t, const char*)
        template <uint32_t N>
        static uint32_t Cat(char (&dst)[N], const char* src) { return Cat(dst, N, src); }

        /// Copies up to `srcLen` characters from the source string to the end of the destination
        /// string. The destination buffer is garuanteed to be null terminated if `dstLen > 0`.
        ///
        /// \param dst The destination buffer to copy into.
        /// \param dstLen The size of the destination buffer.
        /// \param src The string to copy from.
        /// \param srcLen The maximum number of characters to copy.
        /// \return The length of the `dst` string after the concatenation completes.
        static uint32_t CatN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen);

        /// \copydoc CatN(char*, uint32_t, const char*, uint32_t)
        template <uint32_t N>
        static uint32_t CatN(char (&dst)[N], const char* src, uint32_t srcLen) { return CatN(dst, N, src, srcLen); }

        /// Searching the null terminated string for a character.
        ///
        /// \param str The string to search within.
        /// \param search The character to search for.
        /// \return A pointer to the found character in `str`, or a null pointer if not found.
        static const char* Find(const char* str, char search);

        /// Searching the null terminated string for a character.
        ///
        /// \param str The string to search within.
        /// \param search The character to search for.
        /// \return A pointer to the found character in `str`, or a null pointer if not found.
        static char* Find(char* str, char search);

        /// Searching the null terminated string for a substring.
        ///
        /// \param str The string to search within.
        /// \param search The string to search for.
        /// \return A pointer to the start of the found substring in `str`, or a null pointer if
        /// not found. If `str` is empty, then `search` is returned.
        static const char* Find(const char* str, const char* search);

        /// Searching the null terminated string for a substring.
        ///
        /// \param str The string to search within.
        /// \param search The string to search for.
        /// \return A pointer to the start of the found substring in `str`, or a null pointer if
        /// not found. If `str` is empty, then `search` is returned.
        static char* Find(char* str, const char* search);

        /// Parses the string into a integral value.
        /// If successful, an integer value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, a range error
        /// occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is
        /// returned. If no conversion can be performed, zero is returned.
        ///
        /// \param[in] str The string to parse.
        /// \param[in] end Optional. A pointer to the end of the string to stop parsing. If
        ///     nullptr (default) the string is parsed until a null terminator is reached.
        /// \param[in] base Optional. The numerical base of the value being parsed.
        /// \return The parsed number.
        template <typename T>
        static T ToInteger(const char* str, const char** end = nullptr, int32_t base = 10);

        /// Parses the string into a floating point value.
        /// If successful, a floating point value corresponding to the contents of str is returned.
        /// If the converted value falls out of range of corresponding return type, a range error
        /// occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is
        /// returned. If no conversion can be performed, zero is returned.
        ///
        /// \param[in] str The string to parse.
        /// \param[in] end Optional. A pointer to the end of the string to stop parsing. If
        ///     nullptr (default) the string is parsed until a null terminator is reached.
        /// \return The parsed number.
        template <typename T>
        static T ToFloat(const char* str, const char** end = nullptr);

    public:
        // ----------------------------------------------------------------------------------------
        // Construction

        /// Construct an empty string.
        ///
        /// \param allocator The allocator to use for any allocations.
        explicit String(Allocator& allocator);

        /// Construct a string by copying from the null terminated string `str`.
        ///
        /// \param allocator The allocator to use for any allocations.
        /// \param str The string to copy from.
        String(Allocator& allocator, const char* str);

        /// Construct a string by copying `len` characters from the string `str`.
        /// This does not stop early if it encounters a null terminator before `len` characters.
        ///
        /// \param allocator The allocator to use for any allocations.
        /// \param str The string to copy from.
        /// \param len The number of characters to copy.
        String(Allocator& allocator, const char* str, uint32_t len);

        /// Construct a string by copying `x`, and using `allocator` for this string's allocations.
        ///
        /// \param allocator The allocator to use for any allocations.
        /// \param x The string to copy from.
        String(Allocator& allocator, const String& x);

        /// Construct a string by moving `x`, and using `allocator` for this string's allocations.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param allocator The allocator to use for any allocations.
        /// \param x The string to move from.
        String(Allocator& allocator, String&& x);

        /// Construct a string by copying `x`, using the allocator from `x`.
        ///
        /// \param x The string to copy from.
        String(const String& x);

        /// Construct a string by moving `x`, using the allocator from `x`.
        ///
        /// \param x The string to move from.
        String(String&& x);

        /// Destructs the string, freeing any memory allocations.
        ~String();

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Copy the string `x` into this string.
        ///
        /// \param x The string to copy from.
        String& operator=(const String& x);

        /// Move the string `x` into this string.
        /// If the allocators do not match then a copy operation will be performed.
        ///
        /// \param x The string to move from.
        String& operator=(String&& x);

        /// Gets a reference to the character at `index`. Asserts if `index` is not less
        /// than \see Size().
        ///
        /// \param index The index of the character to return.
        /// \return A reference to the character at `index`.
        const char& operator[](uint32_t index) const;

        /// \copydoc operator[](uint32_t)
        char& operator[](uint32_t index) { return const_cast<char&>(const_cast<const String&>(*this)[index]); }

        /// Appends the string object to the end of this string.
        ///
        /// \param str The string to append.
        String& operator+=(const String& str) { Insert(Size(), str.Data(), str.Size()); return *this; }

        /// Appends the null terminated string to the end of this string.
        ///
        /// \param str The string to append.
        String& operator+=(const char* str) { Insert(Size(), str, Length(str)); return *this; }

        /// Appends the character to the end of this string.
        ///
        /// \param c The character to append.
        String& operator+=(char c) { Insert(Size(), &c, 1); return *this; }

        /// Checks if this string is equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are equal, false otherwise.
        bool operator==(const String& x) const { return CompareTo(x) == 0; }

        /// Checks if this string is not equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if the strings are not equal, false otherwise.
        bool operator!=(const String& x) const { return CompareTo(x) != 0; }

        /// Checks if this string is less than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than `x`, false otherwise.
        bool operator<(const String& x) const { return CompareTo(x) < 0; }

        /// Checks if this string is less than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is less than or equal to `x`, false otherwise.
        bool operator<=(const String& x) const { return CompareTo(x) <= 0; }

        /// Checks if this string is greater than `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than `x`, false otherwise.
        bool operator>(const String& x) const { return CompareTo(x) > 0; }

        /// Checks if this string is greater than or equal to `x`.
        ///
        /// \param x The string to check against.
        /// \return True if this string is greater than or equal to `x`, false otherwise.
        bool operator>=(const String& x) const { return CompareTo(x) >= 0; }

        // ----------------------------------------------------------------------------------------
        // Capacity

        /// Checks if this string is using embedded storage for the string data.
        ///
        /// \return Returns true if no heap allocation is being used.
        bool IsEmbedded() const { return m_embed[EmbedSize - 1] != HeapFlag; }

        /// Checks if this string is empty.
        ///
        /// \return Returns true if this is an empty string.
        bool IsEmpty() const { return Size() == 0; }

        /// The capacity the string has for characters.
        ///
        /// Note: This is not the size of the allocation, but rather the number of total
        /// characters the string can hold before having to reallocate.
        ///
        /// \return Number of total chracters this string can store.
        uint32_t Capacity() const { return (IsEmbedded() ? EmbedSize : m_heap.capacity) - 1; }

        /// The length of the string that is currently stored.
        ///
        /// \return Number of characters in the string, not including the null terminator.
        uint32_t Size() const { return IsEmbedded() ? GetSizeEmbed() : m_heap.size; }

        /// Reserves capacity for `len` characters. \see Capacity() is garuanteed to return
        /// at least `len` after this operation.
        ///
        /// \param len The length of characters to reserve capacity for.
        void Reserve(uint32_t len);

        /// Resizes the string to be `len` characters long. If this size is longer than the
        /// current length then the new characters are uninitialized.
        ///
        /// \param len The length to make the string.
        void Resize(uint32_t len, DefaultInitTag);

        /// Resizes the string to be `len` characters long. If this size is longer than the
        /// current length then `c` is used to fill in the new characters.
        ///
        /// \param len The length to make the string.
        /// \param c The character to use to copy to new entries in the string.
        void Resize(uint32_t len, char c = '\0');

        /// Shrinks the memory allocation to fit the current size of the string. If the string
        /// is allocated on the heap and shrinking would let it fit into the embedded data it
        /// will copy the string into the embedded data and deallocate the heap string.
        void ShrinkToFit();

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Gets a pointer to the string's character buffer.
        ///
        /// \return A pointer to the character buffer.
        const char* Data() const { return IsEmbedded() ? m_embed : m_heap.data; }

        /// \copydoc Data()
        char* Data() { return IsEmbedded() ? m_embed : m_heap.data; }

        /// Gets a reference to the string's first character. The string must not be empty.
        ///
        /// \return A reference to the first character.
        const char& Front() const;

        /// \copydoc Front()
        char& Front() { return const_cast<char&>(const_cast<const String*>(this)->Front()); }

        /// Gets a reference to the string's last character. The string must not be empty.
        ///
        /// \return A reference to the last character.
        const char& Back() const;

        /// \copydoc Back()
        char& Back() { return const_cast<char&>(const_cast<const String*>(this)->Back()); }

        /// Returns a reference to the allocator object used by the string.
        ///
        /// \return The allocator object this string uses.
        Allocator& GetAllocator() const { return m_allocator; }

        // ----------------------------------------------------------------------------------------
        // Comparison

        /// Compares this string to another and returns the result of the comparison.
        ///
        /// \param x The string to compare against.
        /// \return The result of the comparison.
        ///     If the values are equal, zero is returned.
        ///     If this string is less than `x`, a negative value is returned.
        ///     If this string is greater than `x`, a positive value is returned.
        int32_t CompareTo(const String& x) const;

        // ----------------------------------------------------------------------------------------
        // Iterators

        /// Gets a pointer to the first character in the string.
        ///
        /// \return A pointer to the first character.
        const char* Begin() const { return Data(); }

        /// \copydoc Begin()
        char* Begin() { return Data(); }

        /// Gets a pointer to one past the last character in the string.
        /// This always points to the null terminator.
        ///
        /// \return A pointer to one past the last character.
        const char* End() const { return Data() + Size(); }

        /// \copydoc End()
        char* End() { return Data() + Size(); }

        /// \copydoc Begin()
        const char* begin() const { return Begin(); }

        /// \copydoc Begin()
        char* begin() { return Begin(); }

        /// \copydoc End()
        const char* end() const { return End(); }

        /// \copydoc End()
        char* end() { return End(); }

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
        void Insert(uint32_t index, const char* str) { Insert(index, str, Length(str)); }

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

        /// Removes the last character from the end of the string.
        char PopBack();

        /// Appends the character to the end of this string.
        ///
        /// \param c The character to append.
        void Append(char c) { Insert(Size(), &c, 1); }

        /// Appends the null terminated string to the end of this string.
        ///
        /// \param str The string to append.
        void Append(const char* str) { Insert(Size(), str, Length(str)); }

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

        /// Replaces the contents of this string with a copy of `len` characters of the string `str`.
        ///
        /// \param str The string source to copy from.
        void Assign(const char* str, uint32_t len) { Clear(); Append(str, len); }

        /// Replaces the contents of this string with `str`.
        ///
        /// \param str The string source to copy from.
        void Assign(const String& str) { Clear(); Append(str); }

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
}

#include "he/core/inline/string.inl"

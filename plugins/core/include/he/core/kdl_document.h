// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/enum_ops.h"
#include "he/core/hash_table.h"
#include "he/core/kdl_reader.h"
#include "he/core/kdl_writer.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/variant.h"
#include "he/core/vector.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// Represents a value in a KDL document.
    class KdlValue
    {
    public:
        /// Type of the underlying variant used to store the value.
        using VariantType = Variant<bool, int64_t, uint64_t, double, String>;

        /// Enumeration of the possible value types.
        enum class Kind : uint8_t
        {
            Bool,
            Int,
            Uint,
            Float,
            String,
            Null,
        };

        /// Converts a Kind enumeration value to a VariantType index value.
        ///
        /// \tparam K The Kind enumeration value.
        /// \tparam I The VariantType index value.
        /// \return The VariantType index value.
        template <Kind K, VariantType::IndexType I = EnumToValue(K)>
        static consteval IndexConstant<I> AsIndex() { return IndexConstant<I>{}; }

    public:
        /// Construct a null KDL value.
        KdlValue() noexcept : m_value(), m_type() {}

        /// Construct a KDL value.
        ///
        /// \param v The value to construct with.
        /// \param t Optional. The type annotation of the value.
        KdlValue(bool v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Bool>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(signed char v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Int>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(short v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Int>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(int v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Int>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(long v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Int>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(long long v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Int>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(unsigned char v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Uint>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(unsigned short v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Uint>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(unsigned int v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Uint>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(unsigned long v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Uint>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(unsigned long long v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Uint>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(double v, StringView t = {}) noexcept : m_value(AsIndex<Kind::Float>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(StringView v, StringView t = {}) noexcept : m_value(AsIndex<Kind::String>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(const char* v, StringView t = {}) noexcept : m_value(AsIndex<Kind::String>(), v), m_type(t) {}

        /// \copydoc KdlValue(bool,StringView)
        KdlValue(nullptr_t, StringView t = {}) noexcept : m_value(), m_type(t) {}

        /// Get the type annotation, if any, of the value.
        ///
        /// \return The type annotation of the value.
        const StringView& Type() const { return m_type; }

        /// Get the underlying value variant.
        ///
        /// \return The underlying value variant.
        const VariantType& Value() const { return m_value;}

        /// Set the type annotation of the value.
        ///
        /// \param[in] type The type annotation to set.
        void SetType(StringView type) { m_type = type; }

        /// Clear the type annotation of the value.
        void ClearType() { m_type = {}; }

        /// Get the kind of the value.
        ///
        /// \return The kind of the value.
        Kind GetKind() const { return m_value.IsValid() ? static_cast<Kind>(m_value.Index()) : Kind::Null; }

        /// Check if the value is valid (not null).
        ///
        /// \return True if the value is valid, false otherwise.
        bool IsValid() const { return m_value.IsValid(); }

        /// Clear the value, making it null.
        void Clear() { m_value.Clear(); }

        /// Check if the value is of the specified kind.
        ///
        /// \tparam K The kind to check for.
        /// \return True if the value is of the specified kind, false otherwise.
        template <Kind K> bool Is() const { return m_value.Index() == EnumToValue(K); }

        /// Check if the value is null.
        ///
        /// \return True if the value is null, false otherwise.
        bool IsNull() const { return !IsValid(); }

        /// Check if the value is a boolean.
        ///
        /// \return True if the value is a boolean, false otherwise.
        bool IsBool() const { return Is<Kind::Bool>(); }

        /// Check if the value is an integer.
        ///
        /// \return True if the value is an integer, false otherwise.
        bool IsInt() const { return Is<Kind::Int>(); }

        /// Check if the value is an unsigned integer.
        ///
        /// \return True if the value is an unsigned integer, false otherwise.
        bool IsUint() const { return Is<Kind::Uint>(); }

        /// Check if the value is a floating point number.
        ///
        /// \return True if the value is a floating point number, false otherwise.
        bool IsFloat() const { return Is<Kind::Float>(); }

        /// Check if the value is a string.
        ///
        /// \return True if the value is a string, false otherwise.
        bool IsString() const { return Is<Kind::String>(); }

        /// Sets the value to the specified kind, with a default initialized value.
        ///
        /// \tparam K The kind to set the value to.
        /// \return A reference to the value.
        template <Kind K> decltype(auto) Set() { return m_value.Emplace<EnumToValue(K)>(); }

        /// Sets the value to null.
        void SetNull() { Clear(); }

        /// Sets the value to a boolean.
        void SetBool(bool v) { Set<Kind::Bool>() = v; }

        /// Sets the value to an integer.
        void SetInt(int64_t v) { Set<Kind::Int>() = v; }

        /// Sets the value to an unsigned integer.
        void SetUint(uint64_t v) { Set<Kind::Uint>() = v; }

        /// Sets the value to a floating point number.
        void SetFloat(double v) { Set<Kind::Float>() = v; }

        /// Sets the value to a string.
        void SetString(StringView v) { Set<Kind::String>() = v; }

        /// Gets the value of the specified kind, if it is the correct kind.
        ///
        /// \note This will assert if the kind is not the correct kind.
        ///
        /// \tparam K The kind to get the value of.
        /// \return A reference to the value.
        template <Kind K> decltype(auto) Get() const { return m_value.Get<EnumToValue(K)>(); }

        /// Gets the value as boolean, if it is a boolean.
        ///
        /// \note This will assert if the value is not a boolean.
        /// Use \ref IsBool to check the kind first.
        ///
        /// \return The boolean value.
        bool Bool() const { return Get<Kind::Bool>(); }

        /// Gets the value as an integer, if it is an integer.
        ///
        /// \note This will assert if the value is not an integer.
        /// Use \ref IsInt to check the kind first.
        ///
        /// \return The integer value.
        int64_t Int() const { return Get<Kind::Int>(); }

        /// Gets the value as an unsigned integer, if it is an unsigned integer.
        ///
        /// \note This will assert if the value is not an unsigned integer.
        /// Use \ref IsUint to check the kind first.
        ///
        /// \return The unsigned integer value.
        uint64_t Uint() const { return Get<Kind::Uint>(); }

        /// Gets the value as a double-precision floating point number,
        /// if it is a floating point number.
        ///
        /// \note This will assert if the value is not a floating point number.
        /// Use \ref IsFloat to check the kind first.
        ///
        /// \return The floating point number value.
        double Float() const { return Get<Kind::Float>(); }

        /// Gets the value as a string, if it is a string.
        ///
        /// \note This will assert if the value is not a string.
        /// Use \ref IsString to check the kind first.
        ///
        /// \return The string value.
        StringView String() const { return Get<Kind::String>(); }

        KdlValue& operator=(nullptr_t) { SetNull(); return *this; }
        KdlValue& operator=(bool v) { SetBool(v); return *this; }
        KdlValue& operator=(signed char v) { SetInt(v); return *this; }
        KdlValue& operator=(short v) { SetInt(v); return *this; }
        KdlValue& operator=(int v) { SetInt(v); return *this; }
        KdlValue& operator=(long v) { SetInt(v); return *this; }
        KdlValue& operator=(long long v) { SetInt(v); return *this; }
        KdlValue& operator=(unsigned char v) { SetUint(v); return *this; }
        KdlValue& operator=(unsigned short v) { SetUint(v); return *this; }
        KdlValue& operator=(unsigned int v) { SetUint(v); return *this; }
        KdlValue& operator=(unsigned long v) { SetUint(v); return *this; }
        KdlValue& operator=(unsigned long long v) { SetUint(v); return *this; }
        KdlValue& operator=(double v) { SetFloat(v); return *this; }
        KdlValue& operator=(const char* v) { SetString(v); return *this; }
        KdlValue& operator=(StringView v) { SetString(v); return *this; }

        [[nodiscard]] bool operator==(const KdlValue& x) const { return m_value == x.m_value && m_type == x.m_type; }
        [[nodiscard]] bool operator!=(const KdlValue& x) const { return !(*this == x); }

    private:
        VariantType m_value;
        StringView m_type;
    };

    // --------------------------------------------------------------------------------------------
    class KdlNode
    {
    public:
        explicit KdlNode(Allocator& allocator = Allocator::GetDefault()) noexcept
            : m_name(allocator)
            , m_type(allocator)
            , m_children(allocator)
            , m_arguments(allocator)
            , m_properties(allocator)
        {}

        explicit KdlNode(StringView name, Allocator& allocator = Allocator::GetDefault()) noexcept
            : KdlNode(allocator)
        {
            m_name = name;
        }

        explicit KdlNode(StringView name, StringView type, Allocator& allocator = Allocator::GetDefault()) noexcept
            : KdlNode(allocator)
        {
            m_name = name;
            m_type = type;
        }

        const String& Name() const { return m_name; }
        void SetName(StringView name) { m_name = name; }

        const String& Type() const { return m_type; }
        void SetType(StringView type) { m_type = type; }
        void ClearType() { m_type.Clear(); }

        const Vector<KdlNode>& Children() const { return m_children; }
        Vector<KdlNode>& Children() { return m_children; }

        const Vector<KdlValue>& Arguments() const { return m_arguments; }
        Vector<KdlValue>& Arguments() { return m_arguments; }

        const HashMap<String, KdlValue>& Properties() const { return m_properties; }
        HashMap<String, KdlValue>& Properties() { return m_properties; }

    private:
        String m_name;
        String m_type;
        Vector<KdlNode> m_children;
        Vector<KdlValue> m_arguments;
        HashMap<String, KdlValue> m_properties;
    };

    // --------------------------------------------------------------------------------------------
    class KdlDocument
    {
    public:
        explicit KdlDocument(Allocator& allocator = Allocator::GetDefault()) noexcept;

        Allocator& GetAllocator() const { return m_nodes.GetAllocator(); }

        KdlReadResult Read(StringView data);
        void Write(String& dst) const;

        String ToString() const;

        const Vector<KdlNode>& Nodes() const { return m_nodes; }
        Vector<KdlNode>& Nodes() { return m_nodes; }

        const KdlNode* begin() const { return m_nodes.begin(); }
        KdlNode* begin() { return m_nodes.begin(); }

        const KdlNode* end() const { return m_nodes.end(); }
        KdlNode* end() { return m_nodes.end(); }

    private:
        Vector<KdlNode> m_nodes;
    };
}

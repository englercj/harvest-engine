// Copyright Chad Engler

#include "he/core/toml_document.h"

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    class TomlDocumentReadHandler : public TomlReader::Handler
    {
    public:
        explicit TomlDocumentReadHandler(TomlDocument& doc) noexcept
            : m_doc(doc)
        {}

        TomlReadResult Result() const { return m_result; }

    private:
        bool StartDocument() override
        {
            // Nothing to do here
            return true;
        }

        bool EndDocument() override
        {
            // Nothing to do here
            return true;
        }

        bool Comment(StringView value) override
        {
            // Nothing to do here
            HE_UNUSED(value);
            return true;
        }

        bool Bool(bool value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetBool(value);
            return true;
        }

        bool Int(int64_t value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetInt(value);
            return true;
        }

        bool Uint(uint64_t value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetUint(value);
            return true;
        }

        bool Float(double value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetFloat(value);
            return true;
        }

        bool String(StringView value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetString(value);
            return true;
        }

        bool DateTime(SystemTime value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetDateTime(value);
            return true;
        }

        bool Time(Duration value) override
        {
            HE_ASSERT(m_value);
            if (m_value->IsArray())
                m_value->Array().EmplaceBack(value);
            else
                m_value->SetTime(value);
            return true;
        }

        bool Table(Span<const he::String> path, bool isArray) override
        {
            if (!WalkPath(path))
                return false;

            if (isArray)
            {
                if (!m_value->IsArray())
                    m_value->SetArray(m_doc.GetAllocator());

                m_value = &m_value->Array().EmplaceBack();
            }

            return true;
        }

        bool Key(Span<const he::String> path) override
        {
            return WalkPath(path);
        }

        bool StartInlineTable() override
        {
            HE_ASSERT(m_value);
            m_value->SetTable(m_doc.GetAllocator());
            return true;
        }

        bool EndInlineTable(uint32_t length) override
        {
            HE_ASSERT(m_value && m_value->IsTable());
            HE_ASSERT(m_value->Table().Size() == length);
            return true;
        }

        bool StartArray() override
        {
            HE_ASSERT(m_value);
            m_value->SetArray(m_doc.GetAllocator());
            return true;
        }

        bool EndArray(uint32_t length) override
        {
            HE_ASSERT(m_value && m_value->IsArray());
            HE_ASSERT(m_value->Array().Size() == length);
            return true;
        }

    private:
        bool WalkPath(Span<const he::String> path)
        {
            m_value = &m_doc.Root();

            for (uint32_t i = 0; i < path.Size(); ++i)
            {
                const he::String& name = path[i];

                if (m_value->IsArray())
                    m_value = &m_value->Array().Back();

                if (m_value->IsTable())
                {
                    m_value = &m_value->Table()[name];
                }
                else if (!m_value->IsValid())
                {
                    m_value->SetTable(m_doc.GetAllocator());
                    m_value = &m_value->Table()[name];
                }
                else
                {
                    // Value is already set, this isn't valid. Break out of the loop so the error
                    // handling after this can run.
                    break;
                }
            }

            if (m_value->IsValid())
            {
                // If we get here it means the path refers a value that is already set. Keys are not
                // allowed to be set multiple times by the same document. Tables are also not allowed
                // to be defined multiple times.
                // TODO: Return error code because this table is not valid.
                m_result = { TomlReadError::InvalidDocument, 0, 0 };
                return false;
            }

            return true;
        }

    private:
        TomlDocument& m_doc;
        TomlValue* m_value{ nullptr };
        TomlReadResult m_result{};
    };

    // --------------------------------------------------------------------------------------------
    class TomlDocumentWriteHandler
    {
    public:
        void WriteValue(TomlWriter& writer, const TomlValue& value)
        {
            switch (value.GetKind())
            {
                case TomlValue::Kind::Bool: writer.Bool(value.Bool()); break;
                case TomlValue::Kind::Int: writer.Int(value.Int()); break;
                case TomlValue::Kind::Uint: writer.Uint(value.Uint()); break;
                case TomlValue::Kind::Float: writer.Float(value.Float()); break;
                case TomlValue::Kind::String: writer.String(value.String()); break;
                case TomlValue::Kind::DateTime: writer.DateTime(value.DateTime()); break;
                case TomlValue::Kind::Time: writer.Time(value.Time()); break;
                case TomlValue::Kind::Table: WriteTable(writer, value); break;
                case TomlValue::Kind::Array: WriteArray(writer, value); break;
                case TomlValue::Kind::Invalid: break;
            }
        }

    private:
        void WriteTable(TomlWriter& writer, const TomlValue& value)
        {
            HE_ASSERT(value.IsTable());

            if (m_arrayDepth > 0)
                writer.StartInlineTable();
            else if (!m_keys.IsEmpty())
                writer.Table(m_keys, m_nextTableIsArray);

            m_nextTableIsArray = false;

            // Write non-table values first so they have the right table header
            for (auto&& it : value.Table())
            {
                if (it.value.IsTable() || IsArrayOfTables(it.value))
                    continue;

                writer.Key(it.key);
                WriteValue(writer, it.value);
            }

            // Next write the table values each as their own header
            for (auto&& it : value.Table())
            {
                if (!it.value.IsTable() && !IsArrayOfTables(it.value))
                    continue;

                m_keys.PushBack(it.key);
                WriteValue(writer, it.value);
                m_keys.PopBack();
            }

            if (m_arrayDepth > 0)
                writer.EndInlineTable();
        }

        void WriteArray(TomlWriter& writer, const TomlValue& value)
        {
            HE_ASSERT(value.IsArray());

            if (value.Array().IsEmpty())
            {
                writer.StartArray();
                writer.EndArray();
                return;
            }

            // Check if all the values in the array are tables. If so, we can write them as
            // normal table headers with array markers (`[[table]]`). Otherwise we need to
            // write them as inline values (`[1, {b = 2}]`).
            const bool allTables = IsArrayOfTables(value);

            if (!allTables)
            {
                writer.StartArray();
                ++m_arrayDepth;
            }

            for (const TomlValue& element : value.Array())
            {
                m_nextTableIsArray = allTables;
                WriteValue(writer, element);
            }

            if (!allTables)
            {
                --m_arrayDepth;
                writer.EndArray();
            }
        }

        bool IsArrayOfTables(const TomlValue& value) const
        {
            if (!value.IsArray())
                return false;

            bool allTables = true;
            for (const TomlValue& element : value.Array())
            {
                allTables &= element.IsTable();
            }

            return allTables;
        }

    private:
        Vector<StringView> m_keys;
        uint32_t m_arrayDepth{ 0 };
        bool m_nextTableIsArray{ false };
    };

    // --------------------------------------------------------------------------------------------
    TomlDocument::TomlDocument(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {
        m_root.SetTable(allocator);
    }

    TomlReadResult TomlDocument::Read(StringView data)
    {
        m_root.SetTable(m_allocator);

        TomlDocumentReadHandler handler(*this);
        TomlReader reader(m_allocator);
        const TomlReadResult result = reader.Read(data, handler);
        if (!result)
            return result;

        return handler.Result();
    }

    void TomlDocument::Write(String& dst) const
    {
        TomlDocumentWriteHandler handler;
        TomlWriter writer(dst);
        handler.WriteValue(writer, m_root);
    }

    String TomlDocument::ToString() const
    {
        String toml;
        Write(toml);
        return toml;
    }

    // --------------------------------------------------------------------------------------------
    template <>
    const char* AsString(TomlValue::Kind kind)
    {
        switch (kind)
        {
            case TomlValue::Kind::Bool: return "Bool";
            case TomlValue::Kind::Int: return "Int";
            case TomlValue::Kind::Uint: return "Uint";
            case TomlValue::Kind::Float: return "Float";
            case TomlValue::Kind::String: return "String";
            case TomlValue::Kind::DateTime: return "DateTime";
            case TomlValue::Kind::Time: return "Time";
            case TomlValue::Kind::Table: return "Table";
            case TomlValue::Kind::Array: return "Array";
            case TomlValue::Kind::Invalid: return "Invalid";
        }

        return "<unknown>";
    }
}

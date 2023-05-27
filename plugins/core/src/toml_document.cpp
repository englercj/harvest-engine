// Copyright Chad Engler

#include "he/core/toml_document.h"

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    TomlDocument::TomlDocument(Allocator& allocator) noexcept
        : m_allocator(allocator)
    {
        m_root.SetTable(allocator);
    }

    TomlReadResult TomlDocument::Read(StringView data)
    {
        m_root.SetTable(m_allocator);

        TomlDocument::ReadHandler handler(*this);
        TomlReader reader(m_allocator);
        const TomlReadResult result = reader.Read(data, handler);
        if (!result)
            return result;

        return handler.m_result;
    }

    void TomlDocument::Write(String& dst)
    {
        WriteHandler handler;
        TomlWriter writer(dst);
        handler.WriteValue(writer, m_root);
    }

    // --------------------------------------------------------------------------------------------
    TomlDocument::ReadHandler::ReadHandler(TomlDocument& doc) noexcept
        : m_doc(doc)
    {}

    bool TomlDocument::ReadHandler::StartDocument()
    {
        // Nothing to do here
        return true;
    }

    bool TomlDocument::ReadHandler::EndDocument()
    {
        // Nothing to do here
        return true;
    }

    bool TomlDocument::ReadHandler::Comment(StringView value)
    {
        // Nothing to do here
        HE_UNUSED(value);
        return true;
    }

    bool TomlDocument::ReadHandler::Bool(bool value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetBool(value);
        return true;
    }

    bool TomlDocument::ReadHandler::Int(int64_t value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetInt(value);
        return true;
    }

    bool TomlDocument::ReadHandler::Uint(uint64_t value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetUint(value);
        return true;
    }

    bool TomlDocument::ReadHandler::Float(double value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetFloat(value);
        return true;
    }

    bool TomlDocument::ReadHandler::String(StringView value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetString(value);
        return true;
    }

    bool TomlDocument::ReadHandler::DateTime(SystemTime value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetDateTime(value);
        return true;
    }

    bool TomlDocument::ReadHandler::Time(Duration value)
    {
        HE_ASSERT(m_value);
        if (m_value->IsArray())
            m_value->Array().EmplaceBack(value);
        else
            m_value->SetTime(value);
        return true;
    }

    bool TomlDocument::ReadHandler::Table(Span<const he::String> path, bool isArray)
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

    bool TomlDocument::ReadHandler::Key(Span<const he::String> path)
    {
        return WalkPath(path);
    }

    bool TomlDocument::ReadHandler::StartInlineTable()
    {
        HE_ASSERT(m_value);
        m_value->SetTable(m_doc.GetAllocator());
        return true;
    }

    bool TomlDocument::ReadHandler::EndInlineTable(uint32_t length)
    {
        HE_ASSERT(m_value && m_value->IsTable());
        HE_ASSERT(m_value->Table().Size() == length);
        return true;
    }

    bool TomlDocument::ReadHandler::StartArray()
    {
        HE_ASSERT(m_value);
        m_value->SetArray(m_doc.GetAllocator());
        return true;
    }

    bool TomlDocument::ReadHandler::EndArray(uint32_t length)
    {
        HE_ASSERT(m_value && m_value->IsArray());
        HE_ASSERT(m_value->Array().Size() == length);
        return true;
    }

    bool TomlDocument::ReadHandler::WalkPath(Span<const he::String> path)
    {
        m_value = &m_doc.Root();

        for (uint32_t i = 0; i < path.Size(); ++i)
        {
            const he::String& name = path[i];

            if (m_value->IsTable())
            {
                m_value = &m_value->Table()[name];
            }
            else if (m_value->IsArray())
            {
                const uint32_t index = he::String::ToInteger<uint32_t>(name.Begin(), name.End());
                m_value = &m_value->Array()[index];
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

    // --------------------------------------------------------------------------------------------
    void TomlDocument::WriteHandler::WriteValue(TomlWriter& writer, const TomlValue& value)
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

    void TomlDocument::WriteHandler::WriteTable(TomlWriter& writer, const TomlValue& value)
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
            if (it.value.IsTable())
                continue;

            writer.Key(it.key);
            WriteValue(writer, it.value);
        }

        // Next write the table values each as their own header
        for (auto&& it : value.Table())
        {
            if (!it.value.IsTable())
                continue;

            m_keys.PushBack(it.key);
            WriteValue(writer, it.value);
            m_keys.PopBack();
        }

        if (m_arrayDepth > 0)
            writer.EndInlineTable();
    }

    void TomlDocument::WriteHandler::WriteArray(TomlWriter& writer, const TomlValue& value)
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
        bool allTables = true;
        for (const TomlValue& element : value.Array())
        {
            allTables &= element.IsTable();
        }

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

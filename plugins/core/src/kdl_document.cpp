// Copyright Chad Engler

#include "he/core/kdl_document.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    class KdlDocumentReadHandler : public KdlReader::Handler
    {
    public:
        explicit KdlDocumentReadHandler(KdlDocument& doc) noexcept
            : m_doc(doc)
        {}

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

        bool Comment([[maybe_unused]] StringView value) override
        {
            // Nothing to do here
            return true;
        }

        bool StartComment() override
        {
            m_inComment = true;
            return true;
        }

        bool EndComment() override
        {
            m_inComment = false;
            return true;
        }

        bool StartNode(StringView name, StringView type) override
        {
            if (m_inComment)
                return true;

            Vector<KdlNode>& children = m_nodeStack.IsEmpty() ? m_doc.Nodes() : m_nodeStack.Back()->Children();
            KdlNode& node = children.EmplaceBack(m_doc.GetAllocator());
            m_nodeStack.PushBack(&node);
            return true;
        }

        bool EndNode() override
        {
            if (m_nodeStack.IsEmpty()) [[unlikely]]
                return false;

            m_nodeStack.PopBack();
            return true;
        }

        template <typename T>
        bool AddArg(T value, StringView type) noexcept
        {
            if (m_nodeStack.IsEmpty()) [[unlikely]]
                return false;

            if (m_inComment)
                return true;

            KdlNode* node = m_nodeStack.Back();
            node->Arguments().EmplaceBack(value, type);
            return true;
        }

        bool Argument(bool value, StringView type) override { return AddArg(value, type); }
        bool Argument(int64_t value, StringView type) override { return AddArg(value, type); }
        bool Argument(uint64_t value, StringView type) override { return AddArg(value, type); }
        bool Argument(double value, StringView type) override { return AddArg(value, type); }
        bool Argument(StringView value, StringView type) override { return AddArg(value, type); }
        bool Argument(nullptr_t value, StringView type) override { return AddArg(value, type); }

        template <typename T>
        bool AddProp(StringView name, T value, StringView type) noexcept
        {
            if (m_nodeStack.IsEmpty()) [[unlikely]]
                return false;

            if (m_inComment)
                return true;

            KdlNode* node = m_nodeStack.Back();
            node->Properties().Emplace(name, value, type);
            return true;
        }

        bool Property(StringView name, bool value, StringView type) override { return AddProp(name, value, type); }
        bool Property(StringView name, int64_t value, StringView type) override { return AddProp(name, value, type); }
        bool Property(StringView name, uint64_t value, StringView type) override { return AddProp(name, value, type); }
        bool Property(StringView name, double value, StringView type) override { return AddProp(name, value, type); }
        bool Property(StringView name, StringView value, StringView type) override { return AddProp(name, value, type); }
        bool Property(StringView name, nullptr_t value, StringView type) override { return AddProp(name, value, type); }

    private:
        KdlDocument& m_doc;
        Vector<KdlNode*> m_nodeStack{};
        bool m_inComment{ false };
    };
    // --------------------------------------------------------------------------------------------
    static void WriteNode(KdlWriter& writer, const KdlNode& node)
    {
        writer.Node(node.Name(), node.Type());

        for (const KdlValue& value : node.Arguments())
        {
            switch (value.GetKind())
            {
                case KdlValue::Kind::Bool: writer.Argument(value.GetBool(), value.Type()); break;
                case KdlValue::Kind::Int: writer.Argument(value.GetInt(), value.Type()); break;
                case KdlValue::Kind::Uint: writer.Argument(value.GetUint(), value.Type()); break;
                case KdlValue::Kind::Float: writer.Argument(value.GetFloat(), value.Type()); break;
                case KdlValue::Kind::String: writer.Argument(value.GetString(), value.Type()); break;
                case KdlValue::Kind::Null: writer.Argument(nullptr, value.Type()); break;
            }
        }

        for (const auto& entry : node.Properties())
        {
            const String& name = entry.key;
            const KdlValue& value = entry.value;
            switch (value.GetKind())
            {
                case KdlValue::Kind::Bool: writer.Property(name, value.GetBool(), value.Type()); break;
                case KdlValue::Kind::Int: writer.Property(name, value.GetInt(), value.Type()); break;
                case KdlValue::Kind::Uint: writer.Property(name, value.GetUint(), value.Type()); break;
                case KdlValue::Kind::Float: writer.Property(name, value.GetFloat(), value.Type()); break;
                case KdlValue::Kind::String: writer.Property(name, value.GetString(), value.Type()); break;
                case KdlValue::Kind::Null: writer.Property(name, nullptr, value.Type()); break;
            }
        }

        if (!node.Children().IsEmpty())
        {
            writer.StartNodeChildren();
            for (const KdlNode& child : node.Children())
                WriteNode(writer, child);
            writer.EndNodeChildren();
        }
    }

    // --------------------------------------------------------------------------------------------
    KdlDocument::KdlDocument(Allocator& allocator) noexcept
        : m_nodes(allocator)
    {}

    KdlReadResult KdlDocument::Read(StringView data)
    {
        m_nodes.Clear();

        KdlDocumentReadHandler handler(*this);
        KdlReader reader(GetAllocator());
        return reader.Read(data, handler);
    }

    void KdlDocument::Write(String& dst) const
    {
        KdlWriter writer(dst);
        for (const KdlNode& node : m_nodes)
            WriteNode(writer, node);
    }

    String KdlDocument::ToString() const
    {
        String kdl;
        Write(kdl);
        return kdl;
    }

    // --------------------------------------------------------------------------------------------
    template <>
    const char* EnumTraits<KdlValue::Kind>::ToString(KdlValue::Kind x) noexcept
    {
        switch (x)
        {
            case KdlValue::Kind::Bool: return "Bool";
            case KdlValue::Kind::Int: return "Int";
            case KdlValue::Kind::Uint: return "Uint";
            case KdlValue::Kind::Float: return "Float";
            case KdlValue::Kind::String: return "String";
            case KdlValue::Kind::Null: return "Null";
        }

        return "<unknown>";
    }
}

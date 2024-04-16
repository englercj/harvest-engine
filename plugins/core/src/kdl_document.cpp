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
            ++m_commentDepth;
            return true;
        }

        bool EndComment() override
        {
            if (!HE_VERIFY(m_commentDepth > 0))
                return false;

            --m_commentDepth;
            return true;
        }

        bool StartNode(StringView name, const StringView* type) override
        {
            if (m_commentDepth > 0)
                return true;

            Vector<KdlNode>& children = m_nodeStack.IsEmpty() ? m_doc.Nodes() : m_nodeStack.Back()->Children();
            KdlNode& node = children.EmplaceBack(name, m_doc.GetAllocator());
            if (type)
            {
                node.SetType(*type);
            }
            m_nodeStack.PushBack(&node);
            return true;
        }

        bool EndNode() override
        {
            if (m_commentDepth > 0)
                return true;

            if (m_nodeStack.IsEmpty()) [[unlikely]]
                return false;

            m_nodeStack.PopBack();
            return true;
        }

        template <typename T>
        bool AddArg(T value, const StringView* type) noexcept
        {
            if (m_commentDepth > 0)
                return true;

            if (m_nodeStack.IsEmpty()) [[unlikely]]
                return false;

            KdlNode* node = m_nodeStack.Back();
            KdlValue& arg = node->Arguments().EmplaceBack(value);
            if (type)
            {
                arg.SetType(*type);
            }
            return true;
        }

        bool Argument(bool value, const StringView* type) override { return AddArg(value, type); }
        bool Argument(int64_t value, const StringView* type) override { return AddArg(value, type); }
        bool Argument(uint64_t value, const StringView* type) override { return AddArg(value, type); }
        bool Argument(double value, const StringView* type) override { return AddArg(value, type); }
        bool Argument(StringView value, const StringView* type) override { return AddArg(value, type); }
        bool Argument(nullptr_t value, const StringView* type) override { return AddArg(value, type); }

        template <typename T>
        bool AddProp(StringView name, T value, const StringView* type) noexcept
        {
            if (m_commentDepth > 0)
                return true;

            if (m_nodeStack.IsEmpty()) [[unlikely]]
                return false;

            KdlNode* node = m_nodeStack.Back();
            const auto result = node->Properties().EmplaceOrAssign(name, value);
            KdlValue& prop = result.entry.value;
            if (type)
            {
                prop.SetType(*type);
            }
            return true;
        }

        bool Property(StringView name, bool value, const StringView* type) override { return AddProp(name, value, type); }
        bool Property(StringView name, int64_t value, const StringView* type) override { return AddProp(name, value, type); }
        bool Property(StringView name, uint64_t value, const StringView* type) override { return AddProp(name, value, type); }
        bool Property(StringView name, double value, const StringView* type) override { return AddProp(name, value, type); }
        bool Property(StringView name, StringView value, const StringView* type) override { return AddProp(name, value, type); }
        bool Property(StringView name, nullptr_t value, const StringView* type) override { return AddProp(name, value, type); }

    private:
        KdlDocument& m_doc;
        Vector<KdlNode*> m_nodeStack{};
        uint32_t m_commentDepth{ 0 };
    };
    // --------------------------------------------------------------------------------------------
    static void WriteNode(KdlWriter& writer, const KdlNode& node)
    {
        if (node.Type())
        {
            writer.Node(node.Name(), *node.Type());
        }
        else
        {
            writer.Node(node.Name());
        }

        for (const KdlValue& value : node.Arguments())
        {
            StringView* type = nullptr;
            StringView typeStorage;
            if (value.Type())
            {
                typeStorage = *value.Type();
                type = &typeStorage;
            }

            switch (value.GetKind())
            {
                case KdlValue::Kind::Bool: writer.Argument(value.Bool(), type); break;
                case KdlValue::Kind::Int: writer.Argument(value.Int(), type); break;
                case KdlValue::Kind::Uint: writer.Argument(value.Uint(), type); break;
                case KdlValue::Kind::Float: writer.Argument(value.Float(), type); break;
                case KdlValue::Kind::String: writer.Argument(value.String(), type); break;
                case KdlValue::Kind::Null: writer.Argument(nullptr, type); break;
            }
        }

        for (const auto& entry : node.Properties())
        {
            const String& name = entry.key;
            const KdlValue& value = entry.value;

            StringView* type = nullptr;
            StringView typeStorage;
            if (value.Type())
            {
                typeStorage = *value.Type();
                type = &typeStorage;
            }

            switch (value.GetKind())
            {
                case KdlValue::Kind::Bool: writer.Property(name, value.Bool(), type); break;
                case KdlValue::Kind::Int: writer.Property(name, value.Int(), type); break;
                case KdlValue::Kind::Uint: writer.Property(name, value.Uint(), type); break;
                case KdlValue::Kind::Float: writer.Property(name, value.Float(), type); break;
                case KdlValue::Kind::String: writer.Property(name, value.String(), type); break;
                case KdlValue::Kind::Null: writer.Property(name, nullptr, type); break;
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

        // Trailing newline
        if (dst.IsEmpty() || dst.Back() != '\n')
            dst.Append('\n');
    }

    String KdlDocument::ToString() const
    {
        String kdl;
        kdl.Reserve(1024);
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

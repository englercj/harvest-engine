// Copyright Chad Engler

#include "he/schema/parser.h"

#include "utf8_helpers.h"

#include "he/core/appender.h"
#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/enum_fmt.h"
#include "he/core/file.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/core/string_view_fmt.h"
#include "he/core/utils.h"

#include "fmt/format.h"

#include <limits>

namespace he::schema
{
    // Keywords
    constexpr char KW_Alias[] = "alias";
    constexpr char KW_Attribute[] = "attribute";
    constexpr char KW_Const[] = "const";
    constexpr char KW_Enum[] = "enum";
    constexpr char KW_Enumerator[] = "enumerator";
    constexpr char KW_Extends[] = "extends";
    constexpr char KW_False[] = "false";
    constexpr char KW_Field[] = "field";
    constexpr char KW_File[] = "file";
    constexpr char KW_Implements[] = "implements";
    constexpr char KW_Import[] = "import";
    constexpr char KW_Interface[] = "interface";
    constexpr char KW_List[] = "list";
    constexpr char KW_Map[] = "map";
    constexpr char KW_Method[] = "method";
    constexpr char KW_Namespace[] = "namespace";
    constexpr char KW_Parameter[] = "parameter";
    constexpr char KW_Set[] = "set";
    constexpr char KW_Struct[] = "struct";
    constexpr char KW_True[] = "true";
    constexpr char KW_Union[] = "union";

    // Builtin Types
    struct BuiltinType { const StringView name; const BaseType base; };
    constexpr BuiltinType BuiltinTypes[] =
    {
        { "bool", BaseType::Bool },
        { "int8", BaseType::Int8 },
        { "int16", BaseType::Int16 },
        { "int32", BaseType::Int32 },
        { "int64", BaseType::Int64 },
        { "uint8", BaseType::Uint8 },
        { "uint16", BaseType::Uint16 },
        { "uint32", BaseType::Uint32 },
        { "uint64", BaseType::Uint64 },
        { "float", BaseType::Float32 },
        { "double", BaseType::Float64 },
        { "string", BaseType::String },
    };

    // Builtin Attributes
    struct BuiltinAttribute { const StringView name; const AttributeTarget targets; const BaseType param; };
    constexpr BuiltinAttribute BuiltinAttributes[] =
    {
        { "Flags", AttributeTarget::Enum, BaseType::Unknown },
        { "Service", AttributeTarget::Interface, BaseType::Unknown },
    };

    Parser::Parser(Allocator& allocator)
        : m_allocator(allocator)
        , m_schema(allocator)
        , m_lexer(allocator)
        , m_errors(allocator)
        , m_decodedString(allocator)
    {
        // build maps of builtins for faster lookups
        for (const BuiltinAttribute& a : BuiltinAttributes)
        {
            auto result = m_builtinAttributes.emplace(a.name, allocator);
            AttributeDef& def = result.first->second;
            def.name = a.name;
            def.targets = a.targets;
            def.type.base = a.param;
        }

        for (const BuiltinType& t : BuiltinTypes)
        {
            m_builtinTypes[t.name] = t.base;
        }
    }

    bool Parser::ParseFile(const char* path, Span<const char*> includeDirs)
    {
        return ParseFileInternal(path, includeDirs);
    }

    bool Parser::Parse(const char* src, Span<const char*> includeDirs)
    {
        if (!m_lexer.Reset(src))
        {
            AddError(m_lexer.GetErrorText());
            return false;
        }

        NextDecl();

        if (!ParseImports(includeDirs))
            return false;

        if (!ParseNamespace())
            return false;

        while (!AtEnd() && !m_lexer.HasError())
        {
            if (!ParseTopLevelStatement())
            {
                SkipStatement();

                if (At(Lexer::TokenType::Semicolon))
                {
                    AddError("Unmatched curly bracket ('}}')");
                }
            }

            // Too many errors, just bail
            if (m_errors.Size() > 32)
            {
                return false;
            }
        }

        if (m_lexer.HasError())
        {
            AddError(m_lexer.GetErrorText());
        }

        if (HasErrors())
            return false;

        return true;
    }

    bool Parser::ParseFileInternal(const char* path, Span<const char*> includeDirs)
    {
        String contents(m_allocator);
        m_fileName = path;

        if (!LoadFile(contents, path, includeDirs))
            return false;

        return Parse(contents.Data(), includeDirs);
    }

    bool Parser::OpenFile(File& file, const char* path)
    {
        Result r = file.Open(path, FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);

        if (!r && GetFileResult(r) != FileResult::NotFound)
        {
            AddError("Failed to open file: '{}'. Error: {}", path, r);
            return false;
        }

        return true;
    }

    bool Parser::LoadFile(String& dst, const char* path, Span<const char*> includeDirs)
    {
        File file;

        if (IsAbsolutePath(path))
        {
            if (!OpenFile(file, path))
                return false;

            if (file.IsOpen())
                return ReadFile(dst, file, path);

            AddError("No file found at: {}", path);
            return false;
        }

        String fullPath(m_allocator);
        for (const char* dir : includeDirs)
        {
            fullPath = dir;
            ConcatPath(fullPath, path);
            NormalizePath(fullPath);

            if (!OpenFile(file, fullPath.Data()))
                return false;

            if (file.IsOpen())
                return ReadFile(dst, file, fullPath.Data());
        }

        AddError("Failed to find file: {}", fullPath);
        return false;
    }

    bool Parser::ReadFile(String& dst, File& file, const char* path)
    {
        uint64_t fileSize = file.GetSize();

        if (fileSize > String::MaxHeapCharacters)
        {
            AddError("File size is too large ({}) when reading: {}", fileSize, path);
            return false;
        }

        dst.Resize(static_cast<uint32_t>(fileSize));

        uint32_t bytesRead = 0;
        Result r = file.Read(dst.Data(), dst.Size(), &bytesRead);

        if (r)
            return true;

        AddError("Failed to read file: '{}'. Error: {}", path, r);
        return false;
    }

    template <typename T, typename U> struct _ContainerPointer {};
    template <typename U> struct _ContainerPointer<AttributeDef, U> { static constexpr auto P = &U::attributes; };
    template <typename U> struct _ContainerPointer<AliasDef, U> { static constexpr auto P = &U::aliases; };
    template <typename U> struct _ContainerPointer<EnumDef, U> { static constexpr auto P = &U::enums; };
    template <typename U> struct _ContainerPointer<InterfaceDef, U> { static constexpr auto P = &U::interfaces; };
    template <typename U> struct _ContainerPointer<StructDef, U> { static constexpr auto P = &U::structs; };
    template <typename U> struct _ContainerPointer<UnionDef, U> { static constexpr auto P = &U::unions; };

    template <typename T, typename U> inline constexpr bool _CanEmbedType = false;
    template <> inline constexpr bool _CanEmbedType<StructDef, AliasDef> = true;
    template <> inline constexpr bool _CanEmbedType<StructDef, ConstDef> = true;
    template <> inline constexpr bool _CanEmbedType<StructDef, EnumDef> = true;
    template <> inline constexpr bool _CanEmbedType<StructDef, StructDef> = true;
    template <> inline constexpr bool _CanEmbedType<StructDef, UnionDef> = true;
    template <> inline constexpr bool _CanEmbedType<InterfaceDef, AliasDef> = true;
    template <> inline constexpr bool _CanEmbedType<InterfaceDef, ConstDef> = true;
    template <> inline constexpr bool _CanEmbedType<InterfaceDef, EnumDef> = true;
    template <> inline constexpr bool _CanEmbedType<InterfaceDef, StructDef> = true;
    template <> inline constexpr bool _CanEmbedType<InterfaceDef, UnionDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, AliasDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, AttributeDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, ConstDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, EnumDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, InterfaceDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, StructDef> = true;
    template <> inline constexpr bool _CanEmbedType<SchemaDef, UnionDef> = true;

    template <typename T, typename U>
    const T* Parser::FindDef(const U& schema, StringView name) const
    {
        constexpr auto Container = _ContainerPointer<T, U>::P;

        const char* begin = name.Begin();
        const char* dot = &name.Back();
        while (dot > begin && *dot != '.')
            --dot;

        // No dot found, just search for the symbol name
        if (dot == begin)
        {
            for (uint32_t i = 0; i < (schema.*Container).Size(); ++i)
            {
                const T& entry = (schema.*Container)[i];
                if (entry.name == name)
                {
                    return &entry;
                }
            }

            return nullptr;
        }

        // Dot found, this is potentially an embedded type. Let's get the first name and search
        // for that as a struct or interface. If we find it, we'll recurse for the rest.

        // Search within our structs.
        if constexpr (_CanEmbedType<U, StructDef> && _CanEmbedType<StructDef, T>)
        {
            StringView parentName(begin, dot);

            const StructDef* parent = nullptr;
            for (uint32_t i = 0; i < schema.structs.Size(); ++i)
            {
                const StructDef& entry = schema.structs[i];
                if (entry.name == parentName)
                {
                    parent = &entry;
                    break;
                }
            }

            if (parent)
            {
                return FindDef<T, StructDef>(*parent, StringView(parentName.End() + 1, name.End()));
            }
        }

        // Search within our interfaces.
        if constexpr (_CanEmbedType<U, InterfaceDef> && _CanEmbedType<InterfaceDef, T>)
        {
            StringView parentName(begin, dot);

            const InterfaceDef* parent = nullptr;
            for (uint32_t i = 0; i < schema.interfaces.Size(); ++i)
            {
                const InterfaceDef& entry = schema.interfaces[i];
                if (entry.name == parentName)
                {
                    parent = &entry;
                    break;
                }
            }

            if (parent)
            {
                return FindDef<T, InterfaceDef>(*parent, StringView(parentName.End() + 1, name.End()));
            }
        }

        return nullptr;
    }

    template <typename T>
    const T* Parser::FindDef(StringView name) const
    {
        // FQN, perform a forward search treating each part of the name as a namespace
        // until we find a matching definition.
        if (name[0] == '.')
        {
            const char* begin = name.Begin() + 1; // +1 to get past the dot
            const char* end = name.End();
            StringView searchNamespace(begin, begin);
            StringView searchName(begin, end);

            while (searchNamespace.Size() < (name.Size() - 1))
            {
                auto it = m_imports.find(searchNamespace);
                if (it != m_imports.end())
                {
                    for (const Import& im : it->second)
                    {
                        const T* entry = FindDef<T, SchemaDef>(im.schema, searchName);
                        if (entry)
                            return entry;
                    }
                }

                const char* last = &searchNamespace.Back();
                while (last < end && *last != '.')
                    ++last;

                searchNamespace = StringView(begin, last);
                searchName = StringView(searchNamespace.End(), end);
            }

            return nullptr;
        }

        // Relative name, search first in our schema.
        const T* localEntry = FindDef<T, SchemaDef>(m_schema, name);
        if (localEntry)
            return localEntry;

        // Walk up the parts of our namespace and search each of them for the definition.
        StringView searchNamespace = m_schema.namespaceName;
        while (!searchNamespace.IsEmpty())
        {
            auto it = m_imports.find(searchNamespace);
            if (it != m_imports.end())
            {
                for (const Import& im : it->second)
                {
                    const T* entry = FindDef<T, SchemaDef>(im.schema, name);
                    if (entry)
                        return entry;
                }
            }

            const char* begin = searchNamespace.Begin();
            const char* last = &searchNamespace.Back();
            while (last > begin && *last != '.')
                --last;

            searchNamespace = StringView(begin, last);
        }

        return nullptr;
    }

    const AttributeDef* Parser::FindAttributeDef(StringView name) const
    {
        auto it = m_builtinAttributes.find(name);
        if (it != m_builtinAttributes.end())
        {
            return &it->second;
        }

        return FindDef<AttributeDef>(name);
    }

    const AliasDef* Parser::FindAliasDef(StringView name) const
    {
        return FindDef<AliasDef>(name);
    }

    const EnumDef* Parser::FindEnumDef(StringView name) const
    {
        return FindDef<EnumDef>(name);
    }

    const InterfaceDef* Parser::FindInterfaceDef(StringView name) const
    {
        return FindDef<InterfaceDef>(name);
    }

    const StructDef* Parser::FindStructDef(StringView name) const
    {
        return FindDef<StructDef>(name);
    }

    const UnionDef* Parser::FindUnionDef(StringView name) const
    {
        return FindDef<UnionDef>(name);
    }

    const Type& Parser::ResolveType(const Type& type) const
    {
        const Type* result = &type;
        while (result->base == BaseType::Alias)
        {
            const AliasDef* alias = FindAliasDef(type.key->name);
            HE_ASSERT(alias);
            result = &alias->type;
        }

        return *result;
    }

    bool Parser::Expect(Lexer::TokenType expected)
    {
        if (!At(expected))
        {
            AddError("Unexpected token {}, expected {}", m_token.type, expected);
            return false;
        }

        return true;
    }

    void Parser::Next()
    {
        m_token = m_lexer.GetNextToken();
    }

    bool Parser::Next(Lexer::TokenType expected)
    {
        Next();
        return Expect(expected);
    }

    void Parser::NextDecl()
    {
        Next();

        while (At(Lexer::TokenType::Comment) || At(Lexer::TokenType::Whitespace) || At(Lexer::TokenType::Newline))
            Next();
    }

    bool Parser::NextDecl(Lexer::TokenType expected)
    {
        NextDecl();
        return Expect(expected);
    }

    bool Parser::SkipWhitespace(Lexer::TokenType expected)
    {
        m_token = m_lexer.GetNextToken();

        while (At(Lexer::TokenType::Whitespace))
            m_token = m_lexer.GetNextToken();

        return Expect(expected);
    }

    void Parser::SkipStatement()
    {
        while (!AtEnd())
        {
            if (At(Lexer::TokenType::Semicolon))
            {
                NextDecl();
                return;
            }

            if (At(Lexer::TokenType::OpenCurlyBracket))
            {
                SkipBlock();
                return;
            }

            if (At(Lexer::TokenType::CloseCurlyBracket))
            {
                NextDecl();
                return;
            }

            m_token = m_lexer.GetNextToken();
        }
    }

    void Parser::SkipBlock()
    {
        while (!AtEnd() && !At(Lexer::TokenType::CloseCurlyBracket))
            m_token = m_lexer.GetNextToken();

        if (At(Lexer::TokenType::CloseCurlyBracket))
        {
            m_token = m_lexer.GetNextToken();
            NextDecl();
        }
    }

    bool Parser::TryConsume(Lexer::TokenType expected)
    {
        if (!At(expected))
            return false;

        NextDecl();
        return true;
    }

    bool Parser::TryConsumeKeyword(StringView expected)
    {
        if (!At(Lexer::TokenType::Identifier))
            return false;

        if (m_token.text != expected)
            return false;

        NextDecl();
        return true;
    }

    bool Parser::Consume(Lexer::TokenType expected)
    {
        if (!Expect(expected))
            return false;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeAttribute(Attribute& attribute)
    {
        if (!ConsumeDottedIdentifier(attribute.name))
            return false;

        const AttributeDef* def = FindAttributeDef(attribute.name);
        if (!def)
        {
            AddError("Unknown attribute \"{}\", was it declared?", attribute.name);
            return false;
        }

        if (TryConsume(Lexer::TokenType::OpenParens))
        {
            if (def->type.base == BaseType::Unknown)
            {
                AddError("Cannot pass a parameter to an attribute with no type specified.");
                return false;
            }

            const Type& resolved = ResolveType(def->type);

            if (!ConsumeValue(resolved.base, attribute.parameters.EmplaceBack(m_allocator)))
                return false;

            while (TryConsume(Lexer::TokenType::Comma))
            {
                if (!ConsumeValue(resolved.base, attribute.parameters.EmplaceBack(m_allocator)))
                    return false;
            }

            if (!Consume(Lexer::TokenType::CloseParens))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeAttributes(Vector<Attribute>& attributes)
    {
        while (TryConsume(Lexer::TokenType::OpenSquareBracket))
        {
            if (!ConsumeAttribute(attributes.EmplaceBack(m_allocator)))
                return false;

            if (!Consume(Lexer::TokenType::CloseSquareBracket))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeKeyword(StringView expected)
    {
        if (!Expect(Lexer::TokenType::Identifier))
            return false;

        if (m_token.text != expected)
        {
            AddError("Expected \"{}\" but got \"{}\"", expected, m_token.text);
            return false;
        }

        NextDecl();
        return true;
    }

    bool Parser::ConsumeIdentifier(String& out)
    {
        if (!Expect(Lexer::TokenType::Identifier))
            return false;

        out += m_token.text;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeDottedIdentifier(String& out)
    {
        if (TryConsume(Lexer::TokenType::Dot))
            out += '.';

        if (!ConsumeIdentifier(out))
            return false;

        while (TryConsume(Lexer::TokenType::Dot))
        {
            out += '.';
            if (!ConsumeIdentifier(out))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeBool(bool& out)
    {
        if (TryConsumeKeyword(KW_True))
        {
            out = true;
            return true;
        }

        if (TryConsumeKeyword(KW_False))
        {
            out = false;
            return true;
        }

        AddError("Expected true or false for boolean value");
        return false;
    }

    template <std::integral T>
    bool Parser::ConsumeIntegerRaw(T& out)
    {
        if (!Expect(Lexer::TokenType::Integer))
            return false;

        const char* begin = m_token.text.begin();
        const char* end = m_token.text.end();

        int32_t base = 10;
        if (*begin == '0')
        {
            ++begin;
            switch (*begin)
            {
                case 'x':
                case 'X':
                    ++begin;
                    base = 16;
                    break;
                case 'b':
                case 'B':
                    ++begin;
                    base = 2;
                    break;
                default:
                    base = 8;
                    break;
            }
        }

        out = String::ToInteger<T>(begin, &end, base);

        NextDecl();
        return true;
    }

    template <std::integral T>
    bool Parser::ConsumeInteger(T& out)
    {
        bool isSigned = TryConsume(Lexer::TokenType::Minus);

        std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t> value = 0;
        if (!ConsumeIntegerRaw(value))
            return false;

        if (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
            AddError("Integer value out of range");
            return false;
        }

        out = static_cast<T>(value);

        if (isSigned)
        {
            if constexpr (std::is_signed_v<T>)
            {
                out *= -1;
            }
            else
            {
                // Disallow assignment of a signed value to an unsigned type, except for the
                // special case of allowing `-1` which is a common "set all bits" value.
                if (out == 1)
                {
                    out = static_cast<T>(-1);
                }
                else
                {
                    AddError("Illegal negative value assigned to unsigned integer type");
                    return false;
                }
            }
        }

        return true;
    }

    bool Parser::ConsumeFloatRaw(double& out)
    {
        if (!Expect(Lexer::TokenType::Float))
            return false;

        out = m_token.text.ToFloat<double>();

        NextDecl();
        return true;
    }

    template <std::floating_point T>
    bool Parser::ConsumeFloat(T& out)
    {
        bool isSigned = TryConsume(Lexer::TokenType::Minus);

        double value = 0;
        if (!ConsumeFloatRaw(value))
            return false;

        if (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
            AddError("Float value out of range");
            return false;
        }

        out = static_cast<T>(value);

        if (isSigned)
            out *= -1;

        return true;
    }

    bool Parser::ConsumeString(String& out)
    {
        if (!Expect(Lexer::TokenType::String))
            return false;

        if (!DecodeString())
            return false;

        out = m_decodedString;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeTypeRaw(Type& type)
    {
        // Built-in type
        if (At(Lexer::TokenType::Identifier))
        {
            auto it = m_builtinTypes.find(m_token.text);
            if (it != m_builtinTypes.end())
            {
                m_schema.MarkTypeUsed(it->second);
                type.base = it->second;
                NextDecl();
                return true;
            }
        }

        // list<T>
        if (TryConsumeKeyword(KW_List))
        {
            m_schema.MarkTypeUsed(BaseType::List);
            type.base = BaseType::List;
            type.element = m_allocator.New<Type>(m_allocator);

            if (!Consume(Lexer::TokenType::OpenAngleBracket))
                return false;

            if (!ConsumeType(*type.element))
                return false;

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;

            return true;
        }

        // set<T>
        if (TryConsumeKeyword(KW_Set))
        {
            m_schema.MarkTypeUsed(BaseType::Set);
            type.base = BaseType::Set;
            type.element = m_allocator.New<Type>(m_allocator);

            if (!Consume(Lexer::TokenType::OpenAngleBracket))
                return false;

            if (!ConsumeType(*type.element))
                return false;

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;

            return true;
        }

        // map<K, T>
        if (TryConsumeKeyword(KW_Map))
        {
            m_schema.MarkTypeUsed(BaseType::Map);
            type.base = BaseType::Map;
            type.key = m_allocator.New<Type>(m_allocator);
            type.element = m_allocator.New<Type>(m_allocator);

            if (!Consume(Lexer::TokenType::OpenAngleBracket))
                return false;

            if (!ConsumeType(*type.key))
                return false;

            const Type& keyType = ResolveType(*type.key);
            if (IsArithmetic(keyType.base))
            {
                AddError("Invalid key for map, must be an arithmetic type.");
                return false;
            }

            if (!Consume(Lexer::TokenType::Comma))
                return false;

            if (!ConsumeType(*type.element))
                return false;

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;

            return true;
        }

        // User defined type
        type.base = BaseType::Unknown;

        if (!ConsumeDottedIdentifier(type.name))
            return false;

        // These checks are ordered based on likelyhood. For example a type lookup is more likely
        // to be a struct than an interface, simply because there are often more structs than
        // anything else.
        if (FindStructDef(type.name) != nullptr)
            type.base = BaseType::Struct;
        else if (FindEnumDef(type.name) != nullptr)
            type.base = BaseType::Enum;
        else if (FindAliasDef(type.name) != nullptr)
            type.base = BaseType::Alias;
        else if (FindInterfaceDef(type.name) != nullptr)
            type.base = BaseType::Interface;
        else if (FindUnionDef(type.name) != nullptr)
            type.base = BaseType::Union;

        if (type.base == BaseType::Unknown)
        {
            AddError("Encountered unknown type name {}", type.name);
            return false;
        }

        m_schema.MarkTypeUsed(type.base);

        if (TryConsume(Lexer::TokenType::OpenAngleBracket))
        {
            if (type.base != BaseType::Alias && type.base != BaseType::Interface && type.base != BaseType::Struct)
            {
                AddError("Only alias, interface, and struct types can have generic parameters");
                return false;
            }

            if (!ConsumeType(type.AddTypeParam()))
                return false;

            while (TryConsume(Lexer::TokenType::Comma))
            {
                if (!ConsumeType(type.AddTypeParam()))
                    return false;
            }

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeTypeParams(Vector<String>& params)
    {
        if (TryConsume(Lexer::TokenType::OpenAngleBracket))
        {
            if (!ConsumeIdentifier(params.EmplaceBack(m_allocator)))
                return false;

            while (TryConsume(Lexer::TokenType::Comma))
            {
                if (!ConsumeIdentifier(params.EmplaceBack(m_allocator)))
                    return false;
            }

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;
        }

        return true;
    }

    bool Parser::ConsumeType(Type& type)
    {
        if (!ConsumeTypeRaw(type))
            return false;

        while (TryConsume(Lexer::TokenType::OpenSquareBracket))
        {
            Type* element = m_allocator.New<Type>(m_allocator);
            *element = Move(type);

            type.base = BaseType::Vector;
            type.fixedSize = 0;
            type.name.Clear();
            type.element = element;

            if (m_token.type == Lexer::TokenType::Integer)
            {
                type.base = BaseType::Array;
                if (!ConsumeInteger(type.fixedSize))
                    return false;
            }
            else if (m_token.type == Lexer::TokenType::Identifier || m_token.type == Lexer::TokenType::Dot)
            {
                type.base = BaseType::Array;
                if (!ConsumeDottedIdentifier(type.name))
                    return false;
            }

            if (!Consume(Lexer::TokenType::CloseSquareBracket))
                return false;

            m_schema.MarkTypeUsed(type.base);
        }

        return true;
    }

    bool Parser::ConsumeValue(BaseType type, Value& value)
    {
        switch (type)
        {
            case BaseType::Bool:
                return ConsumeBool(value.basic.b);
            case BaseType::Int8:
                return ConsumeInteger(value.basic.i8);
            case BaseType::Int16:
                return ConsumeInteger(value.basic.i16);
            case BaseType::Int32:
                return ConsumeInteger(value.basic.i32);
            case BaseType::Int64:
                return ConsumeInteger(value.basic.i64);
            case BaseType::Uint8:
                return ConsumeInteger(value.basic.u8);
            case BaseType::Uint16:
                return ConsumeInteger(value.basic.u16);
            case BaseType::Uint32:
                return ConsumeInteger(value.basic.u32);
            case BaseType::Uint64:
                return ConsumeInteger(value.basic.u64);
            case BaseType::Float32:
                return ConsumeFloat(value.basic.f32);
            case BaseType::Float64:
                return ConsumeFloat(value.basic.f64);
            case BaseType::String:
                value.str.Clear();
                return ConsumeString(value.str);
            case BaseType::Interface:
                AddError("Interface types cannot have a default value");
                return false;
            case BaseType::Alias:
                AddError("Type alias should've been resolved before consuming value. This is a he_schema bug.");
                return false;
            case BaseType::Array:
            case BaseType::List:
            case BaseType::Map:
            case BaseType::Set:
            case BaseType::Vector:
            case BaseType::Enum:
            case BaseType::Struct:
                value.str.Clear();
                while (m_token.type != Lexer::TokenType::Semicolon)
                {
                    value.str += m_token.text;
                    Next();
                }
                return true;
            case BaseType::Unknown:
                AddError("Value specified for an undefined type.");
                return false;
        }

        HE_ASSERT(false, "Type is not a known enum value");
        AddError("Tried to consume a value for an invalid type, this should never happen");
        return false;
    }

    bool Parser::ParseImports(Span<const char*> includeDirs)
    {
        ++m_importDepth;
        HE_AT_SCOPE_EXIT([&]() { --m_importDepth; });

        if (m_importDepth > 256)
        {
            AddError("Import depth too deep, do you have an import cycle?");
            return false;
        }

        Vector<String> directImports(m_allocator);

        String importPath(m_allocator);
        while (TryConsumeKeyword(KW_Import))
        {
            if (!ConsumeString(importPath))
                return false;

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;

            Lexer tempLexer(Move(m_lexer));
            Lexer::Token tempToken(Move(m_token));
            StringView tempFileName = m_fileName;

            if (!ParseFileInternal(importPath.Data(), includeDirs))
                return false;

            m_lexer = Move(tempLexer);
            m_token = Move(tempToken);
            m_fileName = tempFileName;

            // Move the parsed schema into the imports list
            auto result = m_imports.try_emplace(m_schema.namespaceName, m_allocator);
            Vector<Import>& imports = result.first->second;
            Import& im = imports.EmplaceBack(m_allocator);
            im.directImport = m_importDepth == 1;
            im.importPath = Move(importPath);
            im.schema = Move(m_schema);

            if (im.directImport)
                directImports.PushBack(im.importPath);

            // reset the local schema
            m_schema = SchemaDef(m_allocator);
        }

        if (m_importDepth == 1)
        {
            m_schema.imports = Move(directImports);
        }

        return true;
    }

    bool Parser::ParseNamespace()
    {
        if (TryConsumeKeyword(KW_Namespace))
        {
            if (!ConsumeDottedIdentifier(m_schema.namespaceName))
                return false;

            if (m_schema.namespaceName.IsEmpty())
            {
                AddError("Namespace identifier cannot be empty");
                return false;
            }

            if (m_schema.namespaceName[0] == '.')
            {
                AddError("Namespace identifier cannot start with a dot");
                return false;
            }

            m_schema.namespaceId = FNV32::HashString(m_schema.namespaceName.Data());
            if (m_schema.namespaceId == 0)
            {
                AddError("Namespace '{}' hashes to zero (0), which is reserved; try a different name", m_schema.namespaceName);
                return false;
            }

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;
        }

        return true;
    }

    bool Parser::ParseTopLevelStatement()
    {
        // empty statement, ignore it
        if (TryConsume(Lexer::TokenType::Semicolon))
            return true;

        Vector<Attribute> attributes(m_allocator);
        if (!ConsumeAttributes(attributes))
            return false;

        if (AtIdentifier(KW_Struct))
        {
            m_schema.objects.PushBack({ ObjectDef::Type::Struct, m_schema.structs.Size() });
            StructDef& def = m_schema.structs.EmplaceBack(m_allocator);
            def.attributes = Move(attributes);
            return ParseStruct(def, m_schema.namespaceId);
        }

        if (AtIdentifier(KW_Attribute))
        {
            if (!attributes.IsEmpty())
            {
                AddError("Attributes cannot be tagged with attributes");
                // we can continue to parse, but we'll ignore these attributes.
            }

            m_schema.objects.PushBack({ ObjectDef::Type::Attribute, m_schema.attributes.Size() });
            AttributeDef& def = m_schema.attributes.EmplaceBack(m_allocator);
            return ParseAttribute(def);
        }

        if (AtIdentifier(KW_Enum))
        {
            m_schema.objects.PushBack({ ObjectDef::Type::Enum, m_schema.enums.Size() });
            EnumDef& def = m_schema.enums.EmplaceBack(m_allocator);
            def.attributes = Move(attributes);
            return ParseEnum(def);
        }

        if (AtIdentifier(KW_Interface))
        {
            m_schema.objects.PushBack({ ObjectDef::Type::Interface, m_schema.interfaces.Size() });
            InterfaceDef& def = m_schema.interfaces.EmplaceBack(m_allocator);
            def.attributes = Move(attributes);
            return ParseInterface(def);
        }

        if (AtIdentifier(KW_Alias))
        {
            m_schema.objects.PushBack({ ObjectDef::Type::Alias, m_schema.aliases.Size() });
            AliasDef& def = m_schema.aliases.EmplaceBack(m_allocator);
            def.attributes = Move(attributes);
            return ParseAlias(def);
        }

        if (AtIdentifier(KW_Const))
        {
            m_schema.objects.PushBack({ ObjectDef::Type::Const, m_schema.consts.Size() });
            ConstDef& def = m_schema.consts.EmplaceBack(m_allocator);
            def.attributes = Move(attributes);
            return ParseConst(def);
        }

        if (AtIdentifier(KW_Union))
        {
            m_schema.objects.PushBack({ ObjectDef::Type::Union, m_schema.unions.Size() });
            UnionDef& def = m_schema.unions.EmplaceBack(m_allocator);
            def.attributes = Move(attributes);
            return ParseUnion(def, m_schema.namespaceId);
        }

        if (AtIdentifier(KW_Import))
        {
            AddError("Imports must be the first statements in a file");
            return false;
        }

        if (AtIdentifier(KW_Namespace))
        {
            AddError("There can only be a single namespace statement, just after the imports, in a file");
            return false;
        }

        AddError("Expected top level statement (alias, attribute, const, enum, interface, struct)");
        return false;
    }

    bool Parser::ParseStruct(StructDef& def, uint32_t parentId)
    {
        if (!ConsumeKeyword(KW_Struct))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        def.id = FNV32::HashString(def.name.Data(), parentId);
        if (def.id == 0)
        {
            AddError("Struct '{}' has a fully qualified name that hashes to zero (0), which is reserved; try a different name", def.name);
            return false;
        }

        if (!ConsumeTypeParams(def.typeParams))
            return false;

        if (TryConsumeKeyword(KW_Extends))
        {
            if (!ConsumeType(def.extends))
                return false;

            const Type& extends = ResolveType(def.extends);
            if (extends.base != BaseType::Struct)
            {
                AddError("Expected struct name for extends value, but got {}", extends.base);
                return false;
            }
        }

        return ParseStructBlock(def);
    }

    bool Parser::ParseStructBlock(StructDef& def)
    {
        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in struct definition (missing '}}')");
                return false;
            }

            if (!ParseStructStatement(def))
            {
                SkipStatement();
            }
        }

        // Check for hash collision in struct field names.
        const uint32_t size = def.fields.Size();
        for (uint32_t i = 0; i < size; ++i)
        {
            for (uint32_t j = i + 1; j < size; ++j)
            {
                if (def.fields[i].id == def.fields[j].id)
                {
                    if (def.fields[i].name == def.fields[j].name)
                        AddError("Two fields in struct '{}' have the name '{}'", def.name, def.fields[i].name);
                    else
                        AddError("Fields '{}' and '{}' in struct '{}' have names that generate a hash collision. Try renaming one of them.",
                            def.fields[i].name, def.fields[j].name, def.name);
                    return false;
                }
            }
        }

        return true;
    }

    bool Parser::ParseStructStatement(StructDef& def)
    {
        // ignore empty statements
        if (TryConsume(Lexer::TokenType::Semicolon))
            return true;

        Vector<Attribute> attributes(m_allocator);
        if (!ConsumeAttributes(attributes))
            return false;

        if (AtIdentifier(KW_Alias))
        {
            def.objects.PushBack({ ObjectDef::Type::Alias, def.aliases.Size() });
            AliasDef& d = def.aliases.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseAlias(d);
        }

        if (AtIdentifier(KW_Const))
        {
            def.objects.PushBack({ ObjectDef::Type::Const, def.consts.Size() });
            ConstDef& d = def.consts.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseConst(d);
        }

        if (AtIdentifier(KW_Enum))
        {
            def.objects.PushBack({ ObjectDef::Type::Enum, def.enums.Size() });
            EnumDef& d = def.enums.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseEnum(d);
        }

        if (AtIdentifier(KW_Struct))
        {
            def.objects.PushBack({ ObjectDef::Type::Struct, def.structs.Size() });
            StructDef& d = def.structs.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseStruct(d, def.id);
        }

        if (AtIdentifier(KW_Union))
        {
            def.objects.PushBack({ ObjectDef::Type::Union, def.unions.Size() });
            UnionDef& d = def.unions.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseUnion(d, def.id);
        }

        FieldDef& d = def.fields.EmplaceBack(m_allocator);
        d.attributes = Move(attributes);
        return ParseField(d);
    }

    bool Parser::ParseUnion(UnionDef& def, uint32_t parentId)
    {
        if (!ConsumeKeyword(KW_Union))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        def.id = FNV32::HashString(def.name.Data(), parentId);
        if (def.id == 0)
        {
            AddError("Struct '{}' has a fully qualified name that hashes to zero (0), which is reserved; try a different name", def.name);
            return false;
        }

        return ParseUnionBlock(def);
    }

    bool Parser::ParseUnionBlock(UnionDef& def)
    {
        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in union definition (missing '}}')");
                return false;
            }

            if (!ParseUnionStatement(def))
            {
                SkipStatement();
            }
        }

        // Check for hash collision in union field names.
        const uint32_t size = def.fields.Size();
        for (uint32_t i = 0; i < size; ++i)
        {
            for (uint32_t j = i + 1; j < size; ++j)
            {
                if (def.fields[i].id == def.fields[j].id)
                {
                    if (def.fields[i].name == def.fields[j].name)
                        AddError("Two fields in union '{}' have the name '{}'", def.name, def.fields[i].name);
                    else
                        AddError("Fields '{}' and '{}' in union '{}' have names that generate a hash collision; try renaming one of them",
                            def.fields[i].name, def.fields[j].name, def.name);
                    return false;
                }
            }
        }

        return true;
    }

    bool Parser::ParseUnionStatement(UnionDef& def)
    {
        // ignore empty statements
        if (TryConsume(Lexer::TokenType::Semicolon))
            return true;

        Vector<Attribute> attributes(m_allocator);
        if (!ConsumeAttributes(attributes))
            return false;

        FieldDef& d = def.fields.EmplaceBack(m_allocator);
        d.attributes = Move(attributes);
        if (!ParseField(d))
            return false;

        const Type& resolved = ResolveType(d.type);
        if (!IsArithmetic(resolved.base))
        {
            AddError("Field '{}' in union '{}' has a non-arithmetic type", d.name, def.name);
            return false;
        }

        return true;
    }

    bool Parser::ParseField(FieldDef& def)
    {
        if (!ConsumeIdentifier(def.name))
            return false;

        def.id = FNV32::HashString(def.name.Data());
        if (def.id == 0)
        {
            AddError("Field '{}' in struct '{}' has a name that hashes to zero (0), which is reserved; try a different name", def.name);
            return false;
        }

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        if (!ConsumeType(def.type))
            return false;

        const Type& resolved = ResolveType(def.type);

        if (resolved.base == BaseType::Interface)
        {
            AddError("Fields cannot use interfaces as a type", def.name);
            return false;
        }

        if (TryConsume(Lexer::TokenType::Equals))
        {
            if (ConsumeValue(resolved.base, def.defaultValue))
                return false;
        }

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ParseAttribute(AttributeDef& def)
    {
        if (!ConsumeKeyword(KW_Attribute))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        if (TryConsume(Lexer::TokenType::OpenParens))
        {
            if (!At(Lexer::TokenType::CloseParens))
            {
                if (!ParseAttributeTarget(def))
                    return false;

                while (TryConsume(Lexer::TokenType::Comma))
                {
                    if (!ParseAttributeTarget(def))
                        return false;
                }
            }
            else
            {
                def.targets = AttributeTarget::All;
            }

            if (!Consume(Lexer::TokenType::CloseParens))
                return false;
        }
        else
        {
            def.targets = AttributeTarget::All;
        }

        if (TryConsume(Lexer::TokenType::Colon))
        {
            if (!ConsumeType(def.type))
                return false;
        }

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ParseAttributeTarget(AttributeDef& def)
    {
        String target(m_allocator);
        if (!ConsumeIdentifier(target))
            return false;

        if (target == KW_Const)
            def.targets |= AttributeTarget::Const;
        else if (target == KW_Enum)
            def.targets |= AttributeTarget::Enum;
        else if (target == KW_Enumerator)
            def.targets |= AttributeTarget::EnumValue;
        else if (target == KW_Field)
            def.targets |= AttributeTarget::Field;
        else if (target == KW_File)
            def.targets |= AttributeTarget::File;
        else if (target == KW_Interface)
            def.targets |= AttributeTarget::Interface;
        else if (target == KW_Method)
            def.targets |= AttributeTarget::Method;
        else if (target == KW_Parameter)
            def.targets |= AttributeTarget::Parameter;
        else if (target == KW_Struct)
            def.targets |= AttributeTarget::Struct;
        else
        {
            AddError("Expected a valid attribute target: const, enum, enumerator, field, file, interface, method, parameter, or struct");
            return false;
        }

        return true;
    }

    bool Parser::ParseEnum(EnumDef& def)
    {
        const bool isFlags = HasAttribute("Flags", def.attributes);

        if (!ConsumeKeyword(KW_Enum))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        def.base = isFlags ? BaseType::Uint32 : BaseType::Int32;

        if (TryConsume(Lexer::TokenType::Colon))
        {
            Type enumType(m_allocator);
            if (!ConsumeType(enumType))
                return false;

            const Type& resolved = ResolveType(enumType);

            if (isFlags && (resolved.base == BaseType::Bool || !IsUnsignedIntegral(resolved.base)))
            {
                AddError("Expected unsigned integral type for enum flags");
                return false;
            }
            else if (resolved.base == BaseType::Bool || !IsIntegral(resolved.base))
            {
                AddError("Expected integral type for enum");
                return false;
            }

            def.base = resolved.base;
        }

        return ParseEnumBlock(def, isFlags);
    }

    bool Parser::ParseEnumBlock(EnumDef& def, bool isFlags)
    {
        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in enum definition (missing '}}')");
                return false;
            }

            EnumValueDef& newValueDef = def.values.EmplaceBack(m_allocator);
            EnumValueDef* lastValueDef = def.values.Size() > 1 ? &def.values[def.values.Size() - 2] : nullptr;
            if (!ParseEnumStatement(def, isFlags, newValueDef, lastValueDef))
                SkipStatement();
        }

        return true;
    }

    bool Parser::ParseEnumStatement(const EnumDef& enumDef, bool isFlags, EnumValueDef& def, EnumValueDef* lastDef)
    {
        if (!ConsumeAttributes(def.attributes))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        if (TryConsume(Lexer::TokenType::Equals))
        {
            if (!ConsumeValue(enumDef.base, def.value))
                return false;
        }
        else if (lastDef == nullptr)
        {
            def.value.basic.u64 = isFlags ? 1 : 0;
        }
        else if (isFlags)
        {
            def.value.basic.u64 = lastDef->value.basic.u64 * 2;
        }
        else if (IsUnsignedIntegral(enumDef.base))
        {
            def.value.basic.u64 = lastDef->value.basic.u64 + 1;
        }
        else
        {
            def.value.basic.i64 = lastDef->value.basic.i64 + 1;
        }

        return Consume(Lexer::TokenType::Comma);
    }

    bool Parser::ParseInterface(InterfaceDef& def)
    {
        if (!ConsumeKeyword(KW_Interface))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        def.id = FNV32::HashString(def.name.Data(), m_schema.namespaceId);
        if (def.id == 0)
        {
            AddError("Interface '{}' has a fully qualified name that hashes to zero (0), which is reserved; try a different name", def.name);
            return false;
        }

        if (!ConsumeTypeParams(def.typeParams))
            return false;

        if (TryConsumeKeyword(KW_Implements))
        {
            do
            {
                if (!ConsumeType(def.implements.EmplaceBack(m_allocator)))
                    return false;

                const Type& resolved = ResolveType(def.implements.Back());

                if (resolved.base != BaseType::Interface)
                {
                    AddError("Expected interface name for implements value, but got {}", resolved.base);
                    return false;
                }
            } while (TryConsume(Lexer::TokenType::Comma));
        }

        return ParseInterfaceBlock(def);
    }

    bool Parser::ParseInterfaceBlock(InterfaceDef& def)
    {
        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in struct definition (missing '}}')");
                return false;
            }

            if (!ParseInterfaceStatement(def))
            {
                SkipStatement();
            }
        }

        // Check for hash collision in interface method names.
        const uint32_t size = def.methods.Size();
        for (uint32_t i = 0; i < size; ++i)
        {
            for (uint32_t j = i + 1; j < size; ++j)
            {
                if (def.methods[i].id == def.methods[j].id)
                {
                    if (def.methods[i].name == def.methods[j].name)
                        AddError("Two methods in interface '{}' have the name '{}'", def.name, def.methods[i].name);
                    else
                        AddError("Methods '{}' and '{}' in interface '{}' have names that generate a hash collision. Try renaming one of them.",
                            def.methods[i].name, def.methods[j].name, def.name);
                    return false;
                }
            }
        }

        return true;
    }

    bool Parser::ParseInterfaceStatement(InterfaceDef& def)
    {
        // ignore empty statements
        if (TryConsume(Lexer::TokenType::Semicolon))
            return true;

        Vector<Attribute> attributes(m_allocator);
        if (!ConsumeAttributes(attributes))
            return false;

        if (AtIdentifier(KW_Alias))
        {
            def.objects.PushBack({ ObjectDef::Type::Alias, def.aliases.Size() });
            AliasDef& d = def.aliases.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseAlias(d);
        }

        if (AtIdentifier(KW_Const))
        {
            def.objects.PushBack({ ObjectDef::Type::Const, def.consts.Size() });
            ConstDef& d = def.consts.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseConst(d);
        }

        if (AtIdentifier(KW_Enum))
        {
            def.objects.PushBack({ ObjectDef::Type::Enum, def.enums.Size() });
            EnumDef& d = def.enums.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseEnum(d);
        }

        if (AtIdentifier(KW_Struct))
        {
            def.objects.PushBack({ ObjectDef::Type::Struct, def.structs.Size() });
            StructDef& d = def.structs.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseStruct(d, def.id);
        }

        if (AtIdentifier(KW_Union))
        {
            def.objects.PushBack({ ObjectDef::Type::Union, def.unions.Size() });
            UnionDef& d = def.unions.EmplaceBack(m_allocator);
            d.attributes = Move(attributes);
            return ParseUnion(d, def.id);
        }

        MethodDef& d = def.methods.EmplaceBack(m_allocator);
        d.attributes = Move(attributes);
        return ParseInterfaceMethod(d);
    }

    bool Parser::ParseInterfaceMethod(MethodDef& def)
    {
        if (!ConsumeIdentifier(def.name))
            return false;

        def.id = FNV32::HashString(def.name.Data());
        if (def.id == 0)
        {
            AddError("Method '{}' in interface '{}' has a name that hashes to zero (0), which is reserved; try a different name", def.name);
            return false;
        }

        if (!Consume(Lexer::TokenType::OpenParens))
            return false;

        if (!TryConsume(Lexer::TokenType::CloseParens))
        {
            if (!ParseInterfaceMethodParam(def.parameters.EmplaceBack(m_allocator)))
                return false;

            while (TryConsume(Lexer::TokenType::Comma))
            {
                if (!ParseInterfaceMethodParam(def.parameters.EmplaceBack(m_allocator)))
                    return false;
            }

            if (!Consume(Lexer::TokenType::CloseParens))
                return false;
        }

        if (!Consume(Lexer::TokenType::Arrow))
            return false;

        if (!ConsumeType(def.returnType))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ParseInterfaceMethodParam(MethodParamDef& def)
    {
        if (!ConsumeAttributes(def.attributes))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        return ConsumeType(def.type);
    }

    bool Parser::ParseAlias(AliasDef& def)
    {
        if (!ConsumeKeyword(KW_Alias))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        if (!ConsumeTypeParams(def.typeParams))
            return false;

        if (!Consume(Lexer::TokenType::Equals))
            return false;

        if (!ConsumeType(def.type))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ParseConst(ConstDef& def)
    {
        if (!ConsumeKeyword(KW_Const))
            return false;

        if (!ConsumeIdentifier(def.name))
            return false;

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        Type constType(m_allocator);
        if (!ConsumeType(constType))
            return false;

        const Type& resolved = ResolveType(constType);

        if (!IsArithmetic(resolved.base) && resolved.base != BaseType::String)
        {
            AddError("Expected basic type for constant: integral, float, or string");
            return false;
        }

        def.base = resolved.base;

        if (!Consume(Lexer::TokenType::Equals))
            return false;

        if (!ConsumeValue(def.base, def.value))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::At(Lexer::TokenType expected) const
    {
        return m_token.type == expected;
    }

    bool Parser::AtEnd() const
    {
        return At(Lexer::TokenType::Eof);
    }

    bool Parser::AtIdentifier(StringView expected) const
    {
        return At(Lexer::TokenType::Identifier) && m_token.text == expected;
    }

    template <typename... Args>
    void Parser::AddError(fmt::format_string<Args...> fmt, Args&&... args)
    {
        ErrorInfo& entry = m_errors.EmplaceBack(m_allocator);
        entry.file.Assign(m_fileName.Data(), m_fileName.Size());
        entry.line = m_token.line;
        entry.column = m_token.column;
        entry.message.Clear();

        fmt::format_to(Appender(entry.message), fmt, Forward<Args>(args)...);
    }

    bool Parser::DecodeString()
    {
        m_decodedString.Clear();

        bool decodedIsTrivial = true;
        int unicodeHighSurrogate = -1;

        const char* s = m_token.text.begin();

        while (s < m_token.text.end())
        {
            char c = *s++;

            switch (c)
            {
                case '\"':
                    // Ignore surrounding double quotes.
                    break;
                case '\\':
                    decodedIsTrivial = false;
                    s++;

                    if (unicodeHighSurrogate != -1 && *s != 'u')
                    {
                        AddError("Illegal Unicode sequence (unpaired high surrogate)");
                        return false;
                    }

                    switch (*s)
                    {
                        case '\'': // single quote
                            m_decodedString += '\'';
                            s++;
                            break;
                        case '\"': // double quote
                            m_decodedString += '\"';
                            s++;
                            break;
                        case '\\': // backslash
                            m_decodedString += '\\';
                            s++;
                            break;
                        case 'b': // backspace
                            m_decodedString += '\b';
                            s++;
                            break;
                        case 'f': // form feed - new page
                            m_decodedString += '\f';
                            s++;
                            break;
                        case 'n': // line feed - new line
                            m_decodedString += '\n';
                            s++;
                            break;
                        case 'r': // carriage return
                            m_decodedString += '\r';
                            s++;
                            break;
                        case 't': // horizontal tab
                            m_decodedString += '\t';
                            s++;
                            break;
                        case 'v': // vertical tab
                            m_decodedString += '\v';
                            s++;
                            break;
                        case 'x': // hex literal
                        case 'X':
                        {
                            s++;
                            char nibbles[]{ *s++, *s++ };
                            if (!IsHex(nibbles[0]) || !IsHex(nibbles[1]))
                            {
                                AddError("Escape code must be followed 2 hex digits in string literal");
                                return false;
                            }
                            uint8_t value = HexPairToByte(nibbles[0], nibbles[1]);
                            m_decodedString += value;
                            break;
                        }
                        case 'u': // unicode sequence
                        case 'U':
                        {
                            s++;
                            char nibbles[]{ *s++, *s++, *s++, *s++ };
                            if (!IsHex(nibbles[0]) || !IsHex(nibbles[1]) || !IsHex(nibbles[2]) || !IsHex(nibbles[3]))
                            {
                                AddError("Escape code must be followed 4 hex digits in string literal");
                                return false;
                            }
                            uint16_t high = HexPairToByte(nibbles[0], nibbles[1]);
                            uint16_t low = HexPairToByte(nibbles[2], nibbles[3]);
                            uint16_t value = (high << 8) | low;

                            if (value >= 0xD800 && value <= 0xDBFF)
                            {
                                if (unicodeHighSurrogate != -1)
                                {
                                    AddError("Illegal Unicode sequence (multiple high surrogates) in string literal");
                                    return false;
                                }
                                else
                                {
                                    unicodeHighSurrogate = static_cast<int>(value);
                                }
                            }
                            else if (value >= 0xDC00 && value <= 0xDFFF)
                            {
                                if (unicodeHighSurrogate == -1)
                                {
                                    AddError("Illegal Unicode sequence (unpaired low surrogate) in string literal");
                                    return false;
                                }

                                uint32_t ucc = 0x10000 + ((unicodeHighSurrogate & 0x03FF) << 10) + (value & 0x03FF);
                                ToUTF8(m_decodedString, ucc);
                                unicodeHighSurrogate = -1;
                            }
                            else
                            {
                                if (unicodeHighSurrogate == -1)
                                {
                                    AddError("Illegal Unicode sequence (unpaired high surrogate) in string literal");
                                    return false;
                                }
                                ToUTF8(m_decodedString, value);
                            }
                        }
                        default:
                            AddError("Unknown escape sequence: \\{}", *s);
                            return false;
                    }
                    break;
                default:
                    if (unicodeHighSurrogate != -1)
                    {
                        AddError("Illegal Unicode sequence (unpaired high surrogate)");
                        return false;
                    }

                    decodedIsTrivial &= IsPrint(c);

                    m_decodedString += c;
                    break;
            }
        }

        if (unicodeHighSurrogate != -1)
        {
            AddError("Illegal Unicode sequence (unpaired high surrogate)");
            return false;
        }

        if (!decodedIsTrivial && !ValidateUTF8(m_decodedString.Data()))
        {
            AddError("Illegal UTF-8 sequence");
            return false;
        }

        return true;
    }
}

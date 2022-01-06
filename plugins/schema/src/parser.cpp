// Copyright Chad Engler

// TODO: source info, forward declarations

#include "he/schema/parser.h"

#include "struct_layout.h"
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

#include <algorithm>
#include <limits>

namespace he::schema
{

    struct FieldPtrOrdinalSort { bool operator()(const Field* a, const Field* b) const { return a->ordinal < b->ordinal; } };
    struct MethodDeclOrderSort { bool operator()(const Method& a, const Method& b) const { return a.declOrder < b.declOrder; } };
    struct MethodOrdinalSort { bool operator()(const Method& a, const Method& b) const { return a.ordinal < b.ordinal; } };

    // Keywords
    constexpr StringView KW_Attribute = "attribute";
    constexpr StringView KW_Const = "const";
    constexpr StringView KW_Enum = "enum";
    constexpr StringView KW_Enumerator = "enumerator";
    constexpr StringView KW_Extends = "extends";
    constexpr StringView KW_False = "false";
    constexpr StringView KW_Field = "field";
    constexpr StringView KW_File = "file";
    constexpr StringView KW_Group = "group";
    constexpr StringView KW_Import = "import";
    constexpr StringView KW_Interface = "interface";
    constexpr StringView KW_Method = "method";
    constexpr StringView KW_Namespace = "namespace";
    constexpr StringView KW_Parameter = "parameter";
    constexpr StringView KW_Struct = "struct";
    constexpr StringView KW_True = "true";
    constexpr StringView KW_Union = "union";

    constexpr StringView KW_List = "List";
    constexpr StringView KW_Blob = "Blob";

    struct BuiltinType { const StringView name; const TypeKind kind; };
    constexpr BuiltinType BuiltinTypes[] =
    {
        { "void", TypeKind::Void },
        { "bool", TypeKind::Bool },
        { "int8", TypeKind::Int8 },
        { "int16", TypeKind::Int16 },
        { "int32", TypeKind::Int32 },
        { "int64", TypeKind::Int64 },
        { "uint8", TypeKind::Uint8 },
        { "uint16", TypeKind::Uint16 },
        { "uint32", TypeKind::Uint32 },
        { "uint64", TypeKind::Uint64 },
        { "float32", TypeKind::Float32 },
        { "float64", TypeKind::Float64 },
        { "Blob", TypeKind::Blob },
        { "String", TypeKind::String },
        { "AnyPointer", TypeKind::AnyPointer },
    };

    Parser::Parser()
    {
        for (const BuiltinType& t : BuiltinTypes)
        {
            m_builtinTypeMap[t.name] = t.kind;
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
            m_token = m_lexer.PeekNextToken();
            AddLexerError();
            return false;
        }

        NextDecl();

        m_schema.root.kind = DeclKind::File;

        if (!ConsumeFileId())
            return false;

        while (TryConsume(Lexer::TokenType::Semicolon)) {}

        if (!ConsumeImports(includeDirs))
            return false;

        while (!AtEnd())
        {
            if (!ConsumeTopLevelDecl())
            {
                if (At(Lexer::TokenType::Error))
                    break;

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

        return !HasErrors();
    }

    bool Parser::ParseFileInternal(const char* path, Span<const char*> includeDirs)
    {
        // Can't use scratchString here because this function is reentrant, and the scratch
        // string is already used for file paths in LoadFile().
        String contents(Allocator::GetTemp());
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

        m_scratchString.Clear();
        for (const char* dir : includeDirs)
        {
            m_scratchString = dir;
            ConcatPath(m_scratchString, path);
            NormalizePath(m_scratchString);

            if (!OpenFile(file, m_scratchString.Data()))
                return false;

            if (file.IsOpen())
                return ReadFile(dst, file, m_scratchString.Data());
        }

        AddError("Failed to find file: {}", m_scratchString);
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

    bool Parser::Expect(Lexer::TokenType expected)
    {
        if (!At(expected))
        {
            if (At(Lexer::TokenType::Error))
                AddLexerError();
            else
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
        do
        {
            Next();
        } while (At(Lexer::TokenType::Comment) || At(Lexer::TokenType::Whitespace) || At(Lexer::TokenType::Newline));
    }

    bool Parser::NextDecl(Lexer::TokenType expected)
    {
        NextDecl();
        return Expect(expected);
    }

    bool Parser::SkipWhitespace(Lexer::TokenType expected)
    {
        do
        {
            Next();
        } while (At(Lexer::TokenType::Whitespace));

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

            Next();
        }
    }

    void Parser::SkipBlock()
    {
        while (!AtEnd() && !At(Lexer::TokenType::CloseCurlyBracket))
            Next();

        if (At(Lexer::TokenType::CloseCurlyBracket))
        {
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

    bool Parser::TryConsumeId(TypeId& out)
    {
        if (!At(Lexer::TokenType::Ordinal))
            return false;

        const StringView str{ m_token.text.begin() + 1, m_token.text.end() };
        if (!DecodeInt(str, out))
            return false;

        if (!HasFlag(out, TypeIdFlag))
        {
            AddError("Invalid unique ID. Generate one with `he_schemac id`");
            return false;
        }

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

    bool Parser::ConsumeFileId()
    {
        if (!At(Lexer::TokenType::Ordinal))
        {
            AddError("Unexpected token {}, expected file unique ID", m_token.type);
            return false;
        }

        if (!TryConsumeId(m_schema.root.id))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeImports(Span<const char*> includeDirs)
    {
        ++m_importDepth;
        HE_AT_SCOPE_EXIT([&]() { --m_importDepth; });

        if (m_importDepth > 256)
        {
            AddError("Import depth too deep, do you have an import cycle?");
            return false;
        }

        Vector<Import> imports;

        String importPath;
        while (TryConsumeKeyword(KW_Import))
        {
            importPath.Clear();

            if (!ConsumeString(importPath))
                return false;

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;

            Lexer tempLexer(Move(m_lexer));
            Lexer::Token tempToken(Move(m_token));
            StringView tempFileName = m_fileName;
            TypeId tempFileId = m_schema.root.id;

            if (!ParseFileInternal(importPath.Data(), includeDirs))
                return false;

            m_lexer = Move(tempLexer);
            m_token = Move(tempToken);
            m_fileName = tempFileName;

            // Move the parsed schema into the imports list
            Import& im = imports.EmplaceBack();
            im.path = Move(importPath);
            im.schema = Allocator::GetDefault().New<SchemaFile>();

            *im.schema = Move(m_schema);

            // Add the schema to our namespace lookup table
            auto result = m_importsByNamespace.try_emplace(m_schema.root.name);
            result.first->second.PushBack(im.schema);

            // reset the local schema
            m_schema = SchemaFile{};
            m_schema.root.kind = DeclKind::File;
            m_schema.root.id = tempFileId;
        }

        m_schema.imports = Move(imports);

        return true;
    }

    bool Parser::ConsumeNamespace()
    {
        if (!m_schema.root.name.IsEmpty())
        {
            AddError("There can only be one namespace declaration per file");
            return false;
        }

        if (!ConsumeIdentifier(m_schema.root.name))
            return false;

        while (TryConsume(Lexer::TokenType::Dot))
        {
            m_schema.root.name += '.';
            if (!ConsumeIdentifier(m_schema.root.name))
                return false;
        }

        if (m_schema.root.name.IsEmpty())
        {
            AddError("Namespace identifier cannot be empty");
            return false;
        }

        if (m_schema.root.name[0] == '.')
        {
            AddError("Namespace identifier cannot start with a dot");
            return false;
        }

        if (!Consume(Lexer::TokenType::Semicolon))
            return false;

        return true;
    }

    bool Parser::ConsumeTopLevelDecl()
    {
        // ignore empty statements
        while (TryConsume(Lexer::TokenType::Semicolon)) {}

        // Consume any file-level attributes
        while (At(Lexer::TokenType::Dollar))
        {
            if (!ConsumeAttributes(m_schema.attributes))
                return false;

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;
        }

        if (!Expect(Lexer::TokenType::Identifier))
            return false;

        const StringView name = m_token.text;
        NextDecl();

        if (name == KW_Namespace)
            return ConsumeNamespace();

        if (name == KW_Attribute)
            return ConsumeAttributeDecl(m_schema.root);

        if (name == KW_Const)
            return ConsumeConstDecl(m_schema.root);

        if (name == KW_Enum)
            return ConsumeEnumDecl(m_schema.root);

        if (name == KW_Interface)
            return ConsumeInterfaceDecl(m_schema.root);

        if (name == KW_Struct)
            return ConsumeStructDecl(m_schema.root);

        if (name == KW_Import)
        {
            AddError("Imports must be the first non-comment statements after the file's unique ID");
            return false;
        }

        AddError("Unknown top-level identifier '{}'. Expected one of: attribute, const, enum, namespace, interface, struct", name);
        return false;
    }

    bool Parser::ConsumeAttributes(Vector<Attribute>& attributes)
    {
        while (TryConsume(Lexer::TokenType::Dollar))
        {
            m_scratchString.Clear();

            if (!ConsumeIdentifier(m_scratchString))
                return false;

            const Declaration* decl = FindDecl(m_scratchString);
            if (decl == nullptr)
            {
                AddError("Unknown attribute '{}', did you try to use it before it was declared?", m_scratchString);
                return false;
            }
            else if (decl->kind != DeclKind::Attribute)
            {
                AddError("The symbol '{}' is not an attribute", m_scratchString);
                return false;
            }

            Attribute& attr = attributes.EmplaceBack();
            attr.id = decl->id;

            if (TryConsume(Lexer::TokenType::OpenParens))
            {
                if (!At(Lexer::TokenType::CloseParens) && decl->attribute_.type.kind == TypeKind::Void)
                {
                    AddError("Cannot pass parameters to an attribute with a type of 'void'");
                    return false;
                }

                if (!ConsumeValue(decl->attribute_.type, attr.value))
                    return false;

                if (!Consume(Lexer::TokenType::CloseParens))
                    return false;
            }
        }

        return true;
    }

    bool Parser::ConsumeArraySize(Type& type)
    {
        // Array size of a type
        if (!TryConsume(Lexer::TokenType::OpenSquareBracket))
            return true;

        Type* elementType = Allocator::GetDefault().New<Type>();
        *elementType = Move(type);

        type.kind = TypeKind::Array;
        type.array_.elementType = elementType;

        constexpr uint32_t MaxValue =  std::numeric_limits<uint16_t>::max();

        if (At(Lexer::TokenType::Integer))
        {
            if (!ConsumeInt(type.array_.size))
                return false;
        }
        else if (At(Lexer::TokenType::Identifier))
        {
            m_scratchString.Clear();
            if (!ConsumeIdentifier(m_scratchString))
                return false;

            const Declaration* constDecl = FindDecl(m_scratchString);
            if (!constDecl || constDecl->kind != DeclKind::Const || !IsIntegral(constDecl->const_.type.kind))
            {
                AddError("Only integral constants or literals can be used for array sizes");
                return false;
            }

            switch (constDecl->const_.type.kind)
            {
                case TypeKind::Int8:
                {
                    const int8_t v = constDecl->const_.value.i8;
                    if (v > 0)
                        type.array_.size = v;
                    break;
                }
                case TypeKind::Int16:
                {
                    const int16_t v = constDecl->const_.value.i16;
                    if (v > 0)
                        type.array_.size = static_cast<uint16_t>(v);
                    break;
                }
                case TypeKind::Int32:
                {
                    const int32_t v = constDecl->const_.value.i32;
                    if (v > 0 && v <= MaxValue)
                        type.array_.size = static_cast<uint16_t>(v);
                    break;
                }
                case TypeKind::Int64:
                {
                    const int64_t v = constDecl->const_.value.i64;
                    if (v > 0 && v <= MaxValue)
                        type.array_.size = static_cast<uint16_t>(v);
                    break;
                }
                case TypeKind::Uint8:
                {
                    const uint8_t v = constDecl->const_.value.u8;
                    if (v > 0)
                        type.array_.size = v;
                    break;
                }
                case TypeKind::Uint16:
                {
                    const uint16_t v = constDecl->const_.value.u16;
                    if (v > 0)
                        type.array_.size = v;
                    break;
                }
                case TypeKind::Uint32:
                {
                    const uint32_t v = constDecl->const_.value.u32;
                    if (v > 0 && v <= MaxValue)
                        type.array_.size = static_cast<uint16_t>(v);
                    break;
                }
                case TypeKind::Uint64:
                {
                    const uint64_t v = constDecl->const_.value.u64;
                    if (v > 0 && v <= MaxValue)
                        type.array_.size = static_cast<uint16_t>(v);
                    break;
                }
                default:
                    HE_ASSERT(false, "Invalid const value type.");
                    AddError("Encountered invalid const value type. This is a Schema parser bug");
                    return false;
            }
        }

        if (type.array_.size == 0)
        {
            AddError("Invalid array size. It must be in the range (0, {}]", MaxValue);
            return false;
        }

        if (!Consume(Lexer::TokenType::CloseSquareBracket))
            return false;

        return true;
    }

    bool Parser::ConsumeType(Type& type, const Declaration* scope)
    {
        m_scratchString.Clear();
        if (!ConsumeIdentifier(m_scratchString))
            return false;

        // Check for a generic type name
        if (scope)
        {
            const TypeParamRef ref = FindTypeParam(m_scratchString, *scope);
            if (ref.scope)
            {
                type.kind = TypeKind::AnyPointer;
                type.any_.paramScopeId = ref.scope->id;
                type.any_.paramIndex = static_cast<uint16_t>(ref.index);
                return ConsumeArraySize(type);
            }
        }

        // Built-in type
        auto it = m_builtinTypeMap.find(m_scratchString);
        if (it != m_builtinTypeMap.end())
        {
            type.kind = it->second;
            return ConsumeArraySize(type);
        }

        // List<T>
        if (m_scratchString == KW_List)
        {
            type.kind = TypeKind::List;
            type.list_.elementType = Allocator::GetDefault().New<Type>();

            if (!Consume(Lexer::TokenType::OpenAngleBracket))
                return false;

            if (!ConsumeType(*type.list_.elementType, scope))
                return false;

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;

            return ConsumeArraySize(type);
        }

        // User-defined type
        const Declaration* decl = FindDecl(m_scratchString);

        if (decl == nullptr && scope != nullptr)
        {
            decl = FindDecl(m_scratchString, *scope);
        }

        if (decl == nullptr)
        {
            decl = FindForwardDecl(m_scratchString);
        }

        if (decl == nullptr && scope != nullptr)
        {
            decl = FindForwardDecl(m_scratchString, *scope);
        }

        if (decl == nullptr)
        {
            AddError("Unknown type '{}', did you try to use it before it was declared?", m_scratchString);
            return false;
        }

        switch (decl->kind)
        {
            case DeclKind::None:
                AddError("Unknown type '{}', declaration kind was None", m_scratchString);
                return false;
            case DeclKind::Attribute:
                AddError("Attributes cannot be used as types");
                return false;
            case DeclKind::Const:
                AddError("Constants cannot be used as types");
                return false;
            case DeclKind::Enum:
                type.kind = TypeKind::Enum;
                type.enum_.id = decl->id;
                break;
            case DeclKind::Interface:
                if (type.kind == TypeKind::Struct)
                    type.interface_.brand = Move(type.struct_.brand);
                type.kind = TypeKind::Interface;
                type.interface_.id = decl->id;
                break;
            case DeclKind::Struct:
                if (type.kind == TypeKind::Interface)
                    type.struct_.brand = Move(type.interface_.brand);
                type.kind = TypeKind::Struct;
                type.struct_.id = decl->id;
                break;
        }

        // Brand for a user-defined pointer type
        if (TryConsume(Lexer::TokenType::OpenAngleBracket))
        {
            if (type.kind != TypeKind::Interface && type.kind != TypeKind::Struct)
            {
                AddError("Only interface and struct types can have generic parameters");
                return false;
            }

            if (decl->typeParams.IsEmpty())
            {
                AddError("The type '{}' is not generic", decl->name);
                return false;
            }

            Brand& brand = type.kind == TypeKind::Struct ? type.struct_.brand : type.interface_.brand;
            Brand::Scope& brandScope = brand.scopes.EmplaceBack();

            brandScope.scopeId = decl->id;

            do
            {
                Type* param = brandScope.params.PushBack(Allocator::GetDefault().New<Type>());

                if (!ConsumeType(*param, scope))
                    return false;

                if (!IsPointer(*param))
                {
                    AddError("Only pointer types can be used as type parameters");
                    return false;
                }
            } while (TryConsume(Lexer::TokenType::Comma));

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;
        }
        else if (!decl->typeParams.IsEmpty())
        {
            AddError("The type '{}' is generic, you must specify type parameters", decl->name);
            return false;
        }

        if (TryConsume(Lexer::TokenType::Dot))
            return ConsumeType(type, decl);

        return ConsumeArraySize(type);
    }

    bool Parser::ConsumeValue(const Type& type, Value& value)
    {
        value.kind = type.kind;

        switch (type.kind)
        {
            case TypeKind::Void:
                AddError("Void types cannot specify a value");
                return false;
            case TypeKind::Bool:
                return ConsumeBool(value.b);
            case TypeKind::Int8:
                return ConsumeInt(value.i8);
            case TypeKind::Int16:
                return ConsumeInt(value.i16);
            case TypeKind::Int32:
                return ConsumeInt(value.i32);
            case TypeKind::Int64:
                return ConsumeInt(value.i64);
            case TypeKind::Uint8:
                return ConsumeInt(value.u8);
            case TypeKind::Uint16:
                return ConsumeInt(value.u16);
            case TypeKind::Uint32:
                return ConsumeInt(value.u32);
            case TypeKind::Uint64:
                return ConsumeInt(value.u64);
            case TypeKind::Float32:
                return ConsumeFloat(value.f32);
            case TypeKind::Float64:
                return ConsumeFloat(value.f64);
            case TypeKind::Array:
            {
                if (!Consume(Lexer::TokenType::OpenSquareBracket))
                    return false;
                do
                {
                    Value* v = value.array.PushBack(Allocator::GetDefault().New<Value>());
                    if (!ConsumeValue(*type.array_.elementType, *v))
                        return false;
                } while (TryConsume(Lexer::TokenType::Comma));

                if (!Consume(Lexer::TokenType::CloseSquareBracket))
                    return false;

                if (value.array.Size() != type.array_.size)
                {
                    AddError("A value for an array must specify a value for each array element");
                    return false;
                }

                return true;
            }
            case TypeKind::Blob:
                return ConsumeBlob(value.blob);
            case TypeKind::String:
                return ConsumeString(value.str);
            case TypeKind::List:
            {
                if (!Consume(Lexer::TokenType::OpenSquareBracket))
                    return false;

                do
                {
                    Value* v = value.list.PushBack(Allocator::GetDefault().New<Value>());
                    if (!ConsumeValue(*type.list_.elementType, *v))
                        return false;
                } while (TryConsume(Lexer::TokenType::Comma));

                if (!Consume(Lexer::TokenType::CloseSquareBracket))
                    return false;

                return true;
            }
            case TypeKind::Enum:
            {
                m_scratchString.Clear();
                if (!ConsumeString(m_scratchString))
                    return false;

                const char* last = &m_scratchString.Back();
                const char* dot = last;
                while (dot > m_scratchString.Begin() && *dot != '.')
                    --dot;

                StringView shortName{ dot, last };

                const Declaration* decl = FindDecl(type.enum_.id);
                if (!decl)
                {
                    HE_ASSERT(FindForwardDecl(type.enum_.id));
                    AddError("Cannot specify a default value for an enum that has only been forward declared");
                    return false;
                }

                HE_ASSERT(decl && decl->kind == DeclKind::Enum);
                for (const Enumerator& e : decl->enum_.enumerators)
                {
                    if (e.name == shortName)
                    {
                        value.enum_ = e.ordinal;
                        return true;
                    }
                }

                AddError("Unknown value '{}' in enum '{}'", shortName, decl->name);
                return false;
            }
            case TypeKind::Struct:
            {
                if (!Consume(Lexer::TokenType::OpenCurlyBracket))
                    return false;

                const Declaration* decl = FindDecl(type.struct_.id);
                if (!decl)
                {
                    HE_ASSERT(FindForwardDecl(type.struct_.id));
                    AddError("Cannot specify a default value for a struct that has only been forward declared");
                    return false;
                }

                HE_ASSERT(decl && decl->kind == DeclKind::Struct);
                do
                {
                    Value::StructValue& v = value.struct_.EmplaceBack();
                    v.value = Allocator::GetDefault().New<Value>();

                    if (!ConsumeIdentifier(v.fieldName))
                        return false;

                    const Type* fieldType = nullptr;
                    for (const Field& field : decl->struct_.fields)
                    {
                        if (field.name == v.fieldName)
                        {
                            fieldType = &field.type;
                            break;
                        }
                    }

                    if (fieldType == nullptr)
                    {
                        AddError("Unknown field '{}' in struct value specifier", v.fieldName);
                        return false;
                    }

                    if (!Consume(Lexer::TokenType::Equals))
                        return false;

                    if (!ConsumeValue(*fieldType, *v.value))
                        return false;
                } while (TryConsume(Lexer::TokenType::Comma));

                if (!Consume(Lexer::TokenType::CloseCurlyBracket))
                    return false;

                return true;
            }
            case TypeKind::Interface:
                AddError("Interface types cannot specify a value");
                return false;
            case TypeKind::AnyPointer:
                AddError("AnyPointer types cannot specify a value");
                return false;
        }

        HE_ASSERT(false, "Type is not a known enum value");
        AddError("Tried to consume a value for an invalid type, this should never happen");
        return false;
    }

    bool Parser::ConsumeTypeParams(Vector<String>& params)
    {
        if (TryConsume(Lexer::TokenType::OpenAngleBracket))
        {
            do
            {
                if (!ConsumeIdentifier(params.EmplaceBack()))
                    return false;
            } while (TryConsume(Lexer::TokenType::Comma));

            if (!Consume(Lexer::TokenType::CloseAngleBracket))
                return false;
        }

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

    bool Parser::ConsumeOrdinal(uint16_t& out)
    {
        if (!Expect(Lexer::TokenType::Ordinal))
            return false;

        const StringView str{ m_token.text.begin() + 1, m_token.text.end() };
        if (!DecodeInt(str, out))
            return false;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeBlob(Vector<uint8_t>& out)
    {
        if (!Expect(Lexer::TokenType::Blob))
            return false;

        const char* s = m_token.text.Begin();
        const char* end = m_token.text.End() - 1; // -1 so we point at end quote

        // Lexer should be validating this is true so we assert here to document our assumptions.
        HE_ASSERT(s[0] == '0' && s[1] == 'x' && s[2] == '"' && *end == '"');
        s += 3; // skip 0x"

        // Reserve the maximum possible byte size. Because we allow spaces this may over-allocate
        // but likely not by much, and that's better than multiple reallocs in the loop.
        out.Reserve(static_cast<uint32_t>(end - s) / 2);

        char first = 0;
        while (s < end)
        {
            if (first == 0)
            {
                first = *s;
            }
            else
            {
                const uint8_t byte = HexPairToByte(first, *s);
                out.PushBack(byte);
                first = 0;
            }

            ++s;
        }

        if (first != 0)
        {
            AddError("Invalid blob byte string, there are trailing nibbles");
            return false;
        }

        NextDecl();
        return true;
    }

    bool Parser::ConsumeString(String& out)
    {
        if (!Expect(Lexer::TokenType::String))
            return false;

        if (!DecodeString())
            return false;

        out = m_scratchString;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeBool(bool& out)
    {
        if (TryConsumeKeyword(KW_True))
        {
            out = true;
            NextDecl();
            return true;
        }

        if (TryConsumeKeyword(KW_False))
        {
            out = false;
            NextDecl();
            return true;
        }

        AddError("Expected `true` or `false` for boolean value");
        return false;
    }

    template <std::integral T>
    bool Parser::ConsumeInt(T& out)
    {
        bool isSigned = TryConsume(Lexer::TokenType::Minus);

        if (!Expect(Lexer::TokenType::Integer))
            return false;

        if (!DecodeInt<T>(m_token.text, out))
            return false;

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

        NextDecl();
        return true;
    }

    template <std::floating_point T>
    bool Parser::ConsumeFloat(T& out)
    {
        bool isSigned = TryConsume(Lexer::TokenType::Minus);

        if (!Expect(Lexer::TokenType::Float))
            return false;

        double value = m_token.text.ToFloat<double>();

        if (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
            AddError("Float value out of range");
            return false;
        }

        out = static_cast<T>(value);

        if (isSigned)
            out *= -1;

        NextDecl();
        return true;
    }

    bool Parser::ConsumeDeclName(Declaration& decl)
    {
        decl.source.docComment = ""; // TODO
        decl.source.file = m_fileName;
        decl.source.line = m_token.line;
        decl.source.column = m_token.column;

        if (!ConsumeIdentifier(decl.name))
            return false;

        if (!IsUpper(decl.name[0]))
        {
            AddError("Declaration names should start with an uppercase character");
            // return false; // No need to return we can continue parsing.
        }

        if (At(Lexer::TokenType::Ordinal) && !TryConsumeId(decl.id))
            return false;
        else
            decl.id = 0;

        return StoreDeclId(decl);
    }

    bool Parser::ConsumeAttributeDecl(Declaration& parent)
    {
        Declaration& decl = parent.children.EmplaceBack();
        decl.parentId = parent.id;
        decl.kind = DeclKind::Attribute;

        if (!ConsumeDeclName(decl))
            return false;

        if (!Consume(Lexer::TokenType::OpenParens))
            return false;

        do
        {
            if (At(Lexer::TokenType::CloseParens))
                break;

            if (TryConsume(Lexer::TokenType::Asterisk))
            {
                decl.attribute_.targetsAttribute = true;
                decl.attribute_.targetsConst = true;
                decl.attribute_.targetsEnum = true;
                decl.attribute_.targetsEnumerator = true;
                decl.attribute_.targetsField = true;
                decl.attribute_.targetsFile = true;
                decl.attribute_.targetsInterface = true;
                decl.attribute_.targetsMethod = true;
                decl.attribute_.targetsParameter = true;
                decl.attribute_.targetsStruct = true;
                break;
            }

            if (!Expect(Lexer::TokenType::Identifier))
                return false;

            if (m_token.text == KW_Attribute)
                decl.attribute_.targetsAttribute = true;
            else if (m_token.text == KW_Const)
                decl.attribute_.targetsConst = true;
            else if (m_token.text == KW_Enum)
                decl.attribute_.targetsEnum = true;
            else if (m_token.text == KW_Enumerator)
                decl.attribute_.targetsEnumerator = true;
            else if (m_token.text == KW_Field)
                decl.attribute_.targetsField = true;
            else if (m_token.text == KW_File)
                decl.attribute_.targetsFile = true;
            else if (m_token.text == KW_Interface)
                decl.attribute_.targetsInterface = true;
            else if (m_token.text == KW_Method)
                decl.attribute_.targetsMethod = true;
            else if (m_token.text == KW_Parameter)
                decl.attribute_.targetsParameter = true;
            else if (m_token.text == KW_Struct)
                decl.attribute_.targetsStruct = true;
            else
            {
                AddError("Expected a valid attribute target. Must be one of: attribute, const, enum, enumerator, field, file, interface, method, parameter, or struct");
                return false;
            }
        } while (TryConsume(Lexer::TokenType::Comma));

        if (!Consume(Lexer::TokenType::CloseParens))
            return false;

        if (TryConsume(Lexer::TokenType::Colon))
        {
            if (!ConsumeType(decl.attribute_.type, nullptr))
                return false;
        }

        if (!ConsumeAttributes(decl.attributes))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeConstDecl(Declaration& parent)
    {
        Declaration& decl = parent.children.EmplaceBack();
        decl.parentId = parent.id;
        decl.kind = DeclKind::Const;

        if (!ConsumeDeclName(decl))
            return false;

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        if (!ConsumeType(decl.const_.type, &decl))
            return false;

        if (decl.const_.type.kind == TypeKind::Void)
        {
            AddError("Constant values cannot have a void type");
            return false;
        }

        if (!Consume(Lexer::TokenType::Equals))
            return false;

        if (!ConsumeValue(decl.const_.type, decl.const_.value))
            return false;

        return Consume(Lexer::TokenType::Semicolon);
    }

    bool Parser::ConsumeEnumDecl(Declaration& parent)
    {
        Declaration candidate;
        candidate.parentId = parent.id;
        candidate.kind = DeclKind::Enum;

        if (!ConsumeDeclName(candidate))
            return false;

        const Declaration* forward = FindForwardDecl(candidate.id);
        if (forward)
        {
            if (forward->kind != candidate.kind)
            {
                AddError("Enum was previously declared as a {}", forward->kind);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->name != candidate.name)
            {
                AddError("Enum was previously declared with a different name: {}", candidate.name);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->parentId != candidate.parentId)
            {
                const Declaration* forwardParent = FindDecl(forward->parentId);
                HE_ASSERT(forwardParent);
                AddError("Enum was previously declared within a different parent object: {}", forwardParent->name);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }
        }

        if (At(Lexer::TokenType::Semicolon))
        {
            m_nameMap.erase(candidate.id);
            m_forwardIds.emplace(candidate.id);
            parent.forwards.PushBack(Move(candidate));
            return true;
        }

        Declaration& decl = parent.children.PushBack(Move(candidate));

        if (!ConsumeAttributes(decl.attributes))
            return false;

        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in enum definition (missing '}}')");
                return false;
            }

            Enumerator& member = decl.enum_.enumerators.EmplaceBack();
            member.declOrder = static_cast<uint16_t>(decl.enum_.enumerators.Size() - 1);

            if (!ConsumeIdentifier(member.name))
                return false;

            if (!IsUpper(member.name[0]))
            {
                AddError("Enumerator names must start with an uppercase letter");
                // return false; // No need to return we can continue parsing.
            }

            if (!ConsumeOrdinal(member.ordinal))
                return false;

            if (!ConsumeAttributes(member.attributes))
                return false;

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;
        }

        // Check for duplicate member names
        const uint32_t size = decl.enum_.enumerators.Size();
        for (uint32_t i = 0; i < size; ++i)
        {
            for (uint32_t j = i + 1; j < size; ++j)
            {
                const Enumerator& a = decl.enum_.enumerators[i];
                const Enumerator& b = decl.enum_.enumerators[j];

                if (a.name == b.name)
                {
                    AddError("Multiple enumerators in enum '{}' have the name '{}'", decl.name, a.name);
                    return false;
                }

                if (a.ordinal == b.ordinal)
                {
                    AddError("Multiple enumerators in enum '{}' have the ordinal '@{}'", decl.name, a.ordinal);
                    return false;
                }
            }
        }

        return true;
    }

    bool Parser::ConsumeInterfaceDecl(Declaration& parent)
    {
        Declaration candidate;
        candidate.parentId = parent.id;
        candidate.kind = DeclKind::Interface;

        if (!ConsumeDeclName(candidate))
            return false;

        if (!ConsumeTypeParams(candidate.typeParams))
            return false;

        const Declaration* forward = FindForwardDecl(candidate.id);
        if (forward)
        {
            if (forward->kind != candidate.kind)
            {
                AddError("Interface was previously declared as a {}", forward->kind);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->name != candidate.name)
            {
                AddError("Interface was previously declared with a different name: {}", candidate.name);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->parentId != candidate.parentId)
            {
                const Declaration* forwardParent = FindDecl(forward->parentId);
                HE_ASSERT(forwardParent);
                AddError("Interface was previously declared within a different parent object: {}", forwardParent->name);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->typeParams != candidate.typeParams)
            {
                AddError("Interface was previously declared with different type parameters");
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }
        }

        if (At(Lexer::TokenType::Semicolon))
        {
            m_nameMap.erase(candidate.id);
            m_forwardIds.emplace(candidate.id);
            parent.forwards.PushBack(Move(candidate));
            return true;
        }

        Declaration& decl = parent.children.PushBack(Move(candidate));

        if (TryConsumeKeyword(KW_Extends))
        {
            if (!ConsumeType(decl.interface_.super, &decl))
                return false;

            if (decl.interface_.super.kind != TypeKind::Interface)
            {
                AddError("Interfaces can only extend other interfaces");
                return false;
            }
        }

        if (!ConsumeAttributes(decl.attributes))
            return false;

        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in interface declaration (missing '}}')");
                return false;
            }

            // ignore empty statements
            if (TryConsume(Lexer::TokenType::Semicolon))
                return true;

            if (!Expect(Lexer::TokenType::Identifier))
                return false;

            StringView name = m_token.text;
            NextDecl();

            if (name == KW_Const)
            {
                if (!ConsumeConstDecl(decl))
                    return false;
                continue;
            }

            if (name == KW_Enum)
            {
                if (!ConsumeEnumDecl(decl))
                    return false;
                continue;
            }

            if (name == KW_Struct)
            {
                if (!ConsumeStructDecl(decl))
                    return false;
                continue;
            }

            if (!IsUpper(name[0]))
            {
                AddError("Interface method names must start with an uppercase letter");
                // return false; // No need to return we can continue parsing.
            }

            Method& method = decl.interface_.methods.EmplaceBack();
            method.declOrder = static_cast<uint16_t>(decl.interface_.methods.Size() - 1);
            method.name = name;

            if (!ConsumeTypeParams(method.typeParams))
                return false;

            if (!ConsumeOrdinal(method.ordinal))
                return false;

            Declaration& paramStruct = decl.children.EmplaceBack();
            paramStruct.kind = DeclKind::Struct;
            paramStruct.struct_.isAutoGenerated = true;
            paramStruct.parentId = decl.id;
            paramStruct.name = method.name;
            paramStruct.name += "_ParamStruct";
            paramStruct.name[0] = ToUpper(paramStruct.name[0]);
            if (!StoreDeclId(paramStruct))
                return false;
            method.paramStruct = paramStruct.id;

            if (!ConsumeTupleStruct(paramStruct))
                return false;

            if (!Consume(Lexer::TokenType::Arrow))
                return false;

            if (!At(Lexer::TokenType::Semicolon))
            {
                Declaration& resultStruct = decl.children.EmplaceBack();
                resultStruct.kind = DeclKind::Struct;
                resultStruct.struct_.isAutoGenerated = true;
                resultStruct.parentId = decl.id;
                resultStruct.name = method.name;
                resultStruct.name += "_ResultStruct";
                resultStruct.name[0] = ToUpper(resultStruct.name[0]);
                if (!StoreDeclId(resultStruct))
                    return false;
                method.resultStruct = resultStruct.id;

                if (!ConsumeTupleStruct(resultStruct))
                    return false;
            }

            if (!Consume(Lexer::TokenType::Semicolon))
                return false;
        }

        const uint32_t size = decl.interface_.methods.Size();

        if (size == 0)
            return true;

        std::sort(decl.interface_.methods.begin(), decl.interface_.methods.end(), MethodOrdinalSort{});

        if (decl.interface_.methods[0].ordinal != 0)
        {
            AddError("Methods in interface '{}' skipped ordinal @0. Ordinals must be sequential, without any gaps, starting at zero", decl.name);
            return false;
        }

        // Check for duplicate member names, duplicate ordinals, and ordinal gaps
        for (uint32_t i = 0; i < size; ++i)
        {
            const Method& a = decl.interface_.methods[i];

            if (i < (size - 1))
            {
                const Method& b = decl.interface_.methods[i + 1];
                const uint32_t diff = b.ordinal - a.ordinal;

                if (diff == 0)
                {
                    AddError("Multiple methods in interface '{}' have the ordinal '@{}'", decl.name, a.ordinal);
                    return false;
                }

                if (diff > 1)
                {
                    AddError("Methods in interface '{}' skipped ordinal @{}. Ordinals must be sequential, without any gaps, starting at zero", decl.name, a.ordinal + 1);
                    return false;
                }
            }

            for (uint32_t j = i + 1; j < size; ++j)
            {
                const Method& b = decl.interface_.methods[j];

                if (a.name == b.name)
                {
                    AddError("Multiple methods in interface '{}' have the name '{}'", decl.name, a.name);
                    return false;
                }
            }
        }

        std::sort(decl.interface_.methods.begin(), decl.interface_.methods.end(), MethodDeclOrderSort{});
        return true;
    }

    bool Parser::ConsumeStructDecl(Declaration& parent)
    {
        Declaration candidate;
        candidate.parentId = parent.id;
        candidate.kind = DeclKind::Struct;

        if (!ConsumeDeclName(candidate))
            return false;

        if (!ConsumeTypeParams(candidate.typeParams))
            return false;

        const Declaration* forward = FindForwardDecl(candidate.id);
        if (forward)
        {
            if (forward->kind != candidate.kind)
            {
                AddError("Structure was previously declared as a {}", forward->kind);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->name != candidate.name)
            {
                AddError("Structure was previously declared with a different name: {}", candidate.name);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->parentId != candidate.parentId)
            {
                const Declaration* forwardParent = FindDecl(forward->parentId);
                HE_ASSERT(forwardParent);
                AddError("Structure was previously declared within a different parent object: {}", forwardParent->name);
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }

            if (forward->typeParams != candidate.typeParams)
            {
                AddError("Structure was previously declared with different type parameters");
                AddDeclError(*forward, "See previous declaration here");
                return false;
            }
        }

        if (At(Lexer::TokenType::Semicolon))
        {
            m_nameMap.erase(candidate.id);
            m_forwardIds.emplace(candidate.id);
            parent.forwards.PushBack(Move(candidate));
            return true;
        }

        Declaration& decl = parent.children.PushBack(Move(candidate));

        if (!ConsumeAttributes(decl.attributes))
            return false;

        if (!ConsumeStructBlock(decl))
            return false;

        if (!ValidateAndLayoutStruct(decl))
            return false;

        return true;
    }

    bool Parser::ConsumeStructBlock(Declaration& decl)
    {
        if (!Consume(Lexer::TokenType::OpenCurlyBracket))
            return false;

        while (!TryConsume(Lexer::TokenType::CloseCurlyBracket))
        {
            if (AtEnd())
            {
                AddError("Reached end of input in struct declaration (missing '}}')");
                return false;
            }

            // ignore empty statements
            if (TryConsume(Lexer::TokenType::Semicolon))
                return true;

            if (!Expect(Lexer::TokenType::Identifier))
                return false;

            const StringView name = m_token.text;
            NextDecl();

            // Normal field
            if (At(Lexer::TokenType::Ordinal))
            {
                if (!IsLower(name[0]))
                {
                    AddError("Field names must start with a lowercase character");
                    // return false; // No need to return we can continue parsing.
                }

                Field& field = decl.struct_.fields.EmplaceBack();
                field.declOrder = static_cast<uint16_t>(decl.struct_.fields.Size() - 1);
                field.name = name;

                if (!ConsumeField(field, decl, true))
                    return false;

                if (!Consume(Lexer::TokenType::Semicolon))
                    return false;
            }
            // Union or group field
            else if (TryConsume(Lexer::TokenType::Colon))
            {
                if (!IsLower(name[0]))
                {
                    AddError("Field names must start with a lowercase character");
                    // return false; // No need to return we can continue parsing.
                }

                if (!Expect(Lexer::TokenType::Identifier))
                    return false;

                const bool isGroup = m_token.text == KW_Group;
                const bool isUnion = m_token.text == KW_Union;
                NextDecl();

                if (!isGroup && !isUnion)
                {
                    AddError("Only groups, unions, and type declarations can specify a name without an ordinal");
                    return false;
                }

                Declaration& group = decl.children.EmplaceBack();
                group.kind = DeclKind::Struct;
                group.parentId = decl.id;
                group.name = name;
                group.name[0] = ToUpper(group.name[0]);
                group.struct_.isAutoGenerated = true;
                group.struct_.isGroup = isGroup;
                group.struct_.isUnion = isUnion;
                if (!StoreDeclId(group))
                    return false;

                if (!ConsumeStructBlock(group))
                    return false;

                Vector<Field*> sortedFields(Allocator::GetTemp());
                for (Field& field : group.struct_.fields)
                {
                    sortedFields.PushBack(&field);
                }

                std::sort(sortedFields.begin(), sortedFields.end(), FieldPtrOrdinalSort{});
                for (uint32_t i = 0; i < sortedFields.Size(); ++i)
                {
                    sortedFields[i]->unionTag = static_cast<uint16_t>(i);
                }

                Field& field = decl.struct_.fields.EmplaceBack();
                field.declOrder = static_cast<uint16_t>(decl.struct_.fields.Size() - 1);
                field.name = name;
                field.ordinal = sortedFields[0]->ordinal;
                field.type.kind = TypeKind::Struct;
                field.isGroup = isGroup;
                field.isUnion = isUnion;
                field.typeId = group.id;
            }
            // Nested declaration (const, enum, or struct)
            else if (At(Lexer::TokenType::Identifier))
            {
                if (decl.struct_.isGroup || decl.struct_.isUnion)
                {
                    AddError("Only fields, groups, and unions are allowed in group and union blocks");
                    return false;
                }

                if (name == KW_Const)
                {
                    if (!ConsumeConstDecl(decl))
                        return false;
                    continue;
                }

                if (name == KW_Enum)
                {
                    if (!ConsumeEnumDecl(decl))
                        return false;
                    continue;
                }

                if (name == KW_Struct)
                {
                    if (!ConsumeStructDecl(decl))
                        return false;
                    continue;
                }

                AddError("Invalid declaration. Expected one of: const, enum, struct");
                return false;
            }
        }

        return true;
    }

    bool Parser::ConsumeField(Field& field, const Declaration& scope, bool requireOrdinal)
    {
        // Ordinals are optional for tuple struct syntax
        if (requireOrdinal || At(Lexer::TokenType::Ordinal))
        {
            if (!ConsumeOrdinal(field.ordinal))
                return false;
        }

        if (!Consume(Lexer::TokenType::Colon))
            return false;

        if (!ConsumeType(field.type, &scope))
            return false;

        if (TryConsume(Lexer::TokenType::Equals))
        {
            if (!ConsumeValue(field.type, field.defaultValue))
                return false;
        }

        if (!ConsumeAttributes(field.attributes))
            return false;

        return true;
    }

    bool Parser::ConsumeTupleStruct(Declaration& decl)
    {
        if (!Consume(Lexer::TokenType::OpenParens))
            return false;

        decl.kind = DeclKind::Struct;

        if (!TryConsume(Lexer::TokenType::CloseParens))
        {
            do
            {
                Field& field = decl.struct_.fields.EmplaceBack();
                field.declOrder = static_cast<uint16_t>(decl.struct_.fields.Size() - 1);
                field.ordinal = field.declOrder;

                if (!ConsumeIdentifier(field.name))
                    return false;

                if (!ConsumeField(field, decl, false))
                    return false;
            } while (TryConsume(Lexer::TokenType::Comma));

            if (!Consume(Lexer::TokenType::CloseParens))
                return false;
        }

        if (!ValidateAndLayoutStruct(decl))
            return false;

        return true;
    }

    template <typename... Args>
    void Parser::AddError(fmt::format_string<Args...> fmt, Args&&... args)
    {
        ErrorInfo& entry = m_errors.EmplaceBack();
        entry.file = m_fileName;
        entry.line = m_token.line;
        entry.column = m_token.column;
        entry.message.Clear();

        fmt::format_to(Appender(entry.message), fmt, Forward<Args>(args)...);
    }

    template <typename... Args>
    void Parser::AddDeclError(const Declaration& decl, fmt::format_string<Args...> fmt, Args&&... args)
    {
        ErrorInfo& entry = m_errors.EmplaceBack();
        entry.file = decl.source.file;
        entry.line = decl.source.line;
        entry.column = decl.source.column;
        entry.message.Clear();

        fmt::format_to(Appender(entry.message), fmt, Forward<Args>(args)...);
    }

    void Parser::AddLexerError()
    {
        HE_ASSERT(m_token.type == Lexer::TokenType::Error);
        ErrorInfo& entry = m_errors.EmplaceBack();
        entry.file = m_fileName;
        entry.line = m_token.line;
        entry.column = m_token.column;
        entry.message = m_token.error;
    }

    bool Parser::DecodeString()
    {
        m_scratchString.Clear();

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
                            m_scratchString += '\'';
                            s++;
                            break;
                        case '\"': // double quote
                            m_scratchString += '\"';
                            s++;
                            break;
                        case '\\': // backslash
                            m_scratchString += '\\';
                            s++;
                            break;
                        case 'b': // backspace
                            m_scratchString += '\b';
                            s++;
                            break;
                        case 'f': // form feed - new page
                            m_scratchString += '\f';
                            s++;
                            break;
                        case 'n': // line feed - new line
                            m_scratchString += '\n';
                            s++;
                            break;
                        case 'r': // carriage return
                            m_scratchString += '\r';
                            s++;
                            break;
                        case 't': // horizontal tab
                            m_scratchString += '\t';
                            s++;
                            break;
                        case 'v': // vertical tab
                            m_scratchString += '\v';
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
                            m_scratchString += value;
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
                                ToUTF8(m_scratchString, ucc);
                                unicodeHighSurrogate = -1;
                            }
                            else
                            {
                                if (unicodeHighSurrogate == -1)
                                {
                                    AddError("Illegal Unicode sequence (unpaired high surrogate) in string literal");
                                    return false;
                                }
                                ToUTF8(m_scratchString, value);
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

                    m_scratchString += c;
                    break;
            }
        }

        if (unicodeHighSurrogate != -1)
        {
            AddError("Illegal Unicode sequence (unpaired high surrogate)");
            return false;
        }

        if (!decodedIsTrivial && !ValidateUTF8(m_scratchString.Data()))
        {
            AddError("Illegal UTF-8 sequence");
            return false;
        }

        return true;
    }

    template <std::integral T>
    bool Parser::DecodeInt(StringView s, T& out)
    {
        const char* begin = s.begin();
        const char* end = s.end();

        int32_t base = 10;
        if (*begin == '0' && s.Size() > 1)
        {
            ++begin;
            switch (*begin)
            {
                case 'x':
                    ++begin;
                    base = 16;
                    break;
                case 'o':
                    ++begin;
                    base = 8;
                    break;
                case 'b':
                    ++begin;
                    base = 2;
                    break;
            }
        }

        using LargestType = std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>;
        LargestType value = String::ToInteger<T>(begin, &end, base);

        if (value > std::numeric_limits<T>::max() || value < std::numeric_limits<T>::min())
        {
            AddError("Integer value out of range");
            return false;
        }

        out = static_cast<T>(value);

        return true;
    }

    Parser::TypeParamRef Parser::FindTypeParam(StringView name, const Declaration& scope)
    {
        for (uint32_t i = 0; i < scope.typeParams.Size(); ++i)
        {
            if (name == scope.typeParams[i])
                return { &scope, i };
        }

        if (scope.parentId != m_schema.root.id)
        {
            const Declaration* parent = FindDecl(scope.parentId);
            HE_ASSERT(parent);
            return FindTypeParam(name, *parent);
        }

        return {};
    }

    const Declaration* Parser::FindDecl(StringView name) const
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
                auto it = m_importsByNamespace.find(searchNamespace);
                if (it != m_importsByNamespace.end())
                {
                    for (const SchemaFile* schema : it->second)
                    {
                        const Declaration* entry = FindDecl(searchName, schema->root);
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
        const Declaration* localEntry = FindDecl(name, m_schema.root);
        if (localEntry)
            return localEntry;

        // Walk up the parts of our namespace and search each of them for the definition.
        StringView searchNamespace = m_schema.root.name;
        while (!searchNamespace.IsEmpty())
        {
            auto it = m_importsByNamespace.find(searchNamespace);
            if (it != m_importsByNamespace.end())
            {
                for (const SchemaFile* schema : it->second)
                {
                    const Declaration* entry = FindDecl(name, schema->root);
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

    const Declaration* Parser::FindDecl(StringView name, const Declaration& scope) const
    {
        const char* begin = name.Begin();
        const char* dot = &name.Back();
        while (dot > begin && *dot != '.')
            --dot;

        // No dot found, just search for the symbol name
        if (dot == begin)
        {
            for (const Declaration& decl : scope.children)
            {
                if (decl.name == name)
                {
                    return &decl;
                }
            }

            return nullptr;
        }

        // Dot found, this is potentially an embedded type. Let's get the first name and search
        // for that as a struct or interface. If we find it, we'll recurse for the rest.
        StringView parentName(begin, dot);

        const Declaration* parent = nullptr;
        for (const Declaration& decl : scope.children)
        {
            if (decl.name == parentName)
            {
                parent = &decl;
                break;
            }
        }

        if (parent)
            return FindDecl(StringView(parentName.End() + 1, name.End()), *parent);

        return nullptr;
    }

    const Declaration* Parser::FindDecl(TypeId id, const SchemaFile* schema) const
    {
        if (schema == nullptr)
        {
            // Fast check for a non-existant ID
            if (!m_nameMap.contains(id))
                return nullptr;

            schema = &m_schema;
        }

        const Declaration* decl = FindDecl(id, schema->root);
        if (decl)
            return decl;

        for (const Import& im : schema->imports)
        {
            decl = FindDecl(id, im.schema);
            if (decl)
                return decl;
        }

        return nullptr;
    }

    const Declaration* Parser::FindDecl(TypeId id, const Declaration& scope) const
    {
        for (const Declaration& decl : scope.children)
        {
            if (decl.id == id)
                return &decl;

            const Declaration* child = FindDecl(id, decl);
            if (child)
                return child;
        }

        return nullptr;
    }


    const Declaration* Parser::FindForwardDecl(StringView name) const
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
                auto it = m_importsByNamespace.find(searchNamespace);
                if (it != m_importsByNamespace.end())
                {
                    for (const SchemaFile* schema : it->second)
                    {
                        const Declaration* entry = FindForwardDecl(searchName, schema->root);
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
        const Declaration* localEntry = FindForwardDecl(name, m_schema.root);
        if (localEntry)
            return localEntry;

        // Walk up the parts of our namespace and search each of them for the definition.
        StringView searchNamespace = m_schema.root.name;
        while (!searchNamespace.IsEmpty())
        {
            auto it = m_importsByNamespace.find(searchNamespace);
            if (it != m_importsByNamespace.end())
            {
                for (const SchemaFile* schema : it->second)
                {
                    const Declaration* entry = FindForwardDecl(name, schema->root);
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

    const Declaration* Parser::FindForwardDecl(StringView name, const Declaration& scope) const
    {
        const char* begin = name.Begin();
        const char* dot = &name.Back();
        while (dot > begin && *dot != '.')
            --dot;

        // No dot found, just search for the symbol name
        if (dot == begin)
        {
            for (const Declaration& decl : scope.forwards)
            {
                if (decl.name == name)
                {
                    return &decl;
                }
            }

            return nullptr;
        }

        // Dot found, this is potentially an embedded type. Let's get the first name and search
        // for that as a struct or interface. If we find it, we'll recurse for the rest.
        StringView parentName(begin, dot);

        const Declaration* parent = nullptr;
        for (const Declaration& decl : scope.forwards)
        {
            if (decl.name == parentName)
            {
                parent = &decl;
                break;
            }
        }

        if (parent)
            return FindForwardDecl(StringView(parentName.End() + 1, name.End()), *parent);

        return nullptr;
    }

    const Declaration* Parser::FindForwardDecl(TypeId id, const SchemaFile* schema) const
    {
        if (schema == nullptr)
        {
            // Fast check for a non-existant ID
            if (!m_forwardIds.contains(id))
                return nullptr;

            schema = &m_schema;
        }

        const Declaration* decl = FindForwardDecl(id, schema->root);
        if (decl)
            return decl;

        for (const Import& im : schema->imports)
        {
            decl = FindForwardDecl(id, im.schema);
            if (decl)
                return decl;
        }

        return nullptr;
    }

    const Declaration* Parser::FindForwardDecl(TypeId id, const Declaration& scope) const
    {
        for (const Declaration& decl : scope.forwards)
        {
            if (decl.id == id)
                return &decl;

            const Declaration* child = FindForwardDecl(id, decl);
            if (child)
                return child;
        }

        return nullptr;
    }

    bool Parser::StoreDeclId(Declaration& decl)
    {
        if (decl.id == 0)
        {
            decl.id = FNV64::HashStringN(decl.name.Data(), decl.name.Size(), decl.parentId);

            if (decl.id == 0)
            {
                AddError("Declaration has a fully qualified name that hashes to the invalid type id value (0), which is reserved; try a different name");
                return false;
            }

            decl.id |= TypeIdFlag;
        }

        const auto result = m_nameMap.emplace(decl.id, decl.name);
        if (!result.second)
        {
            AddError("Declaration '{}' has an ID that collides with another declaration '{}'.", decl.name, result.first->second);
            return false;
        }

        return true;
    }

    bool Parser::ValidateAndLayoutStruct(Declaration& decl)
    {
        HE_ASSERT(decl.kind == DeclKind::Struct);

        // No layout required
        if (decl.struct_.fields.IsEmpty())
            return true;

        StructLayout layout(decl);

        const Span<const StructLayout::FieldRef> sortedFields = layout.GetSortedFields();

        if (sortedFields[0].field->ordinal != 0)
        {
            AddError("Fields in struct '{}' skipped ordinal @0. Ordinals must be sequential, without any gaps, starting at zero.", decl.name);
            return false;
        }

        // Check for duplicate or gapped ordinal values
        const uint32_t size = sortedFields.Size();
        for (uint32_t i = 0; i < size; ++i)
        {
            const StructLayout::FieldRef& a = sortedFields[i];

            if (i < (size - 1))
            {
                const StructLayout::FieldRef& b = sortedFields[i + 1];
                const uint32_t diff = b.field->ordinal - a.field->ordinal;

                if (diff == 0)
                {
                    AddError("Multiple fields in struct '{}' have the ordinal @{}", decl.name, a.field->ordinal);
                    return false;
                }

                if (diff > 1)
                {
                    AddError("Fields in struct '{}' skipped ordinal @{}. Ordinals must be sequential, without any gaps, starting at zero.", decl.name, a.field->ordinal + 1);
                    return false;
                }
            }
        }

        if (!ValidateStructFieldNames(decl))
            return false;

        layout.CalculateLayout();
        return true;
    }

    bool Parser::ValidateStructFieldNames(const Declaration& decl)
    {
        // Check for duplicate names
        const uint32_t size = decl.struct_.fields.Size();
        for (uint32_t i = 0; i < size; ++i)
        {
            const Field& a = decl.struct_.fields[i];
            for (uint32_t j = i + 1; j < size; ++j)
            {
                const Field& b = decl.struct_.fields[j];

                if (a.name == b.name)
                {
                    AddError("Multiple fields in struct '{}' have the name '{}'", decl.name, a.name);
                    return false;
                }
            }

            if (a.isGroup || a.isUnion)
            {
                const Declaration* group = FindDecl(a.typeId, decl);
                HE_ASSERT(group && group->kind == DeclKind::Struct);
                if (!ValidateStructFieldNames(*group))
                    return false;
            }
        }

        return true;
    }
}

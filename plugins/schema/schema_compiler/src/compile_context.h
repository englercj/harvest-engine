// Copyright Chad Engler

#pragma once

#include "compiler.h"
#include "parser.h"
#include "verifier.h"

#include "he/core/log.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/ast.h"
#include "he/schema/compile_session.h"
#include "he/schema/schema.h"

#include "fmt/core.h"

#include <set>
#include <unordered_map>

namespace he::schema
{
    class CompileContext
    {
    public:
        CompileContext(const char* path, const CompileSession::Config& config, TypeIdMap& typeIdMap, TypeMap& typeMap)
            : m_path(path)
            , m_config(config)
            , m_typeIdMap(typeIdMap)
            , m_typeMap(typeMap)
        {}

        const CompileSession::Config& Config() const { return m_config; }
        StringView Path() const { return m_path; }
        bool IsFullyParsed() const { return m_fullyParsed; }
        void MarkFullyParsed() { m_fullyParsed = true; }

        const AstFile& Ast() const { return m_parser.Ast(); }
        const Builder& Schema() const { return m_compiler.Schema(); }

        bool LoadFile();
        bool ParseFile() { return m_parser.Parse(m_input.Data(), *this); }
        bool VerifyFile() { return m_verifier.Verify(m_parser.Ast(), *this); }
        bool CompileFile() { return m_compiler.Compile(m_parser.Ast(), *this); }

        const AstNode* FindNode(TypeId id) const;
        const AstNode* FindNode(const AstExpression& name, const AstNode& scope) const;
        const AstNode* FindNode(StringView name, const AstNode& scope) const;

        bool TrackTypeId(const AstNode& node);
        bool TrackType(const TypeKey& key, const TypeValue& value);
        const TypeValue& GetType(const TypeKey& key) const { return m_typeMap.at(key); }

        bool DecodeString(const AstExpression& ast, he::String& out);

        void AddImport(CompileContext* import) { m_imports.PushBack(import); }

        template <typename... Args>
        void AddError(const AstFileLocation& loc, fmt::format_string<Args...> fmt, Args&&... args)
        {
            AddError(loc.line, loc.column, fmt, Forward<Args>(args)...);
        }

        template <typename... Args>
        void AddError(uint32_t line, uint32_t column, fmt::format_string<Args...> fmt, Args&&... args)
        {
            HE_LOG_ERROR(schema_compiler,
                HE_MSG(fmt, Forward<Args>(args)...),
                HE_KV(line, line),
                HE_KV(column, column),
                HE_KV(file, m_path));
        }

    private:
        he::String m_path;
        const CompileSession::Config& m_config;
        TypeIdMap& m_typeIdMap;
        TypeMap& m_typeMap;

        Vector<char> m_input{};
        Vector<CompileContext*> m_imports{};
        bool m_fullyParsed{ false };
        Parser m_parser{};
        Verifier m_verifier{};
        Compiler m_compiler{};
    };
}

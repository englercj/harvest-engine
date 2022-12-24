// Copyright Chad Engler

#pragma once

#include "he/core/hash_table.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/schema/ast.h"
#include "he/schema/codegen.h"
#include "he/schema/compile_types.h"
#include "he/schema/layout.h"
#include "he/schema/schema.h"

namespace he::schema
{
    class CompileContext;

    class CompileSession
    {
    public:
        struct Config
        {
            Span<const char* const> includeDirs{};
            bool includeSourceInfo{ false };
            const char* codegenOutDir{ nullptr };
            CodegenTarget codegenTargets{ CodegenTarget::None };
        };

    public:
        CompileSession(const char* path, const Config& config) noexcept;
        ~CompileSession() noexcept;

        bool Parse();
        bool Verify();
        bool Compile();
        bool GenerateCode();

    private:
        bool ParseFile(CompileContext& ctx);
        bool ParseImport(const AstExpression& ast, CompileContext& ctx);
        CompileContext* TryFindCachedImport(const he::String& path) const;

        bool VerifyAll() const;
        bool CompileAll();
        bool Compile(CompileContext& ctx);

    private:
        enum class Stage
        {
            None,
            Parsed,
            Verified,
            Compiled,
        };

    private:
        const char* m_path{ nullptr };
        Config m_config{};
        Stage m_stage{ Stage::None };

        CompileContext* m_context{};

        TypeIdMap m_typeIdMap{};
        TypeMap m_typeMap{};
        DeclIdMap m_declIdMap{};
        HashMap<he::String, CompileContext*> m_importMap{};
    };
}

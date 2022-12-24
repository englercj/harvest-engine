// Copyright Chad Engler

#include "he/schema/compile_session.h"

#include "compile_context.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/directory.h"
#include "he/core/path.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/schema/ast.h"

namespace he::schema
{
    CompileSession::CompileSession(const char* path, const Config& config) noexcept
        : m_config(config)
        , m_context(nullptr)
    {
        m_context = Allocator::GetDefault().New<CompileContext>(path, config, m_typeIdMap, m_typeMap, m_declIdMap);
    }

    CompileSession::~CompileSession() noexcept
    {
        Allocator& alloc = Allocator::GetDefault();

        alloc.Delete(m_context);
        for (auto&& entry : m_importMap)
        {
            alloc.Delete(entry.value);
        }
    }

    bool CompileSession::Parse()
    {
        HE_ASSERT(m_stage == Stage::None);

        if (!ParseFile(*m_context))
            return false;

        m_stage = Stage::Parsed;
        return true;
    }

    bool CompileSession::Verify()
    {
        HE_ASSERT(m_stage == Stage::Parsed);

        if (!VerifyAll())
            return false;

        m_stage = Stage::Verified;
        return true;
    }

    bool CompileSession::Compile()
    {
        HE_ASSERT(m_stage == Stage::Verified);

        if (!CompileAll())
            return false;

        m_stage = Stage::Compiled;
        return true;
    }

    bool CompileSession::GenerateCode()
    {
        if (!he::String::IsEmpty(m_config.codegenOutDir))
        {
            Directory::Create(m_config.codegenOutDir, true);
        }

        const Builder& builder = m_context->Schema();

        CodeGenRequest req;
        req.schemaData = builder;
        req.schemaFile = builder.Root().TryGetStruct<SchemaFile>();
        req.fileName = GetBaseName(m_context->Path()).Data();
        req.outDir = m_config.codegenOutDir;
        req.declsById = &m_declIdMap;

        if (HasFlag(m_config.codegenTargets, CodegenTarget::Echo))
        {
            if (!GenerateEcho(req))
                return false;
        }

        if (HasFlag(m_config.codegenTargets, CodegenTarget::Cpp))
        {
            if (!GenerateCpp(req))
                return false;
        }

        return true;
    }

    bool CompileSession::ParseFile(CompileContext& ctx)
    {
        // Read the input text from the file
        if (!ctx.LoadFile())
            return false;

        // Parse the input data into an AST
        if (!ctx.ParseFile())
            return false;

        // Parse each of the imports
        const AstFile& ast = ctx.Ast();
        HE_ASSERT(ast.root.kind == AstNode::Kind::File);
        for (const AstExpression& import : ast.root.file.imports)
        {
            if (!ParseImport(import, ctx))
                return false;
        }

        ctx.MarkFullyParsed();
        return true;
    }

    bool CompileSession::ParseImport(const AstExpression& ast, CompileContext& ctx)
    {
        he::String path;
        if (!ctx.DecodeString(ast, path))
            return false;

        // Try to find the imported file in the cache as a path relative to our current file.
        he::String fullPath(GetDirectory(ctx.Path()));
        ConcatPath(fullPath, path);
        CompileContext* cached = TryFindCachedImport(fullPath);
        if (cached)
        {
            if (!cached->IsFullyParsed())
            {
                ctx.AddError(ast.location.line, ast.location.column, "Import loop detected.");
                return false;
            }
            ctx.AddImport(cached);
            return true;
        }

        // Try to find the imported file in the cache as a path relative to an include directory.
        for (const char* includeDir : m_config.includeDirs)
        {
            fullPath = includeDir;
            ConcatPath(fullPath, path);

            cached = TryFindCachedImport(fullPath);
            if (cached)
            {
                if (!cached->IsFullyParsed())
                {
                    ctx.AddError(ast.location.line, ast.location.column, "Import loop detected.");
                    return false;
                }
                ctx.AddImport(cached);
                return true;
            }
        }

        // Try to find the imported file relative to our current file.
        fullPath = GetDirectory(ctx.Path());
        ConcatPath(fullPath, path);
        if (File::Exists(fullPath.Data()))
        {
            CompileContext* importCtx = Allocator::GetDefault().New<CompileContext>(fullPath.Data(), m_config, m_typeIdMap, m_typeMap, m_declIdMap);
            m_importMap[fullPath] = importCtx;
            ctx.AddImport(importCtx);
            return ParseFile(*importCtx);
        }

        // Try to find the imported file relative to an include directory.
        for (const char* includeDir : m_config.includeDirs)
        {
            fullPath = includeDir;
            ConcatPath(fullPath, path);

            if (File::Exists(fullPath.Data()))
            {
                CompileContext* importCtx = Allocator::GetDefault().New<CompileContext>(fullPath.Data(), m_config, m_typeIdMap, m_typeMap, m_declIdMap);
                m_importMap[fullPath] = importCtx;
                ctx.AddImport(importCtx);
                return ParseFile(*importCtx);
            }
        }

        ctx.AddError(ast.location.line, ast.location.column, "Unable to locate imported file '{}', are you missing an include directory?", path);
        return false;
    }

    CompileContext* CompileSession::TryFindCachedImport(const he::String& path) const
    {
        CompileContext* const* ctx = m_importMap.Find(path);
        return ctx ? *ctx: nullptr;
    }

    bool CompileSession::VerifyAll() const
    {
        if (!m_context->VerifyFile())
            return false;

        for (auto&& entry : m_importMap)
        {
            if (!entry.value->VerifyFile())
                return false;
        }

        return true;
    }

    bool CompileSession::CompileAll()
    {
        return Compile(*m_context);
    }

    bool CompileSession::Compile(CompileContext& ctx)
    {
        if (ctx.IsFullyCompiled())
            return true;

        for (CompileContext* importCtx : ctx.Imports())
        {
            if (!Compile(*importCtx))
                return false;
        }

        if (!ctx.CompileFile())
            return false;

        ctx.MarkFullyCompiled();
        return true;
    }
}

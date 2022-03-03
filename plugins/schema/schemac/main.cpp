// Copyright Chad Engler

#include "he/core/args.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/vector.h"
#include "he/schema/codegen.h"
#include "he/schema/compiler.h"
#include "he/schema/lexer.h"
#include "he/schema/parser.h"

#include <filesystem>
#include <iostream>

using namespace he;

struct AppArgs
{
    bool help{ false };
    const char* outDir{ nullptr };
    Vector<const char*> targets{};
    Vector<const char*> includeDirs{};
};

static std::unordered_map<uint32_t, schema::Compiler> s_compilerCache;

static bool CompileFile(const char* path, const AppArgs& args)
{
    using namespace he;

    // Read the input text from the file
    Vector<char> input;
    Result r = File::ReadAll(input, path);
    if (!HE_VERIFY(r))
    {
        HE_LOG_ERROR(schemac, HE_MSG("Failed to read file"), HE_KV(result, r), HE_KV(path, path));
        return false;
    }
    input.PushBack('\0');

    // Initialize the lexer from the source text
    schema::Lexer lexer;
    if (!lexer.Reset(input.Data()))
    {
        schema::Lexer::Token error = lexer.PeekNextToken();
        std::cerr << path << '(' << error.line << ',' << error.column << "): error 0: " << error.error << std::endl;
        return false;
    }

    // Parse the lexer's token stream into an AST
    schema::Parser parser(lexer);
    if (!parser.Parse())
    {
        for (const schema::Parser::ErrorInfo& info : parser.Errors())
        {
            std::cerr << path << '(' << info.line << ',' << info.column << "): error 0: " << info.message.Data() << std::endl;
        }
        return false;
    }

    // Compile the AST into a schema
    schema::Compiler compiler(parser.Ast(), path, {});
    if (!compiler.Compile())
    {
        for (const schema::Compiler::ErrorInfo& info : compiler.Errors())
        {
            std::cerr << path << '(' << info.line << ',' << info.column << "): error 0: " << info.message.Data() << std::endl;
        }
        return false;
    }

    // -------------------------------------------
    // TODO: Compile imports
    // -------------------------------------------

    // Generate code from the schema
    const StringView fname = GetBaseName(path);

    schema::CodeGenRequest req(compiler.Schema());
    req.fileName = fname.Data();
    req.outDir = args.outDir;

    if (!String::IsEmpty(args.outDir))
    {
        Directory::Create(args.outDir, true);
    }

    for (const char* target : args.targets)
    {
        if (String::Equal(target, "cpp") || String::Equal(target, "c++"))
        {
            if (!schema::GenerateCpp(req))
            {
                std::cerr << "Failed to generate C++ code." << std::endl;
                return false;
            }
        }
        else if (String::Equal(target, "echo"))
        {
            if (!schema::GenerateEcho(req))
            {
                std::cerr << "Failed to generate Echo output." << std::endl;
                return false;
            }
        }
    }

    return true;
}

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    AppArgs args;

    ArgDesc ArgDescriptors[] =
    {
        { args.help,        'h', "help",        "Output this help text" },
        { args.outDir,      'o', "out",         "Output directory to write generated files" },
        { args.targets,     't', "target",      "Target language to generate definitions for" },
        { args.includeDirs, 'I', "include",     "Path to search for import declarations" },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help || result.values.IsEmpty())
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    String fullPath;
    String dirStorage;

    for (const char* param : result.values)
    {
        fullPath = param;
        Result r = MakeAbsolute(fullPath);
        if (!HE_VERIFY(r))
        {
            HE_LOG_ERROR(schemac,
                HE_MSG("Failed to get the full path to input file."),
                HE_KV(result, r),
                HE_KV(input_path, param));
            return -1;
        }

        const StringView dir = GetDirectory(fullPath);
        if (!dir.IsEmpty())
        {
            dirStorage = dir;
            args.includeDirs.PushFront(dirStorage.Data());
        }

        if (!CompileFile(fullPath.Data(), args))
            return -1;

        if (!dir.IsEmpty())
        {
            args.includeDirs.PopFront();
        }
    }

    return 0;
}

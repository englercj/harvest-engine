// Copyright Chad Engler

#include "he/core/args.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/vector.h"
#include "he/schema/codegen.h"
#include "he/schema/compile_session.h"

#include <filesystem>
#include <iostream>

using namespace he;

struct AppArgs
{
    bool help{ false };
    const char* outDir{ nullptr };
    Vector<const char*> targets{};
    Vector<const char*> includeDirs{};
    bool includeSourceInfo{ false };
};

static bool CompileFile(const char* path, const AppArgs& args)
{
    using namespace he;

    schema::CompileSession::Config config;
    config.includeDirs = args.includeDirs;
    config.includeSourceInfo = args.includeSourceInfo;
    config.codegenOutDir = args.outDir;

    for (const char* target : args.targets)
    {
        if (String::Equal(target, "cpp") || String::Equal(target, "c++"))
        {
            config.codegenTargets |= schema::CodegenTarget::Cpp;
        }
        else if (String::Equal(target, "echo"))
        {
            config.codegenTargets |= schema::CodegenTarget::Echo;
        }
    }

    schema::CompileSession session(path, config);
    if (!session.Parse())
        return false;

    if (!session.Verify())
        return false;

    if (!session.Compile())
        return false;

    if (!session.GenerateCode())
        return false;

    return true;
}

static void LogToStdOut(const void*, const LogSource& source, const KeyValue* kvs, uint32_t count)
{
    // We specially format the schema_compiler output so it gets picked up by the error window.
    if (String::Equal(source.category, "schema_compiler") && count >= 4)
    {
        uint64_t line = 0;
        uint64_t column = 0;
        const String* msg = nullptr;
        const String* file = nullptr;

        for (uint32_t i = 0; i < count; ++i)
        {
            const KeyValue& kv = kvs[i];
            if (String::Equal(kv.Key(), HE_MSG_KEY))
                msg = &kv.GetString();
            else if (String::Equal(kv.Key(), "file"))
                file = &kv.GetString();
            else if (String::Equal(kv.Key(), "line"))
                line = kv.GetUint();
            else if (String::Equal(kv.Key(), "column"))
                column = kv.GetUint();
        }

        if (msg && file)
        {
            std::ostream& out = source.level >= LogLevel::Error ? std::cerr : std::cout;
            out << file->Data() << '(' << line << ',' << column << "): error 0: " << msg->Data() << std::endl;
            return;
        }
    }

    ConsoleSink(nullptr, source, kvs, count);
}

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    AddLogSink(LogToStdOut);

    AppArgs args;

    ArgDesc ArgDescriptors[] =
    {
        { args.help,        'h', "help",        "Output this help text" },
        { args.outDir,      'o', "out",         "Output directory to write generated files", ArgFlag::Required },
        { args.targets,     't', "target",      "Target language to generate definitions for", ArgFlag::Required },
        { args.includeDirs, 'I', "include",     "Path to search for import declarations" },
        { args.includeSourceInfo, "--src-info", "Include source info in compiled schema (file, line column)" },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help || result.values.IsEmpty())
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    Result r = Directory::Create(args.outDir, true);
    if (!r)
    {
        HE_LOG_ERROR(he_schemac, HE_MSG("Failed to create output directory."), HE_KV(path, args.outDir), HE_KV(result, r));
        return -1;
    }

    String fullPath;

    for (const char* param : result.values)
    {
        fullPath = param;
        r = MakeAbsolute(fullPath);
        if (!r)
        {
            HE_LOG_ERROR(schema_compiler,
                HE_MSG("Failed to get the full path to input file."),
                HE_KV(result, r),
                HE_KV(input_path, param));
            return -1;
        }

        if (!CompileFile(fullPath.Data(), args))
            return -1;
    }

    return 0;
}

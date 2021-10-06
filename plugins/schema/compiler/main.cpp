// Copyright Chad Engler

#include "he/core/args.h"
#include "he/core/file.h"
#include "he/core/path.h"
#include "he/core/vector.h"
#include "he/schema/codegen.h"
#include "he/schema/parser.h"

#include <iostream>

struct AppArgs
{
    bool help{ false };
    bool grpc{ false };
    bool json{ false };
    bool buffer{ false };
    const char* outDir{ nullptr };
    const char* lang{ nullptr };
    he::Vector<const char*> includeDirs{};
};

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    AppArgs args;

    ArgDesc ArgDescriptors[] =
    {
        { args.help,        'h', "help",    "Output this help text" },
        { args.outDir,      'o', "out",     "Output directory to write generated files" },
        { args.lang,        'g', "gen",     "Language to generate definitions for" },
        { args.includeDirs, 'I', "include", "Path to search for import declarations" },
        { args.grpc,             "grpc",    "Generate GRPC interfaces" },
        { args.json,        'j', "json",    "Enable code generation for JSON serialization" },
        { args.buffer,      'b', "buffer",  "Enable code generation for zero-copy buffers" },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help)
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    String fullPath;

    bool first = true;

    for (const char* param : result.values)
    {
        fullPath = param;

        if (!first)
            args.includeDirs.PopFront();

        first = false;

        const String dir(GetDirectory(fullPath));
        args.includeDirs.PushFront(dir.Data());

        schema::Parser parser;

        if (!parser.ParseFile(fullPath.Data(), args.includeDirs))
        {
            for (const auto& info : parser.GetErrors())
            {
                std::cerr << info.file.Data() << '(' << info.line << ',' << info.column << "): error 0:" << info.message.Data() << std::endl;
            }

            return -1;
        }

        const StringView fname = GetBaseName(param);

        schema::CodeGenOptions options{};
        options.fileName = fname.Data();
        options.outDir = args.outDir;
        options.json = args.json;
        options.buffer = args.buffer;
        if (!schema::GenerateCpp(parser.GetSchema(), options))
        {
            std::cerr << "Failed to generate C++ code." << std::endl;
            return -1;
        }
    }

    return 0;
}

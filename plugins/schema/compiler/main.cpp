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
    AppArgs(he::Allocator& allocator) : includeDirs(allocator) {}

    bool help{ false };
    bool grpc{ false };
    const char* outDir{};
    he::Vector<const char*> includeDirs;
};

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    Allocator& alloc = CrtAllocator::Get();
    AppArgs args(alloc);

    ArgDesc ArgDescriptors[] =
    {
        { args.help,        'h', "help",    "Output this help text" },
        { args.outDir,      'o', "out",     "Output directory to write generated files" },
        { args.grpc,             "grpc",    "Generate GRPC interfaces" },
        { args.includeDirs, 'I', "include", "Path to search for import declarations" },
        // { ..., 'g', "gen", "Language to generate definitions for" },
    };

    ArgResult result = ParseArgs(alloc, ArgDescriptors, argc, argv);

    if (!result || args.help)
    {
        String help = MakeHelpString(alloc, ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    String fullPath(alloc);

    bool first = true;

    for (const char* param : result.values)
    {
        fullPath = param;

        if (!first)
            args.includeDirs.PopFront();

        first = false;

        const String dir(alloc, GetDirectory(fullPath));
        args.includeDirs.PushFront(dir.Data());

        schema::Parser parser(alloc);

        if (!parser.ParseFile(fullPath.Data(), args.includeDirs))
        {
            for (const auto& info : parser.GetErrors())
            {
                std::cerr << info.file.Data() << '(' << info.line << ',' << info.column << "): error 0:" << info.message.Data() << std::endl;
            }

            return -1;
        }

        const StringView fname = GetBaseName(param);

        schema::CodeWriter output(alloc);
        schema::CodeGenOptions options{};
        options.fileName = fname.Data();
        options.outDir = args.outDir;
        if (!schema::GenerateCpp(parser.GetSchema(), options, output))
        {
            std::cerr << "Failed to generate C++ code." << std::endl;
            return -1;
        }
    }

    return 0;
}

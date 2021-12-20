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
    bool reflection{ false };
    bool zeroCopy{ false };
    const char* outDir{ nullptr };
    he::Vector<const char*> targets{};
    he::Vector<const char*> includeDirs{};
};

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
        { args.grpc,             "grpc",        "Generate GRPC interfaces" },
        { args.reflection,  'r', "reflection",  "Enable code generation for native reflection information, disabled if zero-copy is set" },
        { args.json,        'j', "json",        "Enable code generation for JSON serialization" },
        { args.buffer,      'b', "buffer",      "Enable code generation for Buffer serialization" },
        { args.zeroCopy,    'z', "zero-copy",   "Enable code generation for zero-copy buffers only (no native)" },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help)
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    String fullPath;
    String dirStorage;

    bool first = true;

    for (const char* param : result.values)
    {
        fullPath = param;

        if (!first)
            args.includeDirs.PopFront();

        first = false;

        const StringView dir = GetDirectory(fullPath);
        if (!dir.IsEmpty())
        {
            dirStorage = dir;
            args.includeDirs.PushFront(dirStorage.Data());
            fullPath = GetBaseName(fullPath);
        }

        schema::Parser parser;

        if (!parser.ParseFile(fullPath.Data(), args.includeDirs))
        {
            for (const auto& info : parser.GetErrors())
            {
                std::cerr << info.file.Data() << '(' << info.line << ',' << info.column << "): error 0: " << info.message.Data() << std::endl;
            }

            return -1;
        }

        const StringView fname = GetBaseName(param);

        schema::CodeGenRequest req(parser.GetSchema());
        req.fileName = fname.Data();
        req.outDir = args.outDir;

        for (const char* target : args.targets)
        {
            if (String::Equal(target, "cpp") || String::Equal(target, "c++"))
            {
               if (!schema::GenerateCpp(req))
               {
                   std::cerr << "Failed to generate C++ code." << std::endl;
                   return -1;
               }
            }
            else if (String::Equal(target, "echo"))
            {
                if (!schema::GenerateEcho(req))
                {
                    std::cerr << "Failed to generate Echo output." << std::endl;
                    return -1;
                }
            }
        }
    }

    return 0;
}

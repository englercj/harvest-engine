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
    he::StringView outDir{ nullptr };
    he::Vector<he::StringView> includeDirs;
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

    String fileName(alloc);

    for (const StringView& paramValue : result.values)
    {
        fileName.Assign(paramValue.Data(), paramValue.Size());

        Vector<char> input(alloc);
        {
            File f;
            if (!f.Open(fileName.Data(), FileOpenMode::ReadExisting))
                return 2;

            const uint32_t size = static_cast<uint32_t>(f.GetSize()) + 1;
            input.Resize(size);

            if (!f.Read(input.Data(), input.Size()))
                return 3;

            input.Back() = '\0';

            f.Close();
        }

        schema::Parser parser(alloc);
        if (!parser.Parse(input.Data(), nullptr))
        {
            for (const auto& info : parser.GetErrors())
            {
                std::cerr << '(' << info.line << ", " << info.column << "): " << info.message.Data() << std::endl;
            }

            return 1;
        }

        schema::CodeWriter output(alloc);
        schema::CodeGenOptions options{};
        if (!schema::GenerateCpp(parser.GetSchema(), options, output))
        {
            std::cerr << "Failed to generate C++ code." << std::endl;
            return 1;
        }

        RemoveExtension(fileName);
        fileName += "_generated.h";

        {
            File f;
            if (!f.Open(fileName.Data(), FileOpenMode::WriteTruncate))
                return 2;

            const StringView out = output.Str();
            if (!f.Write(out.Data(), out.Size()))
                return 3;

            f.Close();
        }
    }

    return 0;
}

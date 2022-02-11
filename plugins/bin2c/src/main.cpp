// Copyright Chad Engler

#include "console_sink.h"
#include "stb_compress.h"
#include "write_file_data.h"

#include "he/core/allocator.h"
#include "he/core/args.h"
#include "he/core/ascii.h"
#include "he/core/compiler.h"
#include "he/core/file.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/result.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/log.h"

#include <iostream>

struct AppArgs
{
    bool help{ false };
    bool text{ false };
    bool compress{ false };
    const char* input{};
    const char* output{};
    const char* name{};
};

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    AddLogSink(ConsoleSink);

    AppArgs args;

    ArgDesc ArgDescriptors[] =
    {
        { args.help,        'h', "help",        "Output this help text." },
        { args.text,        't', "text",        "Treat the input file as text, and output a string." },
        { args.compress,    'c', "compress",    "Compress the input file using stb_compress." },
        { args.input,       'f', "file",        "Input file to process." },
        { args.output,      'o', "out",         "Output file to write to." },
        { args.name,        'n', "name",        "Name of the constant in the output file." },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help || String::IsEmpty(args.input) || String::IsEmpty(args.output))
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    if (String::IsEmpty(args.name))
        args.name = "c_data";

    uint32_t size = 0;

    String fileNameBuf = args.input;

    he::File file;
    if (file.Open(fileNameBuf.Data(), he::FileOpenMode::ReadExisting))
    {
        size = static_cast<uint32_t>(file.GetSize());
        void* data = Allocator::GetTemp().Malloc(size + 4);
        HE_AT_SCOPE_EXIT([&]() { Allocator::GetTemp().Free(data); });

        uint32_t bytesRead = 0;
        he::Result r = file.Read(data, size, &bytesRead);

        if (!HE_VERIFY(bytesRead == size))
            return -1;
        if (!HE_VERIFY_RESULT(r))
            return -1;

        he::MemZero(static_cast<uint8_t*>(data) + size, 4);

        file.Close();

        void* output = data;
        uint32_t outputSize = size;
        if (args.compress)
        {
            // maxLen guess from stb_compress_intofile in stb.h
            uint32_t maxLen = size + 512 + (size >> 2) + sizeof(int); // just guessing
            output = Allocator::GetTemp().Malloc(maxLen);
            outputSize = stb_compress(static_cast<stb_uchar*>(output), static_cast<stb_uchar*>(data), size);
            he::MemZero(static_cast<uint8_t*>(output) + outputSize, maxLen - outputSize);
        }

        fileNameBuf = args.output;
        he::File outFile;
        if (outFile.Open(fileNameBuf.Data(), he::FileOpenMode::WriteTruncate))
        {
            WriteFileData(outFile, args.name, static_cast<uint8_t*>(output), outputSize, args.text);
            outFile.Close();
        }

        if (args.compress)
        {
            Allocator::GetTemp().Free(output);
        }
    }

    return 0;
}

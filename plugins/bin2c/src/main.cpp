// Copyright Chad Engler

#include "stb_compress.h"
#include "write_file_data.h"

#include "he/core/allocator.h"
#include "he/core/args.h"
#include "he/core/ascii.h"
#include "he/core/compiler.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/vector.h"

#include <iostream>

struct AppArgs
{
    bool help{ false };
    bool text{ false };
    bool compress{ false };
    const char* input{ nullptr };
    const char* output{ nullptr };
    const char* name{ nullptr };
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
        { args.input,       'f', "file",        "Input file to process.", ArgFlag::Required },
        { args.output,      'o', "out",         "Output file to write to.", ArgFlag::Required },
        { args.name,        'n', "name",        "Name of the constant in the output file." },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help)
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    if (String::IsEmpty(args.name))
        args.name = "c_data";

    Vector<uint8_t> fileData;
    Result r = File::ReadAll(fileData, args.input);
    if (!r)
    {
        HE_LOG_ERROR(he_bin2c, HE_MSG("Failed to load input file."), HE_KV(path, args.input), HE_KV(result, r));
        return -1;
    }

    // fileData.Resize(fileData.Size() + 4); // huh?

    uint8_t* output = fileData.Data();
    uint32_t outputSize = fileData.Size();
    if (args.compress)
    {
        // maxLen guess from stb_compress_intofile in stb.h
        const uint32_t maxLen = fileData.Size() + 512 + (fileData.Size() >> 2) + sizeof(int);
        output = Allocator::GetDefault().Malloc<uint8_t>(maxLen);
        outputSize = stb_compress(output, fileData.Data(), fileData.Size());
        he::MemZero(output + outputSize, maxLen - outputSize);
    }

    String dirName = GetDirectory(args.output);
    r = Directory::Create(dirName.Data(), true);
    if (!r)
    {
        HE_LOG_ERROR(he_bin2c, HE_MSG("Failed to create parent directories of output file."), HE_KV(path, args.output), HE_KV(result, r));
        return -1;
    }

    he::File outFile;
    r = outFile.Open(args.output, he::FileOpenMode::WriteTruncate);
    if (!r)
    {
        HE_LOG_ERROR(he_bin2c, HE_MSG("Failed to open output file for writing."), HE_KV(path, args.output), HE_KV(result, r));
        return -1;
    }

    WriteFileData(outFile, args.name, output, outputSize, args.text);
    outFile.Close();

    if (args.compress)
    {
        Allocator::GetDefault().Free(output);
    }

    return 0;
}

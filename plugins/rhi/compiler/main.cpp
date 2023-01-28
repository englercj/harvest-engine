// Copyright Chad Engler

#include "he/core/allocator.h"
#include "he/core/args.h"
#include "he/core/ascii.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/fmt.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string_view.h"
#include "he/core/vector.h"

#include "slang.h"
#include "slang-com-ptr.h"

#include <iostream>

struct AppArgs
{
    bool help{ false };
    int32_t optLevel{ 1 };
    const char* outDir{ nullptr };
    he::Vector<const char*> defines{};
    he::Vector<const char*> includeDirs{};
    he::Vector<const char*> targets{};
};

static void SlangDiagHandler(const char* msg, void*)
{
    std::cout << msg << std::endl;
}

void WriteFileData(he::File& file, he::StringView name, const uint8_t* data, size_t size, bool asText);

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    AppArgs args;

    ArgDesc ArgDescriptors[] =
    {
        { args.help,        'h', "help",        "Output this help text" },
        { args.outDir,      'o', "out",         "Output directory to write generated files", ArgFlag::Required },
        { args.optLevel,    'O', "opt-level",   "Optimization level to apply. Default is 1, min is 0, max is 3" },
        { args.defines,     'D', "define",      "Define a symbol for the preprocessor" },
        { args.includeDirs, 'I', "include",     "Path to search for import declarations" },
        { args.targets,     't', "target",      "Target profile to generate for" },
    };

    ArgResult result = ParseArgs(ArgDescriptors, argc, argv);

    if (!result || args.help || result.values.Size() != 1 || args.optLevel < 0 || args.optLevel > 3)
    {
        String help = MakeHelpString(ArgDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    Result res = Directory::Create(args.outDir, true);
    if (!res)
    {
        HE_LOG_ERROR(he_schemac, HE_MSG("Failed to create output directory."), HE_KV(path, args.outDir), HE_KV(result, res));
        return -1;
    }

    // Create the global session and compilation request
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    SlangResult r = slang::createGlobalSession(globalSession.writeRef());
    if (SLANG_FAILED(r))
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("Failed to create global slang session."), HE_KV(result, r));
        return -1;
    }

    Slang::ComPtr<slang::ICompileRequest> request;
    r = globalSession->createCompileRequest(request.writeRef());
    if (SLANG_FAILED(r))
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("Failed to create slang compile request."), HE_KV(result, r));
        return -1;
    }

    request->setDiagnosticCallback(SlangDiagHandler, nullptr);
    request->setMatrixLayoutMode(SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
    request->setOptimizationLevel(args.optLevel);
    request->setOutputContainerFormat(SLANG_CONTAINER_FORMAT_NONE);

    Vector<String> defineNamesStorage;
    defineNamesStorage.Reserve(args.defines.Size());
    for (const char* d : args.defines)
    {
        const char* valueStart = String::Find(d, '=');
        const uint32_t nameLen = valueStart ? static_cast<uint32_t>(valueStart - d) : String::Length(d);

        String& name = defineNamesStorage.EmplaceBack();
        name.Assign(d, nameLen);

        request->addPreprocessorDefine(name.Data(), valueStart ? valueStart + 1 : "1");
    }

    const char* fileName = result.values[0];
    request->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, "");
    request->addTranslationUnitSourceFile(0, fileName);

    Slang::ComPtr<slang::IModule> mod;
    r = request->getModule(0, mod.writeRef());
    if (!mod || SLANG_FAILED(r))
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("Failed to module from compile request."), HE_KV(result, r));
        return -1;
    }

    const he::String fileDir = GetDirectory(fileName);
    request->addSearchPath(fileDir.Data());

    for (const char* includeDir : args.includeDirs)
    {
        request->addSearchPath(includeDir);
    }

    for (const char* target : args.targets)
    {
        SlangCompileTarget codegenTarget;
        if (String::Find(target, "sm_") == target)
        {
            codegenTarget = SLANG_DXBC;
        }
        else if (String::Find(target, "glsl_") == target)
        {
            codegenTarget = SLANG_SPIRV;
        }
        else
        {
            HE_LOG_ERROR(he_shaderc,
                HE_MSG("Unknown compilation target. Only DX Shader Model (sm_*) and GLSL (glsl_*) targets are supported."),
                HE_KV(target, target));
            return -1;
        }

        int32_t index = request->addCodeGenTarget(codegenTarget);

        // TODO: Enable this when it gets more robust.
        //request->setTargetFlags(index, SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY);

        SlangProfileID profileId = globalSession->findProfile(target);
        if (profileId == SLANG_PROFILE_UNKNOWN)
        {
            HE_LOG_ERROR(he_shaderc,
                HE_MSG("Unknown compilation target. No target found with that name."),
                HE_KV(target, target));
            return -1;
        }
        request->setTargetProfile(index, profileId);
    }

    r = request->compile();
    if (SLANG_FAILED(r))
    {
        HE_LOG_ERROR(he_shaderc,
            HE_MSG("Failed to compile shader."),
            HE_KV(result, r));
        return -1;
    }

    slang::ShaderReflection* reflection = slang::ShaderReflection::get(request);
    const uint64_t entryCount = reflection->getEntryPointCount();

    String fileBaseName = GetBaseName(fileName);
    RemoveExtension(fileBaseName);

    String outPath = args.outDir;
    ConcatPath(outPath, fileBaseName);
    outPath += ".shaders.h";

    File f;
    f.Open(outPath.Data(), FileOpenMode::WriteTruncate);

    String constName;
    for (uint32_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        // A bit awkward of an interface, but if I don't add the entry point here we will fail to
        // get it inside the target loop. Seems like there is a disconnect between what the
        // reflection knows to exist in the file and what the request thinks is there.
        slang::EntryPointReflection* entry = reflection->getEntryPointByIndex(entryIndex);
        request->addEntryPoint(0, entry->getName(), entry->getStage());

        for (uint32_t targetIndex = 0; targetIndex < args.targets.Size(); ++targetIndex)
        {
            Slang::ComPtr<slang::IBlob> entryBlob;
            r = request->getEntryPointCodeBlob(entryIndex, targetIndex, entryBlob.writeRef());
            if (!entryBlob || SLANG_FAILED(r))
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Failed to get entry point code blob from compiled shader."),
                    HE_KV(entry_index, entryIndex),
                    HE_KV(entry_name, entry->getName()),
                    HE_KV(entry_stage, entry->getStage()),
                    HE_KV(target_index, targetIndex),
                    HE_KV(result, r));
                return -1;
            }

            constName = "c_";
            constName += fileBaseName;
            for (char& c : constName)
            {
                if (c == '-' || c == '.')
                    c = '_';
            }

            switch (entry->getStage())
            {
                case SLANG_STAGE_VERTEX: constName += "_vs"; break;
                case SLANG_STAGE_COMPUTE: constName += "_cs"; break;
                case SLANG_STAGE_PIXEL: constName += "_ps"; break;
                case SLANG_STAGE_HULL: constName += "_hs"; break;
                case SLANG_STAGE_DOMAIN: constName += "_ds"; break;
                case SLANG_STAGE_GEOMETRY: constName += "_gs"; break;
                case SLANG_STAGE_MESH: constName += "_ms"; break;
                default:
                    HE_LOG_ERROR(he_shaderc,
                        HE_MSG("Encountered unknown shader stage."),
                        HE_KV(entry_index, entryIndex),
                        HE_KV(entry_name, entry->getName()),
                        HE_KV(entry_stage, entry->getStage()),
                        HE_KV(target_index, targetIndex),
                        HE_KV(result, r));
                    return -1;
            }

            const char* target = args.targets[targetIndex];
            if (String::Find(target, "sm_") == target)
            {
                constName += "_dxbc";
            }
            else if (String::Find(target, "glsl_") == target)
            {
                constName += "_spv";
            }
            else
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Encountered unknown shader target."),
                    HE_KV(entry_index, entryIndex),
                    HE_KV(entry_name, entry->getName()),
                    HE_KV(entry_stage, entry->getStage()),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(result, r));
                return -1;
            }

            const uint8_t* data = static_cast<const uint8_t*>(entryBlob->getBufferPointer());
            WriteFileData(f, constName, data, entryBlob->getBufferSize(), false);
        }
    }

    f.Close();

    return 0;
}

void WriteFileData(he::File& file, he::StringView name, const uint8_t* data, size_t size, bool asText)
{
    constexpr size_t HexBytesPerLine = 16;
    constexpr size_t HexByteStrLen = HE_LENGTH_OF("0x00, ") - 1;
    constexpr size_t HexLineStrLen = HexBytesPerLine * HexByteStrLen;
    constexpr size_t MemoryBufferSize = (1024 * 8);

    he::String buf;
    buf.Reserve(MemoryBufferSize + 256); // little extra to prevent regrowth if a line goes over a bit

    const auto flush = [&]() -> bool
    {
        he::Result r = file.Write(buf.Data(), buf.Size());
        if (!HE_VERIFY(r, HE_KV(result, r)))
            return false;
        buf.Clear();
        return true;
    };

    if (asText)
        FormatTo(buf, "static const char {}[{}] =\n{{\n", name, size);
    else
        FormatTo(buf, "static const unsigned char {}[{}] =\n{{\n", name, size);

    if (data != nullptr)
    {
        char asciiBuf[HexBytesPerLine + 1];

        while (size > HexBytesPerLine)
        {
            for (size_t i = 0; i < HexBytesPerLine; ++i)
                asciiBuf[i] = he::IsPrint(data[i]) && data[i] != '\\' ? data[i] : '.';
            asciiBuf[HexBytesPerLine] = '\0';

            he::FormatTo(buf, "    {0:#04x}, // {1}\n", he::FmtJoin(data, data + HexBytesPerLine, ", "), asciiBuf);

            if (buf.Size() >= MemoryBufferSize)
            {
                if (!flush())
                    return;
            }

            data += HexBytesPerLine;
            size -= HexBytesPerLine;
        }

        if (size > 0)
        {
            for (size_t i = 0; i < size; ++i)
                asciiBuf[i] = he::IsPrint(data[i]) && data[i] != '\\' ? data[i] : '.';
            asciiBuf[size] = '\0';

            const uint32_t lenBefore = buf.Size();
            he::FormatTo(buf, "    {:#04x},", he::FmtJoin(data, data + size, ", "));

            const uint32_t lineLength = buf.Size() - lenBefore;
            if (lineLength < HexLineStrLen)
                buf.Resize(buf.Size() + (HexLineStrLen - lineLength) + 3, ' ');

            he::FormatTo(buf, " // {}\n", asciiBuf);
        }
    }

    buf.Append("};\n");
    flush();
}

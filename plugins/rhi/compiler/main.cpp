// Copyright Chad Engler

#include "he/core/allocator.h"
#include "he/core/args.h"
#include "he/core/ascii.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/fmt.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/string_view.h"
#include "he/core/vector.h"

#include "slang.h"
#include "slang-com-ptr.h"

#include <iostream>

using namespace slang;

struct AppArgs
{
    bool help{ false };
    SlangOptimizationLevelIntegral optLevel{ SLANG_OPTIMIZATION_LEVEL_DEFAULT };
    const char* outDir{ nullptr };
    he::Vector<const char*> defines{};
    he::Vector<const char*> includeDirs{};
    he::Vector<const char*> targets{};
};

void WriteFileData(he::File& file, he::StringView name, const uint8_t* data, size_t size, bool asText);

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    AddLogSink(ConsoleSink);

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

    if (!result || args.help || result.values.Size() != 1 || args.optLevel < SLANG_OPTIMIZATION_LEVEL_NONE || args.optLevel > SLANG_OPTIMIZATION_LEVEL_MAXIMAL)
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

    const char* fileName = result.values[0];

    // Create the global session and compilation request
    SlangGlobalSessionDesc globalSessiondesc{};
    Slang::ComPtr<IGlobalSession> globalSession;
    SlangResult r = createGlobalSession(&globalSessiondesc, globalSession.writeRef());
    if (SLANG_FAILED(r))
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("Failed to create global slang session."), HE_KV(file_name, fileName), HE_KV(result, r));
        return -1;
    }

    SessionDesc sessionDesc{};
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    for (const char* target : args.targets)
    {
        TargetDesc targetDesc{};

        if (StrFind(target, "sm_") == target)
        {
            targetDesc.format = SLANG_DXIL;
        }
        else if (StrFind(target, "glsl_") == target)
        {
            targetDesc.format = SLANG_SPIRV;
        }
        else
        {
            HE_LOG_ERROR(he_shaderc,
                HE_MSG("Unknown compilation target. Only DX Shader Model (sm_*) and GLSL (glsl_*) targets are supported."),
                HE_KV(file_name, fileName),
                HE_VAL(target));
            return -1;
        }

        SlangProfileID profileId = globalSession->findProfile(target);
        if (profileId == SLANG_PROFILE_UNKNOWN)
        {
            HE_LOG_ERROR(he_shaderc,
                HE_MSG("Unknown compilation target. No target found with that name."),
                HE_KV(file_name, fileName),
                HE_VAL(target));
            return -1;
        }
        targetDesc.profile = profileId;
    }

    Vector<const char*> searchPaths;

    const String fileDir = GetDirectory(fileName);
    searchPaths.PushBack(fileDir.Data());
    for (const char* includeDir : args.includeDirs)
    {
        searchPaths.PushBack(includeDir);
    }
    sessionDesc.searchPaths = searchPaths.Data();
    sessionDesc.searchPathCount = searchPaths.Size();

    Vector<String> macroNameStorage;
    macroNameStorage.Reserve(args.defines.Size());
    Vector<PreprocessorMacroDesc> macroDescs;
    macroDescs.Reserve(args.defines.Size());
    for (const char* d : args.defines)
    {
        const char* valueStart = StrFind(d, '=');
        const uint32_t nameLen = valueStart ? static_cast<uint32_t>(valueStart - d) : StrLen(d);

        String& name = macroNameStorage.EmplaceBack();
        name.Assign(d, nameLen);

        PreprocessorMacroDesc& macroDesc = macroDescs.EmplaceBack();
        macroDesc.name = name.Data();
        macroDesc.value = valueStart ? valueStart + 1 : "1";
    }

    CompilerOptionEntry compilerOptions[] =
    {
        { CompilerOptionName::Language, { .intValue0 = SLANG_SOURCE_LANGUAGE_SLANG } },
        { CompilerOptionName::LanguageVersion, { .intValue0 = SLANG_LANGUAGE_VERSION_2026 } },
        { CompilerOptionName::Optimization, { .intValue0 = static_cast<int32_t>(args.optLevel) } },
        { CompilerOptionName::DebugInformation, {.intValue0 = SLANG_DEBUG_INFO_LEVEL_STANDARD } },
        { CompilerOptionName::DebugInformationFormat, { .intValue0 = SLANG_DEBUG_INFO_FORMAT_DEFAULT } },
    };
    sessionDesc.compilerOptionEntries = compilerOptions;
    sessionDesc.compilerOptionEntryCount = HE_LENGTH_OF(compilerOptions);

    Slang::ComPtr<ISession> session;
    r = globalSession->createSession(sessionDesc, session.writeRef());
    if (SLANG_FAILED(r))
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("Failed to create slang compile session."), HE_KV(file_name, fileName), HE_KV(result, r));
        return -1;
    }

    Slang::ComPtr<IBlob> diagnostics;
    Slang::ComPtr<IModule> module(session->loadModule(fileName, diagnostics.writeRef()));

    if (diagnostics)
    {
        const char* msg = static_cast<const char*>(diagnostics->getBufferPointer());
        HE_LOG_ERROR(he_schemac, HE_MSG(msg), HE_KV(file_name, fileName), HE_KV(slang_diag, true));
    }

    if (!module)
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("Failed to load shader module."), HE_KV(file_name, fileName), HE_KV(result, r));
        return -1;
    }

    SlangInt32 entryPointCount = module->getDefinedEntryPointCount();
    if (entryPointCount == 0)
    {
        HE_LOG_ERROR(he_shaderc, HE_MSG("No entry points found in shader module."), HE_KV(file_name, fileName));
        return -1;
    }

    String fileBaseName = GetBaseName(fileName);
    RemoveExtension(fileBaseName);

    String outPath = args.outDir;
    ConcatPath(outPath, fileBaseName);
    outPath += ".shaders.h";

    File outputFile;
    outputFile.Open(outPath.Data(), FileAccessMode::Write, FileCreateMode::CreateAlways);

    String constName;
    for (uint32_t targetIndex = 0; targetIndex < args.targets.Size(); ++targetIndex)
    {
        const char* target = args.targets[targetIndex];

        for (SlangInt32 entryPointIndex = 0; entryPointIndex < entryPointCount; ++entryPointIndex)
        {
            Slang::ComPtr<IEntryPoint> entryPoint;
            r = module->getDefinedEntryPoint(entryPointIndex, entryPoint.writeRef());
            if (SLANG_FAILED(r) || !entryPoint)
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Failed to retrieve entry point from shader module."),
                    HE_KV(file_name, fileName),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(result, r));
                return -1;
            }

            IComponentType* components[] = { module, entryPoint };
            Slang::ComPtr<IComponentType> program;
            r = session->createCompositeComponentType(components, HE_LENGTH_OF(components), program.writeRef());
            if (SLANG_FAILED(r) || !program)
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Failed to create shader program."),
                    HE_KV(file_name, fileName),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(result, r));
                return -1;
            }

            slang::ProgramLayout* layout = program->getLayout();
            slang::EntryPointLayout* entryLayout = layout->getEntryPointByIndex(0);

            // TODO: Emit shader parameter metadata so we can do more automatic binding at runtime.
            // https://shader-slang.org/slang/user-guide/reflection

            Slang::ComPtr<IComponentType> linkedProgram;
            r = program->link(linkedProgram.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                const char* msg = static_cast<const char*>(diagnostics->getBufferPointer());
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG(msg),
                    HE_KV(file_name, fileName),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(entry_name, entryLayout->getName()),
                    HE_KV(entry_stage, entryLayout->getStage()),
                    HE_KV(slang_diag, true));
            }

            if (SLANG_FAILED(r) || !linkedProgram)
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Failed to link program for shader."),
                    HE_KV(file_name, fileName),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(entry_name, entryPoint->getFunctionReflection()->getName()),
                    HE_KV(entry_stage, entryLayout->getStage()),
                    HE_KV(result, r));
                return -1;
            }

            Slang::ComPtr<IBlob> kernelBlob;
            r = program->getEntryPointCode(entryPointIndex, targetIndex, kernelBlob.writeRef(), diagnostics.writeRef());

            if (diagnostics)
            {
                const char* msg = static_cast<const char*>(diagnostics->getBufferPointer());
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG(msg),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(file_name, fileName),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(entry_name, entryPoint->getFunctionReflection()->getName()),
                    HE_KV(entry_stage, entryLayout->getStage()),
                    HE_KV(slang_diag, true));
            }

            if (SLANG_FAILED(r) || !kernelBlob)
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Failed to get entry point code blob from compiled shader."),
                    HE_KV(file_name, fileName),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(entry_name, entryPoint->getFunctionReflection()->getName()),
                    HE_KV(entry_stage, entryLayout->getStage()),
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

            switch (entryLayout->getStage())
            {
                case SLANG_STAGE_VERTEX: constName += "_vs"; break;
                case SLANG_STAGE_HULL: constName += "_hs"; break;
                case SLANG_STAGE_DOMAIN: constName += "_ds"; break;
                case SLANG_STAGE_GEOMETRY: constName += "_gs"; break;
                case SLANG_STAGE_FRAGMENT: constName += "_ps"; break;
                case SLANG_STAGE_COMPUTE: constName += "_cs"; break;
                case SLANG_STAGE_RAY_GENERATION: constName += "_rt_gen"; break;
                case SLANG_STAGE_INTERSECTION: constName += "_rt_int"; break;
                case SLANG_STAGE_ANY_HIT: constName += "_rt_hit"; break;
                case SLANG_STAGE_CLOSEST_HIT: constName += "_rt_cl_hit"; break;
                case SLANG_STAGE_MISS: constName += "_rt_miss"; break;
                case SLANG_STAGE_CALLABLE: constName += "_rt_call"; break;
                case SLANG_STAGE_MESH: constName += "_ms"; break;
                case SLANG_STAGE_AMPLIFICATION: constName += "_as"; break;
                case SLANG_STAGE_DISPATCH: constName += "_ds"; break;
                default:
                    HE_LOG_ERROR(he_shaderc,
                        HE_MSG("Encountered unknown shader stage."),
                        HE_KV(file_name, fileName),
                        HE_KV(target_index, targetIndex),
                        HE_KV(target_name, target),
                        HE_KV(entry_index, entryPointIndex),
                        HE_KV(entry_name, entryLayout->getName()),
                        HE_KV(entry_stage, entryLayout->getStage()));
                    return -1;
            }

            if (StrFind(target, "sm_") == target)
            {
                constName += "_dxil";
            }
            else if (StrFind(target, "glsl_") == target)
            {
                constName += "_spv";
            }
            else
            {
                HE_LOG_ERROR(he_shaderc,
                    HE_MSG("Encountered unknown shader target."),
                    HE_KV(file_name, fileName),
                    HE_KV(target_index, targetIndex),
                    HE_KV(target_name, target),
                    HE_KV(entry_index, entryPointIndex),
                    HE_KV(entry_name, entryLayout->getName()),
                    HE_KV(entry_stage, entryLayout->getStage()));
                return -1;
            }

            const uint8_t* data = static_cast<const uint8_t*>(kernelBlob->getBufferPointer());
            WriteFileData(outputFile, constName, data, kernelBlob->getBufferSize(), false);
        }
    }

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

namespace he
{
    template <>
    const char* EnumTraits<SlangStage>::ToString(SlangStage x) noexcept
    {
        switch (x)
        {
            case SLANG_STAGE_NONE: return "SLANG_STAGE_NONE";
            case SLANG_STAGE_VERTEX: return "SLANG_STAGE_VERTEX";
            case SLANG_STAGE_HULL: return "SLANG_STAGE_HULL";
            case SLANG_STAGE_DOMAIN: return "SLANG_STAGE_DOMAIN";
            case SLANG_STAGE_GEOMETRY: return "SLANG_STAGE_GEOMETRY";
            case SLANG_STAGE_FRAGMENT: return "SLANG_STAGE_FRAGMENT";
            case SLANG_STAGE_COMPUTE: return "SLANG_STAGE_COMPUTE";
            case SLANG_STAGE_RAY_GENERATION: return "SLANG_STAGE_RAY_GENERATION";
            case SLANG_STAGE_INTERSECTION: return "SLANG_STAGE_INTERSECTION";
            case SLANG_STAGE_ANY_HIT: return "SLANG_STAGE_ANY_HIT";
            case SLANG_STAGE_CLOSEST_HIT: return "SLANG_STAGE_CLOSEST_HIT";
            case SLANG_STAGE_MISS: return "SLANG_STAGE_MISS";
            case SLANG_STAGE_CALLABLE: return "SLANG_STAGE_CALLABLE";
            case SLANG_STAGE_MESH: return "SLANG_STAGE_MESH";
            case SLANG_STAGE_AMPLIFICATION: return "SLANG_STAGE_AMPLIFICATION";
            case SLANG_STAGE_DISPATCH: return "SLANG_STAGE_DISPATCH";
            case SLANG_STAGE_COUNT: return "SLANG_STAGE_COUNT";
        }

        return "<unknown>";
    }
}

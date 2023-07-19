// Copyright Chad Engler

@0xf1b2ebc416d18f1e;

import "he/schema/schema.hsc";

namespace he.editor;

// A plugin file that can be opened by the editor
struct Plugin
{
    id @0 :String;
    version @1 :String;
    name @2 :String;
    description @3 :String;
    license @4 :String;
    authors @5 :String[];
    tags @6 :String[];
    modules @7 :Module[];
    install @8 :Install;
    import_plugins @9 :String[];
    extend @10 :Extension;

    enum ModuleType
    {
        ConsoleApp @0 $Toml.Name("console_app");
        Content @1 $Toml.Name("content");
        Custom @2 $Toml.Name("custom");
        Header @3 $Toml.Name("header");
        Static @4 $Toml.Name("static");
        Shared @5 $Toml.Name("shared");
        WindowedApp @6 $Toml.Name("windowed_app");
    }

    struct Module
    {
        name @0 :String;
        type @1 :ModuleType;
        group @2 :String;
        language @3 :String;
        files @4 :String[];
        postBuildCommands @5 :String[] $Toml.Name("post_build_commands");
        preBuildCommands @6 :String[] $Toml.Name("pre_build_commands");
        dependsonRuntime @7 :String[] $Toml.Name("dependson_runtime");
        variants @8 :Variant[];
        warnings @9 :String;
        exec @10 :String;
        copyFiles @11 :String[] $Toml.Name("copy_files");

        publicDefines @12 :String[] $Toml.Name("public_defines");
        publicDependson @13 :String[] $Toml.Name("public_dependson");
        publicDependson_include @14 :String[] $Toml.Name("public_dependson_include");
        publicIncludedirs @15 :String[] $Toml.Name("public_includedirs");

        privateDefines @16 :String[] $Toml.Name("private_defines");
        privateDependson @17 :String[] $Toml.Name("private_dependson");
        privateDependson_include @18 :String[] $Toml.Name("private_dependson_include");
        privateIncludedirs @19 :String[] $Toml.Name("private_includedirs");

        struct Variant
        {
            conditions @0 :String[];

            group @1 :String;
            language @2 :String;
            files @3 :String[];
            postBuildCommands @4 :String[] $Toml.Name("post_build_commands");
            preBuildCommands @5 :String[] $Toml.Name("pre_build_commands");
            dependsonRuntime @6 :String[] $Toml.Name("dependson_runtime");
            warnings @7 :String;
            exec @8 :String;
            copyFiles @9 :String[] $Toml.Name("copy_files");

            publicDefines @10 :String[] $Toml.Name("public_defines");
            publicDependson @11 :String[] $Toml.Name("public_dependson");
            publicDependson_include @12 :String[] $Toml.Name("public_dependson_include");
            publicIncludedirs @13 :String[] $Toml.Name("public_includedirs");

            privateDefines @14 :String[] $Toml.Name("private_defines");
            privateDependson @15 :String[] $Toml.Name("private_dependson");
            privateDependson_include @16 :String[] $Toml.Name("private_dependson_include");
            privateIncludedirs @17 :String[] $Toml.Name("private_includedirs");
        }
    }

    struct Install
    {
        exec @0 :String;
        validTargets @1 :String[] $Toml.Name("valid_targets");
        validHosts @2 :String[] $Toml.Name("valid_hosts");
        archive @3 :String;
        indexArchiveByHost @4 :bool $Toml.Name("index_archive_by_host");
        source @5 :String;
        github @6 :String;
        bitbucket @7 :String;
        nuget @8 :String;
        basepath @9 :String;
    }

    struct Extension
    {
        modules @0 :Module[];
    }
}

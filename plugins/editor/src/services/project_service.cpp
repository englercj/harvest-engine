// Copyright Chad Engler

#include "he/editor/services/project_service.h"

#include "he/core/fmt.h"
#include "he/core/assert.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/span.h"
#include "he/core/span_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_ops.h"
#include "he/core/uuid.h"
#include "he/core/vector.h"
#include "he/schema/toml.h"

namespace he::editor
{
    static void AddPluginToLoad(StringView plugin, StringView cwd, Vector<String>& pluginsToLoad)
    {
        String& pluginPath = pluginsToLoad.EmplaceBack();
        pluginPath = cwd;
        ConcatPath(pluginPath, plugin);

        if (GetExtension(plugin) == ".toml")
        {
            ConcatPath(pluginPath, "he_plugin.toml");
        }
    }

    ProjectService::ProjectService(DirectoryService& directoryService) noexcept
        : m_directoryService(directoryService)
    {}

    bool ProjectService::CreateAndOpen(StringView projectName, StringView projectDir, StringView enginePath)
    {
        if (!Close())
            return false;

        m_projectPath = projectDir;

        // Ensure the project directory exists.
        Result rc = Directory::Create(m_projectPath.Data(), true);
        if (!rc)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create project directory."),
                HE_KV(path, m_projectPath),
                HE_KV(result, rc));
            return false;
        }

        ConcatPath(m_projectPath, "he_project.toml");

        const String slugName = projectName; // TODO: slugify
        const String moduleName = slugName + "_content";

        String contentDir = projectDir;
        ConcatPath(contentDir, moduleName);

        // Ensure the content directory exists
        rc = Directory::Create(contentDir.Data());
        if (!rc)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create content directory in project directory."),
                HE_KV(path, contentDir),
                HE_KV(result, rc));
            return false;
        }

        // Build plugin file and save it out
        {
            schema::TypedBuilder<Plugin> plugin;
            Plugin::Builder pluginBuilder = plugin.GetOrAddRoot();
            schema::String::Builder nameStr = plugin.builder.AddString(projectName);
            schema::String::Builder slugNameStr = plugin.builder.AddString(slugName);
            schema::String::Builder moduleNameStr = plugin.builder.AddString(moduleName);

            pluginBuilder.SetId(slugNameStr);
            pluginBuilder.SetName(nameStr);
            pluginBuilder.SetDescription(nameStr);
            pluginBuilder.SetVersion(plugin.builder.AddString("1.0.0"));
            pluginBuilder.SetLicense(plugin.builder.AddString("UNLICENSED"));

            schema::List<Plugin::Module>::Builder moduleList = plugin.builder.AddList<Plugin::Module>(1);
            Plugin::Module::Builder moduleBuilder = moduleList[0];
            moduleBuilder.SetName(moduleNameStr);
            moduleBuilder.SetType(Plugin::ModuleType::Content);
            moduleBuilder.SetGroup(plugin.builder.AddString("game"));

            schema::List<schema::String>::Builder contentDirList = plugin.builder.AddList<schema::String>(1);
            contentDirList.Set(0, moduleNameStr);
            moduleBuilder.SetContentDirs(contentDirList);

            String buf;
            schema::ToToml<editor::Project>(buf, Project());

            String pluginPath = projectDir;
            ConcatPath(pluginPath, "he_plugin.toml");
            rc = File::WriteAll(buf.Data(), buf.Size(), pluginPath.Data());
            if (!rc)
            {
                HE_LOG_ERROR(editor,
                    HE_MSG("Failed to write content plugin file."),
                    HE_KV(path, pluginPath),
                    HE_KV(result, rc));
                return false;
            }
        }

        // Build project file and save
        {
            Project::Builder builder = m_builder.GetOrAddRoot();

            GenerateProjectId();
            builder.SetName(m_builder.builder.AddString(projectName));

            String relEnginePath = enginePath;
            MakeRelative(relEnginePath, projectDir);
            schema::String::Builder enginePluginPath = m_builder.builder.AddString(relEnginePath);
            schema::String::Builder localPluginPath = m_builder.builder.AddString(".");
            schema::List<schema::String>::Builder pluginList = m_builder.builder.AddList<schema::String>(2);
            pluginList.Set(0, enginePluginPath);
            pluginList.Set(1, localPluginPath);
            builder.SetPlugins(pluginList);

            if (!Save())
                return false;
        }

        // Setup the project as loaded now that we've saved the files.
        return LoadProjectInternal();
    }

    bool ProjectService::Open(StringView path)
    {
        if (!HE_VERIFY(!IsOpen()))
            return false;

        m_projectPath = path;
        return Reload();
    }

    bool ProjectService::Close()
    {
        if (!IsOpen())
            return true;

        if (!Save())
            return false;

        m_builder.builder.Clear();
        m_projectPath.Clear();
        m_plugins.Clear();
        m_onUnloadSignal.Dispatch();
        return true;
    }

    bool ProjectService::Reload()
    {
        String buf;
        Result r = File::ReadAll(buf, m_projectPath.Data());
        if (!r)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to read project file."),
                HE_KV(path, m_projectPath),
                HE_KV(result, r));
            return false;
        }

        if (!Close())
            return false;

        if (!schema::FromToml(m_builder, buf))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to deserialize project file. Is it valid TOML?"),
                HE_KV(path, m_projectPath));
            return false;
        }

        // Generate the ID if missing
        if (!Project().HasId())
        {
            GenerateProjectId();
            Save(); // even if we fail, we still continue with the reload
        }

        LoadProjectInternal();
        return true;
    }

    bool ProjectService::Save() const
    {
        if (!HE_VERIFY(IsOpen()))
            return false;

        String buf;
        schema::ToToml<editor::Project>(buf, Project());

        Result r = File::WriteAll(buf.Data(), buf.Size(), m_projectPath.Data());
        if (!r)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to write project file."),
                HE_KV(path, m_projectPath),
                HE_KV(result, r));
            return false;
        }

        return true;
    }

    String ProjectService::DataDir() const
    {
        String appDir;

        if (HE_VERIFY(IsOpen()))
        {
            appDir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Projects);

            if (!appDir.IsEmpty() && appDir.Back() != '/' && appDir.Back() != '\\')
                appDir.PushBack('/');

            const Span<const uint8_t> projId = Project().GetId().GetValue();
            HE_ASSERT(projId.Size() == sizeof(Uuid));

            FormatTo(appDir, "{:02x}", FmtJoin(projId, ""));
        }

        return appDir;
    }

    bool ProjectService::LoadProjectInternal()
    {
        HE_ASSERT(IsOpen());

        // ensure the data directory exists
        const String dataDir = DataDir();
        const Result rc = Directory::Create(dataDir.Data(), true);
        if (!rc)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create resource directory for newly opened project."),
                HE_KV(project_id, Project().GetId().GetValue()),
                HE_KV(project_name, Project().GetName().AsView()),
                HE_KV(path, dataDir),
                HE_KV(result, rc));
            return false;
        }

        if (!ReadPluginFiles())
        {
            HE_LOG_WARN(editor,
                HE_MSG("Failed to load one or more plugin files. Project may be in an invalid state."),
                HE_KV(project_id, Project().GetId().GetValue()),
                HE_KV(project_name, Project().GetName().AsView()));
            // we continue with the load even if a plugin fails
        }

        m_onLoadSignal.Dispatch();
        return true;
    }

    void ProjectService::GenerateProjectId()
    {
        const Uuid projId = Uuid::CreateV4();
        Span<uint8_t> idData = Project().InitId().GetValue();
        HE_ASSERT(idData.Size() == sizeof(projId.m_bytes));
        MemCopy(idData.Data(), projId.m_bytes, sizeof(projId.m_bytes));
    }

    bool ProjectService::ReadPluginFiles()
    {
        // Initialize some buffers we use throughout the loop to reduce allocations
        Vector<String> pluginsToLoad;
        pluginsToLoad.Reserve(32);

        String fileData;
        fileData.Reserve(4096);

        // Add the plugins listed in the project file to the list
        {
            String cwd = GetDirectory(m_projectPath);
            for (auto&& plugin : Project().GetPlugins())
            {
                AddPluginToLoad(plugin, cwd, pluginsToLoad);
            }
        }

        // Loop through the list of plugins to load and parse each one
        while (!pluginsToLoad.IsEmpty())
        {
            // Get the full path to the plugin we want to load
            const String fullPath = Move(pluginsToLoad.Back());
            pluginsToLoad.PopBack();

            // Read the plugin file into memory
            Result r = File::ReadAll(fileData, fullPath.Data());
            if (!r)
            {
                HE_LOG_WARN(he_editor,
                    HE_MSG("Failed to load plugin file."),
                    HE_KV(path, fullPath),
                    HE_KV(result, r));
                return false;
            }

            // Parse the plugin file into a schema structure
            PluginEntry& entry = m_plugins.EmplaceBack();
            entry.filePath = fullPath;
            if (!schema::FromToml(entry.plugin, fileData))
            {
                HE_LOG_WARN(editor,
                    HE_MSG("Failed to deserialize plugin file. Is it valid TOML?"),
                    HE_KV(path, fullPath));
                return false;
            }

            // Add any additionally imported plugins to the list to be loaded
            String cwd = GetDirectory(fullPath);
            for (auto&& plugin : entry.plugin.Root().GetPlugins())
            {
                AddPluginToLoad(plugin, cwd, pluginsToLoad);
            }
        }

        return true;
    }
}

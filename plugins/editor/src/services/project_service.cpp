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
    ProjectService::ProjectService(DirectoryService& directoryService) noexcept
        : m_directoryService(directoryService)
    {}

    bool ProjectService::Open(const char* path)
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

        Close();

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
            const Uuid projId = Uuid::CreateV4();
            Span<uint8_t> idData = Project().InitId().GetValue();
            HE_ASSERT(idData.Size() == sizeof(projId.m_bytes));
            MemCopy(idData.Data(), projId.m_bytes, sizeof(projId.m_bytes));
            Save();
        }

        // ensure the data directory exists
        const String dataDir = DataDir();
        r = Directory::Create(dataDir.Data(), true);
        if (!r)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create resource directory for newly opened project."),
                HE_KV(project_id, Project().GetId().GetValue()),
                HE_KV(project_name, Project().GetName().AsView()),
                HE_KV(path, dataDir),
                HE_KV(result, r));
            return false;
        }

        // read plugins
        ReadPluginFiles();

        m_onLoadSignal.Dispatch();
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

    void ProjectService::ReadPluginFiles()
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
            String fullPath = Move(pluginsToLoad.Back());
            pluginsToLoad.PopBack();

            // Read the plugin file into memory
            Result r = File::ReadAll(fileData, fullPath.Data());
            if (!r)
            {
                HE_LOG_ERROR(he_editor,
                    HE_MSG("Failed to load plugin file."),
                    HE_KV(path, fullPath),
                    HE_KV(result, r));
                continue;
            }

            // Parse the plugin file into a schema structure
            PluginEntry& entry = m_plugins.EmplaceBack();
            entry.filePath = fullPath;
            if (!schema::FromToml(entry.plugin, fileData))
            {
                HE_LOG_ERROR(editor,
                    HE_MSG("Failed to deserialize plugin file. Is it valid TOML?"),
                    HE_KV(path, m_projectPath));
                continue;
            }

            // Add any additionally imported plugins to the list to be loaded
            String cwd = GetDirectory(fullPath);
            for (auto&& plugin : entry.plugin.Root().GetPlugins())
            {
                AddPluginToLoad(plugin, cwd, pluginsToLoad);
            }
        }
    }
}

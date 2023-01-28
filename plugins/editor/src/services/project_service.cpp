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
#include "he/core/string_fmt.h"
#include "he/core/uuid.h"
#include "he/core/vector.h"
#include "he/schema/toml.h"

namespace he::editor
{
    ProjectService::ProjectService(DirectoryService& directoryService) noexcept
        : m_directoryService(directoryService)
    {}

    bool ProjectService::Create(const char* name, const char* path, const char* assetRoot)
    {
        if (!HE_VERIFY(!IsOpen()))
            return false;

        // Create a new project structure and set it as the root
        m_builder.Clear();
        m_project = m_builder.AddStruct<Project>();
        m_builder.SetRoot(m_project);

        // Generate the ID
        const Uuid projId = Uuid::CreateV4();
        Span<uint8_t> idData = m_project.InitId().GetValue();
        HE_ASSERT(idData.Size() == sizeof(projId.m_bytes));
        MemCopy(idData.Data(), projId.m_bytes, sizeof(projId.m_bytes));

        // Assign the name
        m_project.InitName(name);

        // Assign the project root if it was explicitly set, otherwise we assume the
        // same directory as the project file itself.
        if (!String::IsEmpty(assetRoot))
            m_project.InitAssetRoot(assetRoot);

        // Store off the path to project file and save it out
        m_projectPath = path;

        if (!Save())
        {
            m_projectPath.Clear();
            return false;
        }

        return true;
    }

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

        m_projectPath.Clear();
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

        if (!schema::FromToml<Project>(m_builder, buf.Data()))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to deserialize project file. Is it valid TOML?"),
                HE_KV(path, m_projectPath));
            return false;
        }

        m_project = m_builder.Root().TryGetStruct<Project>();

        // ensure the directory exists
        const String dataDir = DataDir();
        r = Directory::Create(dataDir.Data(), true);
        if (!r)
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create resource directory for newly opened project."),
                HE_KV(project_id, m_project.GetId().GetValue()),
                HE_KV(project_name, m_project.GetName().AsView()),
                HE_KV(path, dataDir),
                HE_KV(result, r));
            return false;
        }

        m_onLoadSignal.Dispatch();
        return true;
    }

    bool ProjectService::Save()
    {
        if (!HE_VERIFY(IsOpen()))
            return false;

        StringBuilder buf;
        if (!schema::ToToml<Project>(buf, m_project))
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to serialize project file. This is likely an editor bug."));
            return false;
        }

        Result r = File::WriteAll(buf.Str().Data(), buf.Size(), m_projectPath.Data());
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
        if (!HE_VERIFY(IsOpen()))
            return "";

        String appDir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Projects);

        if (!appDir.IsEmpty() && appDir.Back() != '/' && appDir.Back() != '\\')
            appDir.PushBack('/');

        const Span<const uint8_t> projId = m_project.GetId().GetValue();
        HE_ASSERT(projId.Size() == sizeof(Uuid));

        FormatTo(appDir, "{:02x}", FmtJoin(projId, ""));
        return appDir;
    }
}

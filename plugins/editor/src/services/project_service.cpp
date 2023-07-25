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
        }

        // ensure the directory exists
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
        if (!HE_VERIFY(IsOpen()))
            return "";

        String appDir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Projects);

        if (!appDir.IsEmpty() && appDir.Back() != '/' && appDir.Back() != '\\')
            appDir.PushBack('/');

        const Span<const uint8_t> projId = Project().GetId().GetValue();
        HE_ASSERT(projId.Size() == sizeof(Uuid));

        FormatTo(appDir, "{:02x}", FmtJoin(projId, ""));
        return appDir;
    }
}

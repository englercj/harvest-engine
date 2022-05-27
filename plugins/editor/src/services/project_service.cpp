// Copyright Chad Engler

#include "project_service.h"

#include "he/core/appender.h"
#include "he/core/assert.h"
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
    ProjectService::ProjectService(DirectoryService& directoryService)
        : m_directoryService(directoryService)
    {}

    bool ProjectService::Create(const char* name, const char* path)
    {
        HE_ASSERT(!IsOpen());

        const Uuid projId = Uuid::CreateV4();

        m_project = m_builder.Root().TryGetStruct<Project>();
        HE_ASSERT(m_project.GetId().Size() == sizeof(projId.m_bytes));
        MemCopy(m_project.GetId().Data(), projId.m_bytes, sizeof(projId.m_bytes));
        m_project.InitName(name);

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
        HE_ASSERT(!IsOpen());

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
        return true;
    }

    bool ProjectService::Reload()
    {
        HE_ASSERT(IsOpen());

        String buf;
        Result r = File::ReadAll(buf, m_projectPath.Data());
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to read project file."),
                HE_KV(path, m_projectPath),
                HE_KV(result, r));
            return false;
        }

        m_project = {};
        m_builder.Clear();

        if (!he::schema::FromToml<Project>(m_builder, buf.Data()))
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to deserialize project file. Is it valid TOML?"),
                HE_KV(path, m_projectPath));
            return false;
        }

        m_project = m_builder.Root().TryGetStruct<Project>();
        return true;
    }

    bool ProjectService::Save()
    {
        HE_ASSERT(IsOpen());

        StringBuilder buf;
        if (!he::schema::ToToml<Project>(buf, m_project))
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to serialize project file. This is likely an editor bug."));
            return false;
        }

        Result r = File::WriteAll(buf.Str().Data(), buf.Size(), m_projectPath.Data());
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to write project file."),
                HE_KV(path, m_projectPath),
                HE_KV(result, r));
            return false;
        }

        return true;
    }

    String ProjectService::GetResourceDir() const
    {
        HE_ASSERT(IsOpen());

        String appDir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Resources);
        appDir += '/';

        const Span<const uint8_t> projId = m_project.AsReader().GetId();
        HE_ASSERT(projId.Size() == sizeof(Uuid));

        fmt::format_to(Appender(appDir), "{}", projId);
        return appDir;
    }
}

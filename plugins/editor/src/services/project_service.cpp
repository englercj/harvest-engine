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
#include "he/schema/json.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

namespace he::editor
{
    ProjectService::ProjectService(DirectoryService& directoryService)
        : m_directoryService(directoryService)
    {}

    bool ProjectService::Create(const char* name, const char* path)
    {
        HE_ASSERT(!IsOpen());

        Uuid id = Uuid::CreateV4();

        static_assert(sizeof(m_project.id) == sizeof(id.m_bytes));
        MemCopy(m_project.id, id.m_bytes, sizeof(id.m_bytes));
        m_project.name = name;
        m_project.assetRoot.Clear();

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

        File file;

        Result r = file.Open(m_projectPath.Data(), FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to open project file for reading."),
                HE_KV(path, m_projectPath),
                HE_KV(error, r));
            return false;
        }

        String fileBuf(Allocator::GetTemp());
        fileBuf.Resize(static_cast<uint32_t>(file.GetSize()), DefaultInit);

        uint32_t bytesRead = 0;
        r = file.Read(fileBuf.Data(), fileBuf.Size(), &bytesRead);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to read project file."),
                HE_KV(path, m_projectPath),
                HE_KV(error, r));
            return false;
        }

        if (bytesRead != fileBuf.Size())
        {
            HE_LOG_ERROR(editor, HE_MSG("Got a short read from project file."),
                HE_KV(path, m_projectPath),
                HE_KV(expected_bytes, fileBuf.Size()),
                HE_KV(actual_bytes, bytesRead));
            return false;
        }

        file.Close();

        rapidjson::Document doc;
        doc.ParseInsitu(fileBuf.Data());

        if (doc.HasParseError())
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to parse project file. The JSON document is invalid."),
                HE_KV(path, m_projectPath),
                HE_KV(error, rapidjson::GetParseError_En(doc.GetParseError())));
            return false;
        }

        if (!he::schema::FromJson(doc, m_project))
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to parse project file. The JSON doesn't match the Project schema."),
                HE_KV(path, m_projectPath));
            return false;
        }

        return true;
    }

    bool ProjectService::Save()
    {
        HE_ASSERT(IsOpen());

        rapidjson::Document doc = he::schema::ToJson(m_project);

        rapidjson::StringBuffer buf(0, 4096);
        rapidjson::PrettyWriter writer(buf);
        doc.Accept(writer);

        File file;

        Result r = file.Open(m_projectPath.Data(), FileOpenMode::WriteTruncate);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to open project file for writing."),
                HE_KV(path, m_projectPath),
                HE_KV(error, r));
            return false;
        }

        r = file.Write(buf.GetString(), static_cast<uint32_t>(buf.GetSize()));
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to write project file."),
                HE_KV(path, m_projectPath),
                HE_KV(error, r));
            return false;
        }

        return true;
    }

    String ProjectService::GetResourceDir() const
    {
        HE_ASSERT(IsOpen());

        String appDir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Resources);
        appDir += '/';

        Span<const uint8_t> id{ m_project.id };
        fmt::format_to(Appender(appDir), "{}", id);

        return appDir;
    }
}

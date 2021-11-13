// Copyright Chad Engler

#include "project_service.h"

#include "schema/kj_file_stream.h"

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

#include "capnp/serialize.h"
#include "capnp/compat/json.h"

namespace he::editor
{
    ProjectService::ProjectService(DirectoryService& directoryService)
        : m_directoryService(directoryService)
    {}

    bool ProjectService::Create(const char* name, const char* path)
    {
        HE_ASSERT(!IsOpen());

        m_project = m_builder.getRoot<schema::Project>();
        m_project.setName(name);
        m_project.disownAssetRoot();

        // Set ID
        const Uuid projId = Uuid::CreateV4();
        schema::ProjectId::Builder idBuilder = m_project.getId();
        idBuilder.setX0(projId.GetLow());
        idBuilder.setX1(projId.GetHigh());

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

        m_project = m_builder.getRoot<Project>();
        capnp::JsonCodec json;
        json.decode({ fileBuf.Data(), fileBuf.Size() }, m_project);

        return true;
    }

    bool ProjectService::Save()
    {
        HE_ASSERT(IsOpen());

        File file;

        Result r = file.Open(m_projectPath.Data(), FileOpenMode::WriteTruncate);
        if (!r)
        {
            HE_LOG_ERROR(editor, HE_MSG("Failed to open project file for writing."),
                HE_KV(path, m_projectPath),
                HE_KV(error, r));
            return false;
        }

        capnp::JsonCodec json;
        json.setHasMode(capnp::HasMode::NON_DEFAULT);
        kj::String data = json.encode(m_project);

        r = file.Write(data.cStr(), static_cast<uint32_t>(data.size()));
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

        const schema::ProjectId::Reader projId = m_project.asReader().getId();
        const kj::ArrayPtr<const kj::byte> bytes = capnp::AnyStruct::Reader(projId).getDataSection();
        const Span<const uint8_t> span(bytes.begin(), bytes.end());

        HE_ASSERT(span.Size() == sizeof(Uuid));

        fmt::format_to(Appender(appDir), "{}", span);

        return appDir;
    }
}

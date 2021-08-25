// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/clock_fmt.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/string_view.h"
#include "he/core/test.h"

using namespace he;

#if defined(HE_PLATFORM_API_WIN32)
    #include "he/core/win32_min.h"
#elif defined(HE_PLATFORM_API_POSIX)
    #include <errno.h>
#endif

// ------------------------------------------------------------------------------------------------
static void TouchTestFile(const char* path, const void* data = nullptr, uint32_t len = 0)
{
    File f;
    Result r = f.Open(path, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);

    if (data && len > 0)
    {
        r = f.Write(data, len);
        HE_EXPECT(r, r);
    }

    f.Close();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, GetFileResult)
{
    HE_EXPECT_EQ(GetFileResult(Result{ 0 }), FileResult::Success);

#if defined(HE_PLATFORM_API_WIN32)
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_SUCCESS }), FileResult::Success);
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_DISK_FULL }), FileResult::DiskFull);
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_ACCESS_DENIED }), FileResult::AccessDenied);
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_ALREADY_EXISTS }), FileResult::AlreadyExists);
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_FILE_NOT_FOUND }), FileResult::NotFound);
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_PATH_NOT_FOUND }), FileResult::NotFound);
    HE_EXPECT_EQ(GetFileResult(Result{ ERROR_NO_DATA }), FileResult::NoData);
#elif defined(HE_PLATFORM_API_POSIX)
    HE_EXPECT_EQ(GetFileResult(Result{ ENOSPC }), FileResult::DiskFull);
    HE_EXPECT_EQ(GetFileResult(Result{ EACCES }), FileResult::AccessDenied);
    HE_EXPECT_EQ(GetFileResult(Result{ EEXIST }), FileResult::AlreadyExists);
    HE_EXPECT_EQ(GetFileResult(Result{ ENOENT }), FileResult::NotFound);
    HE_EXPECT_EQ(GetFileResult(Result{ EAGAIN }), FileResult::NoData);
#endif

    constexpr const char TestPath[] = "a0688026-be17-4a99-b4a3-651307bba9ec";
    File::Remove(TestPath);

    File f;
    Result r = f.Open(TestPath, FileOpenMode::ReadExisting);
    HE_EXPECT(!r);
    HE_EXPECT_EQ(GetFileResult(r), FileResult::NotFound);

    r = File::Remove(TestPath);
    HE_EXPECT(!r);
    HE_EXPECT_EQ(GetFileResult(r), FileResult::NotFound);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Static_Exists)
{
    constexpr const char TestPath[] = "b36cad87-1e72-4dbc-942c-d85aa54774b6";
    File::Remove(TestPath);

    HE_EXPECT(!File::Exists(TestPath));
    TouchTestFile(TestPath);

    HE_EXPECT(File::Exists(TestPath));
    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Static_Remove)
{
    constexpr const char TestPath[] = "b04a864e-1008-4c4d-8f4e-22df1fb148d6";

    TouchTestFile(TestPath);
    HE_EXPECT(File::Exists(TestPath));

    HE_EXPECT(File::Remove(TestPath));
    HE_EXPECT(!File::Exists(TestPath));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Static_Rename)
{
    constexpr const char TestPath[] = "b9f8eefe-d292-4b0a-a494-8f9863c7d4a8";
    constexpr const char TestPath2[] = "1179e46e-526b-493d-a373-041bf340e0a3";
    File::Remove(TestPath2);

    TouchTestFile(TestPath);
    HE_EXPECT(File::Exists(TestPath));
    HE_EXPECT(!File::Exists(TestPath2));

    HE_EXPECT(File::Rename(TestPath, TestPath2));
    HE_EXPECT(!File::Exists(TestPath));
    HE_EXPECT(File::Exists(TestPath2));

    File::Remove(TestPath2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Static_Copy)
{
    constexpr const char TestPath[] = "138ba0d8-7172-4b28-b12d-5a1cd1519796";
    constexpr const char TestPath2[] = "61965c6d-d19f-4bdd-9ec6-4e8f8cfd4baf";
    File::Remove(TestPath);
    File::Remove(TestPath2);

    TouchTestFile(TestPath, TestPath, HE_LENGTH_OF(TestPath));
    HE_EXPECT(File::Exists(TestPath));
    HE_EXPECT(!File::Exists(TestPath2));

    HE_EXPECT(File::Rename(TestPath, TestPath2));
    HE_EXPECT(!File::Exists(TestPath));
    HE_EXPECT(File::Exists(TestPath2));

    File::Remove(TestPath2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Static_GetAttributes)
{
    constexpr const char TestPath[] = "632648bf-6426-4e97-97d0-68a919f1b12d";
    File::Remove(TestPath);

    FileAttributes attributes;
    Result r = File::GetAttributes(TestPath, attributes);
    HE_EXPECT(!r);
    HE_EXPECT_EQ(GetFileResult(r), FileResult::NotFound);

    TouchTestFile(TestPath, TestPath, HE_LENGTH_OF(TestPath));
    r = File::GetAttributes(TestPath, attributes);
    HE_EXPECT(r, r);
    HE_EXPECT_EQ(attributes.flags, FileAttributeFlag::None);
    HE_EXPECT_EQ(attributes.size, HE_LENGTH_OF(TestPath));
    HE_EXPECT_LE(attributes.createTime, SystemClock::Now());
    HE_EXPECT_LE(attributes.accessTime, SystemClock::Now());
    HE_EXPECT_LE(attributes.writeTime, SystemClock::Now());

    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Move)
{
    constexpr const char TestPath[] = "ba50d6b9-46b4-4756-9a1e-f78a5e0cae2e";

    File f0;
    Result r = f0.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);
    HE_EXPECT(f0.IsOpen());

    File f1(Move(f0));
    HE_EXPECT(!f0.IsOpen());
    HE_EXPECT(f1.IsOpen());

    File f2;
    f2 = Move(f1);
    HE_EXPECT(!f0.IsOpen());
    HE_EXPECT(!f1.IsOpen());
    HE_EXPECT(f2.IsOpen());

    f2.Close();

    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Open_Close)
{
    constexpr const char TestPath[] = "a3ac923d-6eb3-497f-ad88-772e9b4b9bdb";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);
    HE_EXPECT(f.IsOpen());

    f.Close();
    HE_EXPECT(!f.IsOpen());

    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, GetSize)
{
    constexpr const char TestPath[] = "126fff87-4211-4a2c-9237-3ad09d02b167";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);

    uint64_t fsize = f.GetSize();
    HE_EXPECT_EQ(fsize, 0);

    f.Write(TestPath, HE_LENGTH_OF(TestPath));
    fsize = f.GetSize();
    HE_EXPECT_EQ(fsize, HE_LENGTH_OF(TestPath));

    f.Close();
    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, SetSize)
{
    constexpr const char TestPath[] = "a2741904-c0a5-4b88-a038-db63fc24ebab";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::ReadWriteTruncate);
    HE_EXPECT(r, r);

    uint64_t fsize = f.GetSize();
    HE_EXPECT_EQ(fsize, 0);

    r = f.SetSize(100);
    HE_EXPECT(r, r);

    fsize = f.GetSize();
    HE_EXPECT_EQ(fsize, 100);

    uint32_t buf = 1;
    r = f.Read(&buf, sizeof(buf));
    HE_EXPECT(r, r);
    HE_EXPECT_EQ(buf, 0);

    f.Close();
    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, GetPos_SetPos)
{
    constexpr const char TestPath[] = "48159f47-7fd1-4f64-ba59-342f1f8a78f1";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);

    r = f.SetSize(100);
    HE_EXPECT(r, r);

    uint64_t pos = f.GetPos();
    HE_EXPECT_EQ(pos, 0);

    r = f.SetPos(25);
    HE_EXPECT(r, r);

    pos = f.GetPos();
    HE_EXPECT_EQ(pos, 25);

    r = f.SetPos(200);
    HE_EXPECT(r, r);

    pos = f.GetPos();
    HE_EXPECT_EQ(pos, 200);

    f.Close();
    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Read_Write)
{
    constexpr const char TestPath[] = "73a94832-ef1a-4e58-8293-70eec2215d52";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::ReadWriteTruncate);
    HE_EXPECT(r, r);

    r = f.SetSize(100);
    HE_EXPECT(r, r);

    uint32_t value0 = 250;

    r = f.Write(&value0, sizeof(value0));
    HE_EXPECT(r, r);

    uint64_t pos = f.GetPos();
    HE_EXPECT_EQ(pos, 4);

    r = f.SetPos(0);
    HE_EXPECT(r, r);

    uint32_t value1 = 0;
    r = f.Read(&value1, sizeof(value1));
    HE_EXPECT(r, r);
    HE_EXPECT_EQ(value1, value0);

    pos = f.GetPos();
    HE_EXPECT_EQ(pos, 4);

    f.Close();
    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, ReadAt_WriteAt)
{
    constexpr const char TestPath[] = "50f044c3-716e-4e0f-906b-b5eef0bb654f";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::ReadWriteTruncate);
    HE_EXPECT(r, r);

    r = f.SetSize(100);
    HE_EXPECT(r, r);

    uint32_t value0 = 250;
    r = f.WriteAt(&value0, sizeof(value0), 4);
    HE_EXPECT(r, r);

    uint32_t value1 = 0;
    r = f.ReadAt(&value1, sizeof(value1), 4);
    HE_EXPECT(r, r);
    HE_EXPECT_EQ(value1, value0);

    f.Close();
    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, Flush)
{
    constexpr const char TestPath[] = "2a3fe5ed-01e7-4467-9225-dbda8f9cc038";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);

    r = f.Flush();
    HE_EXPECT(r, r);

    uint32_t value0 = 250;
    r = f.WriteAt(&value0, sizeof(value0), 4);
    HE_EXPECT(r, r);

    r = f.Flush();
    HE_EXPECT(r, r);

    f.Close();
    File::Remove(TestPath);

}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, GetAttributes)
{
    constexpr const char TestPath[] = "75530d88-2a9d-4f8c-893b-68ed5a6ff16d";
    TouchTestFile(TestPath);

    FileAttributes attributes;
    File f;
    Result r = f.Open(TestPath, FileOpenMode::ReadWriteExisting);
    HE_EXPECT(r, r);

    r = f.Write(TestPath, HE_LENGTH_OF(TestPath));
    HE_EXPECT(r, r);

    r = f.GetAttributes(attributes);
    HE_EXPECT(r, r);
    HE_EXPECT_EQ(attributes.flags, FileAttributeFlag::None);
    HE_EXPECT_EQ(attributes.size, HE_LENGTH_OF(TestPath));
    HE_EXPECT_LE(attributes.createTime, SystemClock::Now());
    HE_EXPECT_LE(attributes.accessTime, SystemClock::Now());
    HE_EXPECT_LE(attributes.writeTime, SystemClock::Now());

    File::Remove(TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, GetPath)
{
    constexpr const char TestPath[] = "c8862020-d165-4a80-b5eb-333131fd02f3";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);

    String path(CrtAllocator::Get());
    r = f.GetPath(path);
    HE_EXPECT(r, r);

    StringView pathBase = GetBaseName(path.Data());
    HE_EXPECT_EQ_STR(pathBase.Data(), TestPath);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, file, SetTimes)
{
    constexpr const char TestPath[] = "a08d0e34-eff5-491d-8fa1-a71f760288de";

    File f;
    Result r = f.Open(TestPath, FileOpenMode::WriteTruncate);
    HE_EXPECT(r, r);

    SystemTime c{ 1234567890123400 };

    r = f.SetTimes(&c, &c);
    HE_EXPECT(r, r);

    FileAttributes attributes;
    r = f.GetAttributes(attributes);
    HE_EXPECT(r, r);

    HE_EXPECT_EQ(attributes.accessTime, c);
    HE_EXPECT_EQ(attributes.writeTime, c);
}

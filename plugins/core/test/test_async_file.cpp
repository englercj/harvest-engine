// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/async_file.h"

#include "he/core/directory.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
class AsyncFileFixture : public TestFixture
{
public:
    void Before() override
    {
        AsyncFileIOConfig config;
        StartupAsyncFileIO(config);
    }

    void After() override
    {
        ShutdownAsyncFileIO();
    }
};

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, async_file, Open_Close, AsyncFileFixture)
{
    constexpr const char TestPath[] = "0219db88-7616-48ed-b274-404cbeff336f";
    const String testPath = GetTempTestPath(TestPath);

    AsyncFile f;
    Result r = f.Open(testPath.Data(), FileAccessMode::Write, FileCreateMode::CreateAlways);
    HE_EXPECT(r, r);
    HE_EXPECT(f.IsOpen());

    f.Close();
    HE_EXPECT(!f.IsOpen());

    File::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, async_file, GetSize, AsyncFileFixture)
{
    constexpr const char TestPath[] = "b33d1c82-af5d-493d-876f-781d5d3ecadb";
    const String testPath = GetTempTestPath(TestPath);

    AsyncFile f;
    Result r = f.Open(testPath.Data(), FileAccessMode::Write, FileCreateMode::CreateAlways);
    HE_EXPECT(r, r);

    uint64_t fsize = f.GetSize();
    HE_EXPECT_EQ(fsize, 0);

    TouchTestFile(testPath.Data(), TestPath, HE_LENGTH_OF(TestPath));

    fsize = f.GetSize();
    HE_EXPECT_EQ(fsize, HE_LENGTH_OF(TestPath));

    f.Close();
    File::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, async_file, Read_Write, AsyncFileFixture)
{
    constexpr const char TestPath[] = "777738a7-1db4-4644-a04f-2f2667843dd2";
    const String testPath = GetTempTestPath(TestPath);

    AsyncFile f;
    Result openResult = f.Open(testPath.Data(), FileAccessMode::ReadWrite, FileCreateMode::CreateAlways);
    HE_EXPECT(openResult, openResult);

    {
        const uint32_t value = 250;
        AsyncFileOp op = f.WriteAsync(&value, 0, sizeof(value));

        uint32_t bytesTransferred = 0;
        Result r = AsyncFile::GetResult(op, &bytesTransferred);
        HE_EXPECT(r, r);
        HE_EXPECT_EQ(bytesTransferred, sizeof(value));
    }

    {
        uint32_t value = 0;
        AsyncFileOp op = f.ReadAsync(&value, 0, sizeof(value));

        uint32_t bytesTransferred = 0;
        Result r = AsyncFile::GetResult(op, &bytesTransferred);
        HE_EXPECT(r, r);
        HE_EXPECT_EQ(bytesTransferred, sizeof(value));
        HE_EXPECT_EQ(value, 250);
    }

    f.Close();
    File::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, async_file, GetAttributes, AsyncFileFixture)
{
    constexpr const char TestPath[] = "5a0c50d8-7918-47a3-9d00-24d955f48680";
    const String testPath = GetTempTestPath(TestPath);
    TouchTestFile(testPath.Data());

    AsyncFile f;
    Result openResult = f.Open(testPath.Data(), FileAccessMode::ReadWrite, FileCreateMode::OpenExisting);
    HE_EXPECT(openResult, openResult);

    {
        AsyncFileOp op = f.WriteAsync(TestPath, 0, HE_LENGTH_OF(TestPath));

        uint32_t bytesTransferred = 0;
        Result r = AsyncFile::GetResult(op, &bytesTransferred);
        HE_EXPECT(r, r);
        HE_EXPECT_EQ(bytesTransferred, HE_LENGTH_OF(TestPath));
    }

    {
        FileAttributes attributes;
        Result r = f.GetAttributes(attributes);
        HE_EXPECT(r, r);
        HE_EXPECT(attributes.flags == FileAttributeFlag::None);
        HE_EXPECT_EQ(attributes.size, HE_LENGTH_OF(TestPath));
        HE_EXPECT_LE(attributes.createTime.val, SystemClock::Now().val);
        HE_EXPECT_LE(attributes.accessTime.val, SystemClock::Now().val);
        HE_EXPECT_LE(attributes.writeTime.val, SystemClock::Now().val);
    }

    File::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, async_file, GetPath, AsyncFileFixture)
{
    constexpr const char TestPath[] = "8c6a85bc-7b50-41b2-bf0f-8b5b8081d143";
    const String testPath = GetTempTestPath(TestPath);

    AsyncFile f;
    Result r = f.Open(testPath.Data(), FileAccessMode::Write, FileCreateMode::CreateAlways);
    HE_EXPECT(r, r);

    String path;
    r = f.GetPath(path);
    HE_EXPECT(r, r);
    HE_EXPECT(IsAbsolutePath(path));
    NormalizePath(path);

    String expectedPath = testPath;
    NormalizePath(expectedPath);

    HE_EXPECT_EQ(path, expectedPath);

    f.Close();
    File::Remove(testPath.Data());
}

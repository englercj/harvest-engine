// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/bulk_file_loader.h"

#include "he/core/file.h"
#include "he/core/result_fmt.h"
#include "he/core/test.h"

using namespace he;

static BulkFileLoader* CreateLoader()
{
    BulkFileLoader::Config config;
    BulkFileLoader* loader;
    Result r = BulkFileLoader::Create(config, loader);
    HE_EXPECT(r, r);
    HE_EXPECT(loader != nullptr);

    return loader;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bulk_file_loader, Static_Create_Destroy)
{
    BulkFileLoader* loader = CreateLoader();
    BulkFileLoader::Destroy(loader);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bulk_file_loader, OpenFile_CloseFile)
{
    constexpr const char TestPath[] = "4bac7653-900f-4d4c-a4cf-695231fb1906";
    const String testPath = GetTempTestPath(TestPath);
    TouchTestFile(testPath.Data(), TestPath, HE_LENGTH_OF(TestPath));

    BulkFileLoader* loader = CreateLoader();

    BulkFileId fd;
    Result r = loader->OpenFile(testPath.Data(), fd);
    HE_EXPECT(r, r);
    HE_EXPECT_NE(fd.val, -1);

    loader->CloseFile(fd);

    BulkFileLoader::Destroy(loader);
    File::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bulk_file_loader, GetAttributes)
{
    BulkFileLoader* loader = CreateLoader();

    constexpr const char TestPath[] = "c8c37a8b-d037-495d-bf78-e4ca7b388710";
    const String testPath = GetTempTestPath(TestPath);
    TouchTestFile(testPath.Data(), TestPath, HE_LENGTH_OF(TestPath));

    BulkFileId fd;
    Result r = loader->OpenFile(testPath.Data(), fd);
    HE_EXPECT(r, r);
    HE_EXPECT_NE(fd.val, -1);

    {
        FileAttributes attributes;
        r = loader->GetAttributes(fd, attributes);
        HE_EXPECT(r, r);
        HE_EXPECT(attributes.flags == FileAttributeFlag::None);
        HE_EXPECT_EQ(attributes.size, HE_LENGTH_OF(TestPath));
        HE_EXPECT_LE(attributes.createTime.val, SystemClock::Now().val);
        HE_EXPECT_LE(attributes.accessTime.val, SystemClock::Now().val);
        HE_EXPECT_LE(attributes.writeTime.val, SystemClock::Now().val);
    }

    loader->CloseFile(fd);
    BulkFileLoader::Destroy(loader);

    File::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bulk_file_loader, CreateQueue_DestroyQueue)
{
    BulkFileLoader* loader = CreateLoader();
    BulkFileQueue* queue = nullptr;
    Result r = loader->CreateQueue({}, queue);
    HE_EXPECT(r, r);
    HE_EXPECT(queue);

    loader->DestroyQueue(queue);
    BulkFileLoader::Destroy(loader);
}


// ------------------------------------------------------------------------------------------------
HE_TEST(core, bulk_file_loader, Read)
{
    BulkFileLoader* loader = CreateLoader();

    constexpr const char TestPath[] = "912f0d29-db0c-4c05-bc1b-f8162b20f4e1";
    const String testPath = GetTempTestPath(TestPath);
    TouchTestFile(testPath.Data(), TestPath, HE_LENGTH_OF(TestPath));

    BulkFileId fd;
    Result r = loader->OpenFile(testPath.Data(), fd);
    HE_EXPECT(r, r);
    HE_EXPECT_NE(fd.val, -1);

    BulkFileQueue* queue = nullptr;
    r = loader->CreateQueue({}, queue);
    HE_EXPECT(r, r);
    HE_EXPECT(queue);

    {
        char buf[HE_LENGTH_OF(TestPath)]{};

        BulkReadRequest req{};
        req.file = fd;
        req.offset = 0;
        req.size = HE_LENGTH_OF(TestPath);
        req.dst = buf;
        req.dstSize = req.size;
        queue->Enqueue(req);

        BulkReadId op = queue->Submit();
        r = queue->GetResult(op);
        HE_EXPECT(r, r);
        HE_EXPECT_EQ_STR(buf, TestPath);
    }

    loader->DestroyQueue(queue);
    loader->CloseFile(fd);
    BulkFileLoader::Destroy(loader);
    File::Remove(testPath.Data());
}

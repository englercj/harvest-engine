// Copyright Chad Engler

#include "he/core/async_file_loader.h"

#include "he/core/file.h"
#include "he/core/result_fmt.h"
#include "he/core/test.h"

using namespace he;

//
// 95518081-bd66-41f2-a7f5-2e0bd6d824ab
// a49e51d0-ab44-4b80-97d2-132a32075738
// 5125a722-3369-4be4-953f-7a801c79045e
// 66af6626-3238-43e0-b90c-26df055dc194
// 3ca59ce3-344a-469a-b7e3-2d845984eeee
// 9830591e-f029-48fc-afad-b3b242203285

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

AsyncFileLoader* CreateLoader()
{
    AsyncFileLoader::Config config;
    AsyncFileLoader* loader;
    Result r = AsyncFileLoader::Create(config, loader);
    HE_EXPECT(r, r);
    HE_EXPECT(loader != nullptr);

    return loader;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, async_file_loader, Static_Create_Destroy)
{
    AsyncFileLoader* loader = CreateLoader();
    AsyncFileLoader::Destroy(loader);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, async_file_loader, OpenFile_CloseFile)
{
    constexpr const char TestPath[] = "4bac7653-900f-4d4c-a4cf-695231fb1906";
    TouchTestFile(TestPath, TestPath, HE_LENGTH_OF(TestPath));

    AsyncFileLoader* loader = CreateLoader();

    AsyncFileId fd;
    Result r = loader->OpenFile(TestPath, fd);
    HE_EXPECT(r, r);

    loader->CloseFile(fd);

    AsyncFileLoader::Destroy(loader);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, async_file_loader, GetAttributes)
{
    AsyncFileLoader* loader = CreateLoader();
    // TODO
    AsyncFileLoader::Destroy(loader);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, async_file_loader, DefaultQueue)
{
    AsyncFileLoader* loader = CreateLoader();
    AsyncFileQueue* queue = loader->DefaultQueue();
    // TODO
    HE_UNUSED(queue);
    AsyncFileLoader::Destroy(loader);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, async_file_loader, CreateQueue_DestroyQueue)
{
    AsyncFileLoader* loader = CreateLoader();
    // TODO
    AsyncFileLoader::Destroy(loader);
}

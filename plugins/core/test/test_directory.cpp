// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/directory.h"

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, DirectoryScanner)
{
    DirectoryScanner scanner;
    DirectoryScanner::Entry entry;

    HE_EXPECT(scanner.Open("."));
    while (scanner.NextEntry(entry))
    {
        HE_EXPECT_NE(entry.name, ".");
        HE_EXPECT_NE(entry.name, "..");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, GetSpecial)
{
    String dir;

    Result r = Directory::GetSpecial(dir, SpecialDirectory::Documents);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());

    r = Directory::GetSpecial(dir, SpecialDirectory::LocalAppData);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());

    r = Directory::GetSpecial(dir, SpecialDirectory::SharedAppData);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());

    r = Directory::GetSpecial(dir, SpecialDirectory::Temp);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, GetCurrent)
{
    String dir;

    Result r = Directory::GetCurrent(dir);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, SetCurrent)
{
    String dir;

    HE_EXPECT(Directory::GetCurrent(dir));
    HE_EXPECT(!dir.IsEmpty());
    HE_EXPECT(Directory::SetCurrent("/"));
    HE_EXPECT(Directory::SetCurrent(dir.Data()));

    String dir2;
    HE_EXPECT(Directory::GetCurrent(dir2));
    HE_EXPECT_EQ(dir, dir2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Rename)
{
    constexpr const char TestPath[] = "f643b654-aa01-4e7f-942a-4eb6091eacfc";
    constexpr const char TestPath2[] = "cbda45d4-340f-4d81-b543-fb72d3a452cd";
    const String testPath = GetTempTestPath(TestPath);
    const String testPath2 = GetTempTestPath(TestPath2);
    Directory::Remove(testPath.Data());
    Directory::Remove(testPath2.Data());

    HE_EXPECT(Directory::Create(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath.Data()));
    HE_EXPECT(Directory::Rename(testPath.Data(), testPath2.Data()));
    HE_EXPECT(!Directory::Exists(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath2.Data()));
    HE_EXPECT(Directory::Remove(testPath2.Data()));
    HE_EXPECT(!Directory::Exists(testPath2.Data()));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Exists)
{
    constexpr const char TestPath[] = "c04ffe30-cf44-4e13-98b7-a7e26be19129";
    const String testPath = GetTempTestPath(TestPath);
    File::Remove(testPath.Data());

    File f;
    HE_EXPECT(f.Open(testPath.Data(), FileAccessMode::Write, FileCreateMode::CreateAlways));
    HE_EXPECT(!Directory::Exists(testPath.Data()));
    f.Close();
    File::Remove(testPath.Data());

    HE_EXPECT(!Directory::Exists(testPath.Data()));
    HE_EXPECT(Directory::Create(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath.Data()));
    HE_EXPECT(Directory::Remove(testPath.Data()));
    HE_EXPECT(!Directory::Exists(testPath.Data()));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Create)
{
    constexpr const char TestPath[] = "8ef2f5c8-f1e1-4267-a6a5-5c431e497cd4";
    const String testPath = GetTempTestPath(TestPath);
    Directory::Remove(testPath.Data());

    HE_EXPECT(Directory::Create(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath.Data()));
    Directory::Remove(testPath.Data());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Remove)
{
    constexpr const char TestPath[] = "bd75ad45-f89b-47ed-bab3-b924606c8e59";
    const String testPath = GetTempTestPath(TestPath);
    Directory::Remove(testPath.Data());

    HE_EXPECT(Directory::Create(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath.Data()));
    HE_EXPECT(Directory::Remove(testPath.Data()));
    HE_EXPECT(!Directory::Exists(testPath.Data()));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, RemoveContents)
{
    constexpr const char TestPath[] = "3f0c5a65-c9bb-4a1a-9b6d-1c74d6b90482";
    constexpr const char TestPath2[] = "3f0c5a65-c9bb-4a1a-9b6d-1c74d6b90482/60f3a0e3-1504-47ed-85d1-b36c89449c55";
    constexpr const char TestPath3[] = "3f0c5a65-c9bb-4a1a-9b6d-1c74d6b90482/60f3a0e3-1504-47ed-85d1-b36c89449c55/94b6c7f4-5ba3-42f1-aa7a-802e368dbd95";
    constexpr const char TestPath4[] = "3f0c5a65-c9bb-4a1a-9b6d-1c74d6b90482/243e8074-7130-46b6-9294-42e815eea7e9";
    const String testPath = GetTempTestPath(TestPath);
    const String testPath2 = GetTempTestPath(TestPath2);
    const String testPath3 = GetTempTestPath(TestPath3);
    const String testPath4 = GetTempTestPath(TestPath4);

    Directory::RemoveContents(testPath.Data());
    Directory::Remove(testPath.Data());

    HE_EXPECT(Directory::Create(testPath.Data()));
    HE_EXPECT(Directory::Create(testPath2.Data()));
    TouchTestFile(testPath3.Data(), TestPath3, HE_LENGTH_OF(TestPath3));
    TouchTestFile(testPath4.Data(), TestPath4, HE_LENGTH_OF(TestPath4));

    HE_EXPECT(Directory::Exists(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath2.Data()));
    HE_EXPECT(File::Exists(testPath3.Data()));
    HE_EXPECT(File::Exists(testPath4.Data()));

    HE_EXPECT(Directory::RemoveContents(testPath.Data()));
    HE_EXPECT(Directory::Exists(testPath.Data()));
    HE_EXPECT(!Directory::Exists(testPath2.Data()));
    HE_EXPECT(!File::Exists(testPath3.Data()));
    HE_EXPECT(!File::Exists(testPath4.Data()));
    HE_EXPECT(Directory::Remove(testPath.Data()));
}

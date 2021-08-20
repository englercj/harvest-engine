// Copyright Chad Engler

#include "he/core/directory.h"

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Scanner)
{
    Allocator& alloc = CrtAllocator::Get();

    Directory::Scanner scanner(alloc);
    String entry(alloc);

    HE_EXPECT(scanner.Open("."));
    while (scanner.NextEntry(entry))
    {
        HE_EXPECT_NE(entry, ".");
        HE_EXPECT_NE(entry, "..");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, GetSpecial)
{
    Allocator& alloc = CrtAllocator::Get();
    String dir(alloc);

    Result r = Directory::GetSpecial(dir, Directory::SpecialId::Documents);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());

    r = Directory::GetSpecial(dir, Directory::SpecialId::LocalAppData);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());

    r = Directory::GetSpecial(dir, Directory::SpecialId::SharedAppData);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());

    r = Directory::GetSpecial(dir, Directory::SpecialId::Temp);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, GetCurrent)
{
    Allocator& alloc = CrtAllocator::Get();
    String dir(alloc);

    Result r = Directory::GetCurrent(dir);
    HE_EXPECT(r);
    HE_EXPECT(!dir.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, SetCurrent)
{
    Allocator& alloc = CrtAllocator::Get();
    String dir(alloc);

    HE_EXPECT(Directory::GetCurrent(dir));
    HE_EXPECT(!dir.IsEmpty());
    HE_EXPECT(Directory::SetCurrent("/"));
    HE_EXPECT(Directory::SetCurrent(dir.Data()));

    String dir2(alloc);
    HE_EXPECT(Directory::GetCurrent(dir2));
    HE_EXPECT_EQ(dir, dir2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Rename)
{
    Directory::Remove("foo");
    Directory::Remove("bar");

    HE_EXPECT(Directory::Create("foo"));
    HE_EXPECT(Directory::Exists("foo"));
    HE_EXPECT(Directory::Rename("foo", "bar"));
    HE_EXPECT(!Directory::Exists("foo"));
    HE_EXPECT(Directory::Exists("bar"));
    HE_EXPECT(Directory::Remove("bar"));
    HE_EXPECT(!Directory::Exists("bar"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Exists)
{
    File f;

    File::Remove("foo");
    HE_EXPECT(f.Open("foo", FileOpenMode::WriteTruncate));
    HE_EXPECT(!Directory::Exists("foo"));
    f.Close();
    File::Remove("foo");

    HE_EXPECT(!Directory::Exists("foo"));
    HE_EXPECT(Directory::Create("foo"));
    HE_EXPECT(Directory::Exists("foo"));
    HE_EXPECT(Directory::Remove("foo"));
    HE_EXPECT(!Directory::Exists("foo"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Create)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, Remove)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, directory, RemoveContents)
{
    // TODO
}

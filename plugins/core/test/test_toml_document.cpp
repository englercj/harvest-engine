// Copyright Chad Engler

#include "he/core/toml_document.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Construct)
{
    TomlDocument doc;
    HE_EXPECT_EQ_PTR(&doc.GetAllocator(), &Allocator::GetDefault());
    HE_EXPECT(doc.Root().IsTable());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Read)
{
    TomlDocument doc;
    doc.Read("a = true\nb = 50\n[c]\nd = \"yes\"\n[[e]]\nf = true\n[[e]]\nf = false");

    TomlValue::TableType& root = doc.Root().Table();
    HE_EXPECT(root.Contains("a"));
    HE_EXPECT(root["a"].IsBool());
    HE_EXPECT_EQ(root["a"].Bool(), true);

    HE_EXPECT(root.Contains("b"));
    HE_EXPECT(root["b"].IsUint());
    HE_EXPECT_EQ(root["b"].Uint(), 50);

    HE_EXPECT(root.Contains("c"));
    HE_EXPECT(root["c"].IsTable());
    HE_EXPECT(root["c"]["d"].IsString());
    HE_EXPECT_EQ(root["c"]["d"].String(), "yes");

    HE_EXPECT(root.Contains("e"));
    HE_EXPECT(root["e"].IsArray());
    HE_EXPECT_EQ(root["e"].Array().Size(), 2);

    TomlValue& e0 = root["e"][0];
    HE_EXPECT(e0.IsTable());
    HE_EXPECT(e0.Table().Contains("f"));
    HE_EXPECT(e0["f"].IsBool());
    HE_EXPECT_EQ(e0.Bool(), true);

    TomlValue& e1 = root["e"][0];
    HE_EXPECT(e1.IsTable());
    HE_EXPECT(e1.Table().Contains("f"));
    HE_EXPECT(e1["f"].IsBool());
    HE_EXPECT_EQ(e1["f"].Bool(), false);

    HE_EXPECT(!root.Contains("d"));
    HE_EXPECT(!root.Contains("f"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Read_Complex)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Write)
{
    TomlDocument doc;

    doc["a"] = true;
    doc["b"] = 50;

    TomlValue::TableType& c = doc["c"].SetTable();
    c["d"] = "yes";

    TomlValue::ArrayType& e = doc["e"].SetArray();
    TomlValue::TableType& e0 = e.EmplaceBack().SetTable();
    e0["f"] = true;
    TomlValue::TableType& e1 = e.EmplaceBack().SetTable();
    e1["f"] = false;

    String toml = doc.ToString();
    HE_EXPECT_EQ(toml, "a = true\nb = 50\n[c]\nd = \"yes\"\n[[e]]\nf = true\n[[e]]\nf = false");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Write_Complex)
{
    // TODO
}

// Copyright Chad Engler

#include "he/schema/toml.h"

#include "he/core/test.h"
#include "he/schema/schema.h"

#include <iostream>

using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, toml, Roundtrip)
{
    Declaration::Reader schema = GetSchema(Declaration::DeclInfo);

    he::StringBuilder toml;
    HE_EXPECT(ToToml<Declaration>(toml, schema));

    Builder builder;
    HE_EXPECT(FromToml<Declaration>(builder, toml.Str().Data()));

    he::StringBuilder toml2;
    HE_EXPECT(ToToml<Declaration>(toml2, builder.Root().TryGetStruct<Declaration>()));

    HE_EXPECT_EQ(toml.Str(), toml2.Str());
}

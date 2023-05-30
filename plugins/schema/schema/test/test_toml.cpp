// Copyright Chad Engler

#include "he/schema/toml.h"

#include "he/core/string.h"
#include "he/core/test.h"
#include "he/schema/schema.h"

#include <iostream>

using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, toml, Roundtrip)
{
    Declaration::Reader schema = GetSchema(Declaration::DeclInfo);

    he::String toml;
    ToToml<Declaration>(toml, schema);

    Builder builder;
    HE_EXPECT(FromToml<Declaration>(builder, toml));

    he::String toml2;
    ToToml<Declaration>(toml2, builder.Root().TryGetStruct<Declaration>());

    HE_EXPECT_EQ(toml, toml2);
}

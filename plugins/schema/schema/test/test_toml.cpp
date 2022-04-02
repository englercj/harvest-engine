// Copyright Chad Engler

#include "he/schema/toml.h"

#include "he/core/test.h"
#include "he/schema/schema.h"

#include <iostream>

using namespace he::schema;

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, toml, ToToml)
{
    he::StringBuilder b;

    Declaration::Reader schema = GetSchema(Declaration::DeclInfo);

    HE_EXPECT(ToToml<Declaration>(b, schema));

    std::cout << b.Str().Data() << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(schema, toml, FromToml)
{
}

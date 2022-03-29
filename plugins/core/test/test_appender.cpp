// Copyright Chad Engler

#include "he/core/appender.h"

#include "he/core/buffer_writer.h"
#include "he/core/string.h"
#include "he/core/string_builder.h"
#include "he/core/test.h"
#include "he/core/vector.h"

using namespace he;

template <typename T>
void TestAppender(T& container)
{
    Appender<T> a(container);

    a = 'a'; ++a;
    a = 'b'; ++a;
    a = 'c'; ++a;

    HE_EXPECT_EQ(container.Size(), 3);
    HE_EXPECT_EQ(container[0], 'a');
    HE_EXPECT_EQ(container[1], 'b');
    HE_EXPECT_EQ(container[2], 'c');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, appender, BufferWriter)
{
    BufferWriter b;
    TestAppender(b);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, appender, String)
{
    String s;
    TestAppender(s);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, appender, StringBuilder)
{
    StringBuilder s;
    TestAppender(s);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, appender, Vector)
{
    Vector<char> v;
    TestAppender(v);
}

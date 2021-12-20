// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

namespace test;

struct Generic<T>
{
    struct C {}

    c @0 :C;
}

struct Test
{
    c @0 :Generic<AnyPointer>.C;
}

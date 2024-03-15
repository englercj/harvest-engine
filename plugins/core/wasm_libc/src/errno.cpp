// Copyright Chad Engler

#include "errno.h"

extern "C"
{
    static thread_local int __errno_storage = 0;

    int* __errno_location()
    {
        return &__errno_storage;
    }
}

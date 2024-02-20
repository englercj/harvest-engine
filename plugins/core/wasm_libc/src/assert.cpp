// Copyright Chad Engler

#include "assert.h"

#include "he/core/error.h"
#include "he/core/key_value.h"
#include "he/core/macros.h"

extern "C" __attribute__((__noreturn__)) void __assert_fail(const char* expr, const char* file, int line, const char* func)
{
    const he::ErrorSource source{ he::ErrorKind::Assert, line, file, func };
    const he::KeyValue kvList[]{ HE_KV(error_kind, he::ErrorKind::Assert), HE_KV(error_expr, expr) };
    return he::HandleError(source, kvList, HE_LENGTH_OF(kvList));
}

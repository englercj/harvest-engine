// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/log.h"

#define HE_SQLITE_ERROR(r, msg, ...) \
    HE_LOG_ERROR(he_sqlite, \
        HE_MSG(msg, __VA_ARGS__), \
        HE_KV(error, r), \
        HE_KV(error_str, sqlite3_errstr(r)), \
        HE_KV(error_msg, sqlite3_errmsg(m_db)))

#define HE_SQLITE_CHECK(result, ...) { \
    int r = (__VA_ARGS__); \
    if (!HE_VERIFY(r == result)) { \
        HE_SQLITE_ERROR(r, "SQLite error. Expected result " #result); \
        return false; \
    } \
}

#define HE_SQLITE_OK(...)   HE_SQLITE_CHECK(SQLITE_OK, __VA_ARGS__)
#define HE_SQLITE_DONE(...) HE_SQLITE_CHECK(SQLITE_DONE, __VA_ARGS__)

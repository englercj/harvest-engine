// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/log.h"

#define HE_SQLITE_CHECK(k, expr, ...) \
    do { \
        const int r_ = (expr); \
        if (!HE_VERIFY(r_ == SQLITE_ ## k, \
            HE_KV(check_expr, #expr), \
            HE_KV(result, r_), \
            HE_KV(result_str, sqlite3_errstr(r_)), \
            HE_KV(result_msg, sqlite3_errmsg(m_db)), ##__VA_ARGS__)) { \
            return false; \
        } \
    } while (0)

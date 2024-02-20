// Copyright Chad Engler

#include "stdio.h"

#include "limits.h"
#include "stdarg.h"



extern "C"
{
    int sprintf(char* __restrict s, const char* __restrict fmt, ...)
    {
        int ret;
        va_list args;
        va_start(args, fmt);
        ret = vsprintf(s, fmt, args);
        va_end(args);
        return ret;
    }

    int snprintf(char* __restrict s, size_t n, const char* __restrict fmt, ...)
    {
        int ret;
        va_list args;
        va_start(args, fmt);
        ret = vsnprintf(s, n, fmt, args);
        va_end(args);
        return ret;
    }

    int vsprintf(char* __restrict s, const char* __restrict fmt, va_list args)
    {
        return vsnprintf(s, INT_MAX, fmt, args);
    }

    int vsnprintf(char* __restrict s, size_t n, const char* __restrict fmt, va_list args)
    {
        unsigned char buf[1];
        char dummy[1];
        struct cookie c = { .s = n ? s : dummy, .n = n ? n-1 : 0 };
        FILE f = {
            .lbf = EOF,
            .write = sn_write,
            .lock = -1,
            .buf = buf,
            .cookie = &c,
        };

        if (n > INT_MAX) {
            errno = EOVERFLOW;
            return -1;
        }

        *c.s = 0;
        return vfprintf(&f, fmt, ap);
    }
}

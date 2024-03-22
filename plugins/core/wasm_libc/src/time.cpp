// Copyright Chad Engler

#include "time.h"

#include "errno.h"
#include "limits.h"

#include "wasm/libc.wasm.h"

#include "he/core/sync.h"

extern "C"
{
    char* tzname[2] = { 0, 0 };

    static char stdName[TZNAME_MAX+1];
    static char dstName[TZNAME_MAX+1];

    static int s_timezone = 0;
    static int s_daylight = 0;

    void tzset(void)
    {
        static he::Mutex s_mutex;
        static bool s_initialized = false;

        if (s_initialized)
            return;

        he::LockGuard lock(s_mutex);

        if (!s_initialized)
        {
            heWASM_TzSet(&s_timezone, &s_daylight, stdName, dstName);
            tzname[0] = stdName;
            tzname[1] = dstName;
            s_initialized = true;
        }
    }

    time_t mktime(struct tm* t)
    {
        tzset();
        const time_t time = heWASM_MkTime(t);
        if (time == -1)
        {
            errno = EOVERFLOW;
        }
        return time;
    }

    time_t timegm(struct tm* t)
    {
        tzset();
        return heWASM_TimeGm(t);
    }

    struct tm* gmtime(const time_t* time)
    {
        static struct tm t;
        return gmtime_r(time, &t);
    }

    struct tm* localtime(const time_t* time)
    {
        static struct tm t;
        return localtime_r(time, &t);
    }

    struct tm* gmtime_r(const time_t* __restrict time, struct tm* __restrict t)
    {
        tzset();
        heWASM_GmTime(*time, t);
        t->tm_isdst = 0;
        t->tm_gmtoff = 0;
        t->tm_zone = "GMT";
        return t;
    }

    struct tm* localtime_r(const time_t* __restrict time, struct tm* __restrict t)
    {
        tzset();
        heWASM_LocalTime(*time, t);
        t->tm_zone = t->tm_isdst ? tzname[1] : tzname[0];
        return t;
    }
}

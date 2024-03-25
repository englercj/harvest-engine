// Copyright Chad Engler

#include "time.h"

#include "errno.h"
#include "limits.h"

#include "wasm/libc.wasm.h"

#include "he/core/sync.h"
#include "he/core/tsa.h"

extern "C"
{
    char* tzname[2] = { 0, 0 };

    static he::Mutex s_tzMutex;
    static char s_stdName[TZNAME_MAX+1] HE_TSA_WRITE_GUARDED_BY(s_tzMutex){};
    static char s_dstName[TZNAME_MAX+1] HE_TSA_WRITE_GUARDED_BY(s_tzMutex){};
    static int s_timezone HE_TSA_WRITE_GUARDED_BY(s_tzMutex) = 0;
    static int s_daylight HE_TSA_WRITE_GUARDED_BY(s_tzMutex) = 0;

    void tzset(void)
    {
        static bool s_initialized = false;

        if (s_initialized)
            return;

        he::LockGuard lock(s_tzMutex);

        if (!s_initialized)
        {
            heWASM_TzSet(
                &s_timezone,
                &s_daylight,
                s_stdName,
                static_cast<uint32_t>(sizeof(s_stdName)),
                s_dstName,
                static_cast<uint32_t>(sizeof(s_dstName)));

            tzname[0] = s_stdName;
            tzname[1] = s_dstName;
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

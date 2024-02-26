// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int time_t;

struct timespec { time_t tv_sec; int tv_nsec; };

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
    int tm_gmtoff;
    const char* tm_zone;
};

extern char *tzname[2];

void tzset(void);

time_t mktime(struct tm* t);
time_t timegm(struct tm* t);

struct tm* gmtime(const time_t* time);
struct tm* localtime(const time_t* time);

struct tm* gmtime_r(const time_t* __restrict time, struct tm* __restrict t);
struct tm* localtime_r(const time_t* __restrict time, struct tm* __restrict t);

#ifdef __cplusplus
}
#endif

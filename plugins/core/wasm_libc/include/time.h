// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef long long time_t;

struct timespec { time_t tv_sec; long tv_nsec; };

#ifdef __cplusplus
}
#endif

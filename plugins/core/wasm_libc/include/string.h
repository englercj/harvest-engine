// Copyright Chad Engler

#pragma once

#include "_alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* __restrict dst, const void* __restrict src, size_t len);
void* memmove(void* dst, const void* src, size_t len);
void* memset(void* mem, int ch, size_t len);
int memcmp(const void* a, const void* b, size_t len);
void* memchr(const void* mem, int ch, size_t len);

char* strcpy(char* __restrict dst, const char* __restrict src);
char* strncpy(char* __restrict dst, const char* __restrict src, size_t len);

char* strcat(char* __restrict dst, const char* __restrict src);
char* strncat(char* __restrict dst, const char* __restrict src, size_t len);

int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t len);

int strcoll(const char* a, const char* b);
size_t strxfrm(char* __restrict dst, const char* __restrict src, size_t len);

char* strchr(const char* str, int ch);
char* strrchr(const char* str, int ch);

size_t strcspn(const char* s, const char* reject);
size_t strspn(const char* s, const char* accept);
char* strpbrk(const char* s, const char* accept);
char* strstr(const char* haystack, const char* needle);
char* strtok(char* __restrict s, const char* __restrict delim);

size_t strlen(const char* str);

char* strerror(int e);

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
    char* strtok_r(char* __restrict s, const char* __restrict delim, char** __restrict saveptr);
    int strerror_r(int e, char* dst, size_t dstLen);
    char* stpcpy(char* __restrict dst, const char* __restrict src);
    char* stpncpy(char* __restrict dst, const char* __restrict src, size_t len);
    size_t strnlen(const char* str, size_t len);
    char* strdup(const char* src);
    char* strndup(const char* src, size_t len);
    char* strsignal(int sig);
    char* strerror_l(int e, locale_t loc);
    int strcoll_l(const char* a, const char* b, locale_t loc);
    size_t strxfrm_l(char* __restrict dst, const char* __restrict src, size_t len, locale_t loc);
    // void* memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen);
#endif

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
    void* memccpy(void* __restrict dst, const void* __restrict src, int c, size_t len);
#endif

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
    char* strsep(char** pstr, const char* delim);
    size_t strlcat(char* dst, const char* src, size_t len);
    size_t strlcpy(char* dst, const char* src, size_t len);
    void explicit_bzero(void* dst, size_t len);
#endif

#ifdef _GNU_SOURCE
    #define strdupa(x) strcpy(__builtin_alloca(strlen(x) + 1), (x))
    // int strverscmp(const char* a, const char* b);
    char* strchrnul(const char* s, int c);
    char* strcasestr(const char* haystack, const char* needle);
    void* memrchr(const void* mem, int ch, size_t len);
    void* mempcpy(void* dst, const void* src, size_t len);
#endif

#ifdef __cplusplus
}
#endif

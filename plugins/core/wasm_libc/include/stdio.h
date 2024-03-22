// Copyright Chad Engler

#pragma once

#include "_alltypes.h"
#include "_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union _fpos64_t
{
	char __opaque[16];
	long long __lldata;
	double __align;
} fpos_t;

#undef EOF
#define EOF (-1)

#define UNGET 8

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define BUFSIZ 1024
#define FILENAME_MAX 4096
#define FOPEN_MAX 1000
#define TMP_MAX 10000
#define L_tmpnam 20

extern FILE* const stdin;
extern FILE* const stdout;
extern FILE* const stderr;

#define stdin  (stdin)
#define stdout (stdout)
#define stderr (stderr)

FILE* fopen(const char* __restrict path, const char* __restrict mode);
FILE* freopen(const char* __restrict path, const char* __restrict mode, FILE* __restrict stream);
int fclose(FILE* stream);

int remove(const char* path);
int rename(const char* path, const char* newPath);

int feof(FILE* stream);
int ferror(FILE* stream);
int fflush(FILE* stream);
void clearerr(FILE* stream);

int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
void rewind(FILE* stream);

int fgetpos(FILE* __restrict stream, fpos_t* __restrict pos);
int fsetpos(FILE* stream, const fpos_t* pos);

size_t fread(void* __restrict ptr, size_t size, size_t nmemb, FILE* __restrict stream);
size_t fwrite(const void* __restrict ptr, size_t size, size_t nmemb, FILE* __restrict stream);

int fgetc(FILE* stream);
int getc(FILE* stream);
int getchar(void);
// int ungetc(int c, FILE* stream);

int fputc(int c, FILE* stream);
int putc(int c, FILE* stream);
int putchar(int c);

// char* fgets(char* __restrict s, int size, FILE* __restrict stream);
#if __STDC_VERSION__ < 201112L
// char* gets(char* s);
#endif

int fputs(const char* __restrict s, FILE* __restrict stream);
int puts(const char* s);

int printf(const char* __restrict format, ...);
int fprintf(FILE* __restrict stream, const char* __restrict format, ...);
int sprintf(char* __restrict s, const char* __restrict format, ...);
int snprintf(char* __restrict s, size_t size, const char* __restrict format, ...);

int vprintf(const char* __restrict format, __builtin_va_list args);
int vfprintf(FILE* __restrict stream, const char* __restrict format, __builtin_va_list args);
int vsprintf(char* __restrict s, const char* __restrict format, __builtin_va_list args);
int vsnprintf(char* __restrict s, size_t size, const char* __restrict format, __builtin_va_list args);

int scanf(const char* __restrict format, ...);
int fscanf(FILE* __restrict stream, const char* __restrict format, ...);
int sscanf(const char* __restrict s, const char* __restrict format, ...);
int vscanf(const char* __restrict format, __builtin_va_list args);
int vfscanf(FILE* __restrict stream, const char* __restrict format, __builtin_va_list args);
int vsscanf(const char* __restrict s, const char* __restrict format, __builtin_va_list args);

void perror(const char* msg);

// int setvbuf(FILE* __restrict stream, char* __restrict buf, int mode, size_t size);
// void setbuf(FILE* __restrict stream, char* __restrict buf);

char* tmpnam(char* s);
FILE* tmpfile(void);

// #if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
//     || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
//     || defined(_BSD_SOURCE)
//     FILE *fmemopen(void *__restrict, size_t, const char *__restrict);
//     FILE *open_memstream(char **, size_t *);
//     FILE *fdopen(int, const char *);
//     FILE *popen(const char *, const char *);
//     int pclose(FILE *);
//     int fileno(FILE *);
//     int fseeko(FILE *, off_t, int);
//     off_t ftello(FILE *);
//     int dprintf(int, const char *__restrict, ...);
//     int vdprintf(int, const char *__restrict, __isoc_va_list);
//     void flockfile(FILE *);
//     int ftrylockfile(FILE *);
//     void funlockfile(FILE *);
//     int getc_unlocked(FILE *);
//     int getchar_unlocked(void);
//     int putc_unlocked(int, FILE *);
//     int putchar_unlocked(int);
//     ssize_t getdelim(char **__restrict, size_t *__restrict, int, FILE *__restrict);
//     ssize_t getline(char **__restrict, size_t *__restrict, FILE *__restrict);
//     int renameat(int, const char *, int, const char *);
//     char *ctermid(char *);
//     #define L_ctermid 20
// #endif

// #if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
//     #define P_tmpdir "/tmp"
//     char* tempnam(const char* dir, const char* pfx);
// #endif

// #if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
//     #define L_cuserid 20
//     char *cuserid(char *);
//     void setlinebuf(FILE *);
//     void setbuffer(FILE *, char *, size_t);
//     int fgetc_unlocked(FILE *);
//     int fputc_unlocked(int, FILE *);
//     int fflush_unlocked(FILE *);
//     size_t fread_unlocked(void *, size_t, size_t, FILE *);
//     size_t fwrite_unlocked(const void *, size_t, size_t, FILE *);
//     void clearerr_unlocked(FILE *);
//     int feof_unlocked(FILE *);
//     int ferror_unlocked(FILE *);
//     int fileno_unlocked(FILE *);
//     int getw(FILE *);
//     int putw(int, FILE *);
//     char *fgetln(FILE *, size_t *);
//     int asprintf(char **, const char *, ...);
//     int vasprintf(char **, const char *, __isoc_va_list);
//     #endif

//     #ifdef _GNU_SOURCE
//     char *fgets_unlocked(char *, int, FILE *);
//     int fputs_unlocked(const char *, FILE *);

//     typedef ssize_t (cookie_read_function_t)(void *, char *, size_t);
//     typedef ssize_t (cookie_write_function_t)(void *, const char *, size_t);
//     typedef int (cookie_seek_function_t)(void *, off_t *, int);
//     typedef int (cookie_close_function_t)(void *);

//     typedef struct _IO_cookie_io_functions_t {
//         cookie_read_function_t *read;
//         cookie_write_function_t *write;
//         cookie_seek_function_t *seek;
//         cookie_close_function_t *close;
//     } cookie_io_functions_t;

//     FILE *fopencookie(void *, const char *, cookie_io_functions_t);
// #endif

#if defined(_LARGEFILE64_SOURCE)
    #define tmpfile64 tmpfile
    #define fopen64 fopen
    #define freopen64 freopen
    #define fseeko64 fseeko
    #define ftello64 ftello
    #define fgetpos64 fgetpos
    #define fsetpos64 fsetpos
    #define fpos64_t fpos_t
    #define off64_t off_t
#endif

#ifdef __cplusplus
}
#endif

// Copyright Chad Engler

#include "string.h"

#include "errno.h"
#include "signal.h"

#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/core/types.h"

extern "C"
{
    _Hidden static const char* const ErrorStrings[] =
    {
        "Success",                                  // (0)
        "Operation not permitted",                  // EPERM (1)
        "No such file or directory",                // ENOENT (2)
        "No such process",                          // ESRCH (3)
        "Interrupted system call",                  // EINTR (4)
        "I/O error",                                // EIO (5)
        "No such device or address",                // ENXIO (6)
        "Argument list too long",                   // E2BIG (7)
        "Exec format error",                        // ENOEXEC (8)
        "Bad file descriptor",                      // EBADF (9)
        "No child process",                         // ECHILD (10)
        "Resource temporarily unavailable",         // EAGAIN / EWOULDBLOCK (11)
        "Out of memory",                            // ENOMEM (12)
        "Permission denied",                        // EACCES (13)
        "Bad address",                              // EFAULT (14)
        "Block device required",                    // ENOTBLK (15)
        "Resource busy",                            // EBUSY (16)
        "File exists",                              // EEXIST (17)
        "Cross-device link",                        // EXDEV (18)
        "No such device",                           // ENODEV (19)
        "Not a directory",                          // ENOTDIR (20)
        "Is a directory",                           // EISDIR (21)
        "Invalid argument",                         // EINVAL (22)
        "Too many open files in system",            // ENFILE (23)
        "No file descriptors available",            // EMFILE (24)
        "Not a tty",                                // ENOTTY (25)
        "Text file busy",                           // ETXTBSY (26)
        "File too large",                           // EFBIG (27)
        "No space left on device",                  // ENOSPC (28)
        "Invalid seek",                             // ESPIPE (29)
        "Read-only file system",                    // EROFS (30)
        "Too many links",                           // EMLINK (31)
        "Broken pipe",                              // EPIPE (32)
        "Domain error",                             // EDOM (33)
        "Result not representable",                 // ERANGE (34)
        "Resource deadlock would occur",            // EDEADLK / EDEADLOCK (35)
        "Filename too long",                        // ENAMETOOLONG (36)
        "No locks available",                       // ENOLCK (37)
        "Function not implemented",                 // ENOSYS (38)
        "Directory not empty",                      // ENOTEMPTY (39)
        "Symbolic link loop",                       // ELOOP (40)
        "", // (41)
        "No message of desired type",               // ENOMSG (42)
        "Identifier removed",                       // EIDRM (43)
        "", // ECHRNG (44)
        "", // EL2NSYNC (45)
        "", // EL3HLT (46)
        "", // EL3RST (47)
        "", // ELNRNG (48)
        "", // EUNATCH (49)
        "", // ENOCSI (50)
        "", // EL2HLT (51)
        "", // EBADE (52)
        "", // EBADR (53)
        "", // EXFULL (54)
        "", // ENOANO (55)
        "", // EBADRQC (56)
        "", // EBADSLT (57)
        "", // (58)
        "", // EBFONT (59)
        "Device not a stream",                      // ENOSTR (60)
        "No data available",                        // ENODATA (61)
        "Device timeout",                           // ETIME (62)
        "Out of streams resources",                 // ENOSR (63)
        "", // ENONET (64)
        "", // ENOPKG (65)
        "", // EREMOTE (66)
        "Link has been severed",                    // ENOLINK (67)
        "", // EADV (68)
        "", // ESRMNT (69)
        "", // ECOMM (70)
        "Protocol error",                           // EPROTO (71)
        "Multihop attempted",                       // EMULTIHOP (72)
        "", // EDOTDOT (73)
        "Bad message",                              // EBADMSG (74)
        "Value too large for data type",            // EOVERFLOW (75)
        "", // ENOTUNIQ (76)
        "File descriptor in bad state",             // EBADFD (77)
        "", // EREMCHG (78)
        "", // ELIBACC (79)
        "", // ELIBBAD (80)
        "", // ELIBSCN (81)
        "", // ELIBMAX (82)
        "", // ELIBEXEC (83)
        "Illegal byte sequence",                    // EILSEQ (84)
        "", // ERESTART (85)
        "", // ESTRPIPE (86)
        "", // EUSERS (87)
        "Not a socket",                             // ENOTSOCK (88)
        "Destination address required",             // EDESTADDRREQ (89)
        "Message too large",                        // EMSGSIZE (90)
        "Protocol wrong type for socket",           // EPROTOTYPE (91)
        "Protocol not available",                   // ENOPROTOOPT (92)
        "Protocol not supported",                   // EPROTONOSUPPORT (93)
        "Socket type not supported",                // ESOCKTNOSUPPORT (94)
        "Not supported",                            // EOPNOTSUPP / ENOTSUP (95)
        "Protocol family not supported",            // EPFNOSUPPORT (96)
        "Address family not supported by protocol", // EAFNOSUPPORT (97)
        "Address in use",                           // EADDRINUSE (98)
        "Address not available",                    // EADDRNOTAVAIL (99)
        "Network is down",                          // ENETDOWN (100)
        "Network unreachable",                      // ENETUNREACH (101)
        "Connection reset by network",              // ENETRESET (102)
        "Connection aborted",                       // ECONNABORTED (103)
        "Connection reset by peer",                 // ECONNRESET (104)
        "No buffer space available",                // ENOBUFS (105)
        "Socket is connected",                      // EISCONN (106)
        "Socket not connected",                     // ENOTCONN (107)
        "Cannot send after socket shutdown",        // ESHUTDOWN (108)
        "", // ETOOMANYREFS (109)
        "Operation timed out",                      // ETIMEDOUT (110)
        "Connection refused",                       // ECONNREFUSED (111)
        "Host is down",                             // EHOSTDOWN (112)
        "Host is unreachable",                      // EHOSTUNREACH (113)
        "Operation already in progress",            // EALREADY (114)
        "Operation in progress",                    // EINPROGRESS (115)
        "Stale file handle",                        // ESTALE (116)
        "", // EUCLEAN (117)
        "", // ENOTNAM (118)
        "", // ENAVAIL (119)
        "", // EISNAM (120)
        "Remote I/O error",                         // EREMOTEIO (121)
        "Quota exceeded",                           // EDQUOT (122)
        "No medium found",                          // ENOMEDIUM (123)
        "Wrong medium type",                        // EMEDIUMTYPE (124)
        "Operation canceled",                       // ECANCELED (125)
        "Required key not available",               // ENOKEY (126)
        "Key has expired",                          // EKEYEXPIRED (127)
        "Key has been revoked",                     // EKEYREVOKED (128)
        "Key was rejected by service",              // EKEYREJECTED (129)
        "Previous owner died",                      // EOWNERDEAD (130)
        "State not recoverable",                    // ENOTRECOVERABLE (131)
        "", // ERFKILL (132)
        "", // EHWPOISON (133)
    };
    static_assert(HE_LENGTH_OF(ErrorStrings) == 134);

    _Hidden static const char* UnknownErrorString = "Unknown error";

    _Hidden static const char* const SignalStrings[] =
    {
        "Unknown signal",           // (0)
        "Hangup",                   // SIGHUP (1)
        "Interrupt",                // SIGINT (2)
        "Quit",                     // SIGQUIT (3)
        "Illegal instruction",      // SIGILL (4)
        "Trace/breakpoint trap",    // SIGTRAP (5)
        "Aborted",                  // SIGABRT / SIGIOT (6)
        "Bus error",                // SIGBUS (7)
        "Arithmetic exception",     // SIGFPE (8)
        "Killed",                   // SIGKILL (9)
        "User defined signal 1",    // SIGUSR1 (10)
        "Segmentation fault",       // SIGSEGV (11)
        "User defined signal 2",    // SIGUSR2 (12)
        "Broken pipe",              // SIGPIPE (13)
        "Alarm clock",              // SIGALRM (14)
        "Terminated",               // SIGTERM (15)
        "Stack fault",              // SIGSTKFLT (16)
        "Child process status",     // SIGCHLD (17)
        "Continued",                // SIGCONT (18)
        "Stopped (signal)",         // SIGSTOP (19)
        "Stopped",                  // SIGTSTP (20)
        "Stopped (tty input)",      // SIGTTIN (21)
        "Stopped (tty output)",     // SIGTTOU (22)
        "Urgent I/O condition",     // SIGURG (23)
        "CPU time limit exceeded",  // SIGXCPU (24)
        "File size limit exceeded", // SIGXFSZ (25)
        "Virtual timer expired",    // SIGVTALRM (26)
        "Profiling timer expired",  // SIGPROF (27)
        "Window changed",           // SIGWINCH (28)
        "I/O possible",             // SIGIO / SIGPOLL (29)
        "Power failure",            // SIGPWR (30)
        "Bad system call",          // SIGSYS / SIGUNUSED (31)
        "RT32", "RT33", "RT34", "RT35", "RT36", "RT37", "RT38", "RT39",
        "RT40", "RT41", "RT42", "RT43", "RT44", "RT45", "RT46", "RT47", "RT48", "RT49",
        "RT50", "RT51", "RT52", "RT53", "RT54", "RT55", "RT56", "RT57", "RT58", "RT59",
        "RT60", "RT61", "RT62", "RT63", "RT64",
    };
    static_assert(HE_LENGTH_OF(SignalStrings) == NSIG);

    void* memcpy(void* __restrict dst, const void* __restrict src, size_t len)
    {
        return he::MemCopy(dst, src, len);
    }

    void* memmove(void* dst, const void* src, size_t len)
    {
        return he::MemMove(dst, src, len);
    }

    void* memset(void* mem, int ch, size_t len)
    {
        return he::MemSet(mem, ch, len);
    }

    int memcmp(const void* a, const void* b, size_t len)
    {
        return he::MemCmp(a, b, len);
    }

    void* memchr(const void* mem, int ch, size_t len)
    {
        return const_cast<void*>(he::MemChr(mem, ch, len));
    }

    char* strcpy(char* __restrict dst, const char* __restrict src)
    {
        he::StrCopy(static_cast<char*>(dst), 0xffffffff, static_cast<const char*>(src));
        return dst;
    }

    char* strncpy(char* __restrict dst, const char* __restrict src, size_t len)
    {
        he::StrCopyN(static_cast<char*>(dst), 0xffffffff, static_cast<const char*>(src), len);
        return dst;
    }

    char* strcat(char* __restrict dst, const char* __restrict src)
    {
        he::StrCat(static_cast<char*>(dst), 0xffffffff, static_cast<const char*>(src));
        return dst;
    }

    char* strncat(char* __restrict dst, const char* __restrict src, size_t len)
    {
        he::StrCatN(static_cast<char*>(dst), 0xffffffff, static_cast<const char*>(src), len);
        return dst;
    }

    int strcmp(const char* a, const char* b)
    {
        return he::StrComp(a, b);
    }

    int strncmp(const char* a, const char* b, size_t len)
    {
        return he::StrCompN(a, b, len);
    }

    int strcoll(const char* a, const char* b)
    {
        return strcmp(a, b);
    }

    size_t strxfrm(char* __restrict dst, const char* __restrict src, size_t len)
    {
        const size_t n = strlen(src);
        if (len > n)
            strcpy(dst, src);
        return n;
    }

    char* strchr(const char* str, int ch)
    {
        return const_cast<char*>(he::StrFind(str, ch));
    }

    char* strrchr(const char* str, int ch)
    {
        return const_cast<char*>(he::StrFindLast(str, ch));
    }

    size_t strcspn(const char* s, const char* c)
    {
        const char* a = s;
        size_t byteset[32 / sizeof(size_t)];

        if (!c[0] || !c[1])
        {
            const char* r = he::StrFind(s, *c);
            return r ? r - a : strlen(s);
        }

        #define BITOP(a, b, op) ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

        memset(byteset, 0, sizeof(byteset));
        while (*c && BITOP(byteset, static_cast<unsigned char>(*c), |=)) { ++c; }
        while (*s && !BITOP(byteset, static_cast<unsigned char>(*s), &)) { ++s; }

        #undef BITOP

        return s - a;
    }

    size_t strspn(const char* s, const char* c)
    {
        const char* a = s;
        size_t byteset[32 / sizeof(size_t)]{};

        if (!c[0])
            return 0;

        if (!c[1])
        {
            while (*s == *c) { ++s; }
            return s - a;
        }

        #define BITOP(a, b, op) ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

        while (*c && BITOP(byteset, static_cast<unsigned char>(*c), |=)) { ++c; }
        while (*s && BITOP(byteset, static_cast<unsigned char>(*s), &)) { ++s; }

        #undef BITOP

        return s - a;
    }

    char* strpbrk(const char* s, const char* accept)
    {
        s += strcspn(s, accept);
        return *s ? const_cast<char*>(s) : nullptr;
    }

    char* strstr(const char* haystack, const char* needle)
    {
        const char* r = he::StrFind(haystack, needle);
        return const_cast<char*>(r);
    }

    char* strtok(char* __restrict s, const char* __restrict delim)
    {
        static char* p = nullptr;

        if (!s && !(s = p))
            return nullptr;

        s += strspn(s, delim);

        if (!*s)
        {
            p = nullptr;
            return p;
        }

        p = s + strcspn(s, delim);

        if (*p)
            *p++ = '\0';
        else
            p = nullptr;

        return s;
    }

    size_t strlen(const char* str)
    {
        return he::StrLen(str);
    }

    char* strerror(int e)
    {
        if (e < 0 || static_cast<uint32_t>(e) >= HE_LENGTH_OF(ErrorStrings))
        {
            return const_cast<char*>(UnknownErrorString);
        }

        return const_cast<char*>(ErrorStrings[e]);
    }

    char* strtok_r(char* __restrict s, const char* __restrict delim, char** __restrict p)
    {
        if (!s && !(s = *p))
            return nullptr;

        s += strspn(s, delim);

        if (!*s)
        {
            *p = nullptr;
            return nullptr;
        }

        *p = s + strcspn(s, delim);

        if (**p)
            *(*p)++ = '\0';
        else
            *p = nullptr;

        return s;
    }

    int strerror_r(int e, char* dst, size_t dstLen)
    {
        char* msg = strerror(e);
        const uint32_t len = he::StrLen(msg);
        if (len >= dstLen)
        {
            if (dstLen)
            {
                he::MemCopy(dst, msg, dstLen - 1);
                dst[dstLen - 1] = '\0';
            }
            return ERANGE;
        }

        he::MemCopy(dst, msg, len + 1);
        return 0;
    }

    char* stpcpy(char* __restrict dst, const char* __restrict src)
    {
        const uint32_t num = he::StrCopy(static_cast<char*>(dst), 0xffffffff, static_cast<const char*>(src));
        return dst + num;
    }

    char* stpncpy(char* __restrict dst, const char* __restrict src, size_t len)
    {
        const uint32_t num = he::StrCopyN(static_cast<char*>(dst), 0xffffffff, static_cast<const char*>(src), len);
        return dst + num;
    }

    size_t strnlen(const char* str, size_t len)
    {
        const char* p = static_cast<const char*>(memchr(str, 0, len));
        return p ? (p - str) : len;
    }

    char* strdup(const char* src)
    {
        return he::StrDup(src);
    }

    char* strndup(const char* src, size_t len)
    {
        return he::StrDupN(src, len);
    }

    char* strsignal(int sig)
    {
        if (sig < 0 || static_cast<uint32_t>(sig) >= HE_LENGTH_OF(SignalStrings))
        {
            return const_cast<char*>(SignalStrings[0]);
        }

        return const_cast<char*>(SignalStrings[sig]);
    }

    char* strerror_l(int e, [[maybe_unused]] locale_t loc)
    {
        return strerror(e);
    }

    int strcoll_l(const char* a, const char* b, [[maybe_unused]] locale_t loc)
    {
        return strcoll(a, b);
    }

    size_t strxfrm_l(char* __restrict dst, const char* __restrict src, size_t len, [[maybe_unused]] locale_t loc)
    {
        return strxfrm(dst, src, len);
    }

    // void* memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen)
    // {
    //     // TODO
    // }

    void* memccpy(void* __restrict dst, const void* __restrict src, int c, size_t len)
    {
        uint8_t* d = static_cast<uint8_t*>(dst);
        const uint8_t* s = static_cast<const uint8_t*>(src);

        while (len && (*d = *s) != c)
        {
            --len;
            ++d;
            ++s;
        }

        if (len)
            return d + 1;

        return nullptr;
    }

    char* strsep(char** pstr, const char* delim)
    {
        char* s = *pstr;

        if (!s)
            return nullptr;

        char* end = s + strcspn(s, delim);

        if (*end)
            *end++ = '\0';
        else
            end = nullptr;

        *pstr = end;
        return s;
    }

    size_t strlcat(char* dst, const char* src, size_t len)
    {
        const size_t l = strnlen(dst, len);

        if (l == len)
            return l + strlen(src);

        return l + strlcpy(dst + l, src, len - l);
    }

    size_t strlcpy(char* dst, const char* src, size_t len)
    {
        if (!len--)
            return strlen(src);

        char* begin = dst;

        while (len && (*dst = *src))
        {
            --len;
            ++dst;
            ++src;
        }

        *dst = '\0';
        return (dst - begin) + strlen(src);
    }

    void explicit_bzero(void* dst, size_t len)
    {
        dst = memset(dst, 0, len);
        asm volatile("" :: "r"(dst) : "memory");
    }

    // int strverscmp(const char* a, const char* b)
    // {
    //     // TODO
    // }

    char* strchrnul(const char* s, int c)
    {
        const char* p = strchr(s, c);
        return p ? const_cast<char*>(p) : const_cast<char*>(s + strlen(s));
    }

    char* strcasestr(const char* haystack, const char* needle)
    {
        const size_t len = strlen(needle);

        while (*haystack)
        {
            if (he::StrEqualNI(haystack, needle, len))
                return const_cast<char*>(haystack);

            ++haystack;
        }
        return nullptr;
    }

    void* memrchr(const void* mem, int ch, size_t len)
    {
        const uint8_t* mem8 = static_cast<const uint8_t*>(mem);
        const uint8_t ch8 = static_cast<uint8_t>(ch);

        while (len--)
        {
            if (mem8[len] == ch8)
                return const_cast<void*>(static_cast<const void*>(mem8 + len));
        }
        return nullptr;
    }

    void* mempcpy(void* dst, const void* src, size_t len)
    {
        return static_cast<char*>(memcpy(dst, src, len)) + len;
    }
}

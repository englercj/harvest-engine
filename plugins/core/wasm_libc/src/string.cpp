// Copyright Chad Engler

#include "string.h"

#include "errno.h"

#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/core/types.h"

extern "C"
{
    static const char* const ErrorStrings[] =
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

    static const char* UnknownErrorString = "Unknown error";

    char* strerror(int e)
    {
        if (e < 0 || e >= HE_LENGTH_OF(ErrorStrings))
        {
            return const_cast<char*>(UnknownErrorString);
        }

        return const_cast<char*>(ErrorStrings[e]);
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
}

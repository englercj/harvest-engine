// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/sync.h"

#include "fmt/core.h"

#include <cstdlib>

namespace he
{
    static ErrorHandlerFunc s_errorHandler = nullptr;
    static void* s_errorHandlerUserData = nullptr;
    static RWLock s_errorHandlerLock{};

    bool DefaultErrorHandler(void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        const LogSource logSource{ LogLevel::Error, source.line, source.file, source.funcName, "app_error" };
        Log(logSource, kvs, count);

        // TODO: Platform-specific handlers (popup for win32)

        // Error kind is always the first KV
        ErrorKind kind = kvs[0].GetEnum<ErrorKind>();
        switch (kind)
        {
            case ErrorKind::Assert:
            case ErrorKind::Except:
                // TODO: std::abort(); or similar
                break;
            case ErrorKind::Expect:
            case ErrorKind::Verify:
                break;
        }

        return true;
    }

    void SetErrorHandler(ErrorHandlerFunc handler, void* userData)
    {
        LockGuard lock(s_errorHandlerLock);

        s_errorHandler = handler;
        s_errorHandlerUserData = userData;
    }

    ErrorHandlerFunc GetErrorHandler(void*& userData)
    {
        s_errorHandlerLock.AcquireRead();

        ErrorHandlerFunc handler = s_errorHandler;
        userData = s_errorHandlerUserData;

        s_errorHandlerLock.ReleaseRead();

        return handler ? handler : DefaultErrorHandler;
    }

    bool HandleError(const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        s_errorHandlerLock.AcquireRead();

        ErrorHandlerFunc handler = s_errorHandler;
        void* userData = s_errorHandlerUserData;

        s_errorHandlerLock.ReleaseRead();

        if (handler)
            return handler(userData, source, kvs, count);

        return DefaultErrorHandler(nullptr, source, kvs, count);
    }

    template <>
    const char* AsString(ErrorKind x)
    {
        switch (x)
        {
            case ErrorKind::Assert: return "Assertion";
            case ErrorKind::Except: return "Exception";
            case ErrorKind::Expect: return "Expectation";
            case ErrorKind::Verify: return "Verify";
        }

        return "<unknown>";
    }
}

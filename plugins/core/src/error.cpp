// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/sync.h"

#include <cstdlib>

namespace he
{
    static Pfn_ErrorHandler s_errorHandler = nullptr;
    static void* s_errorHandlerUserData = nullptr;
    static RWLock s_errorHandlerLock{};

    extern bool _PlatformErrorHandler(const ErrorSource& source, const KeyValue* kvs, uint32_t count);

    bool DefaultErrorHandler(void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        LogSource logSource;
        logSource.level = LogLevel::Error;
        logSource.line = source.line;
        logSource.file = source.file;
        logSource.funcName = source.funcName;
        logSource.category = "app_error";

        Log(logSource, kvs, count);

        return _PlatformErrorHandler(source, kvs, count);
    }

    void SetErrorHandler(Pfn_ErrorHandler handler, void* userData)
    {
        LockGuard lock(s_errorHandlerLock);

        s_errorHandler = handler;
        s_errorHandlerUserData = userData;
    }

    Pfn_ErrorHandler GetErrorHandler(void*& userData)
    {
        s_errorHandlerLock.AcquireRead();

        Pfn_ErrorHandler handler = s_errorHandler;
        userData = s_errorHandlerUserData;

        s_errorHandlerLock.ReleaseRead();

        return handler ? handler : DefaultErrorHandler;
    }

    bool HandleError(const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        s_errorHandlerLock.AcquireRead();

        Pfn_ErrorHandler handler = s_errorHandler;
        void* userData = s_errorHandlerUserData;

        s_errorHandlerLock.ReleaseRead();

        if (handler)
            return handler(userData, source, kvs, count);

        return DefaultErrorHandler(nullptr, source, kvs, count);
    }

    template <>
    const char* EnumTraits<ErrorKind>::ToString(ErrorKind x) noexcept
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

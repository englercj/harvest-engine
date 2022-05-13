// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/alloca.h"
#include "he/core/debug.h"
#include "he/core/enum_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"

#include "fmt/core.h"

#include <algorithm>
#include <cstdlib>
#include <shared_mutex>

namespace he
{
    static ErrorHandlerFunc s_errorHandler = nullptr;
    static void* s_errorHandlerUserData = nullptr;
    static std::shared_mutex s_errorHandlerMutex{};

    bool DefaultErrorHandler(void*, const ErrorSource& source, const LogKV* kvs, uint32_t count)
    {
        const LogSource logSource{ he::LogLevel::Error, source.line, source.file, source.funcName, "app_error" };
        LogKV* extendedKvs = HE_ALLOCA(LogKV, count + 2);
        std::copy(kvs, kvs + count, extendedKvs);
        extendedKvs[count + 0] = HE_KV(error_type, source.type);
        extendedKvs[count + 1] = HE_KV(error_expr, source.expression);
        he::Log(logSource, extendedKvs, count + 2);

        // TODO: Platform-specific handlers (popup for win32)
        switch (source.type)
        {
            case ErrorType::Assert:
            case ErrorType::Except: //exit(-1); break;
            case ErrorType::Verify:
            case ErrorType::Expect: break;
        }

        return true;
    }

    void SetErrorHandler(ErrorHandlerFunc handler, void* userData = nullptr)
    {
        s_errorHandlerMutex.lock();

        s_errorHandler = handler;
        s_errorHandlerUserData = userData;

        s_errorHandlerMutex.unlock();
    }

    ErrorHandlerFunc GetErrorHandler()
    {
        return s_errorHandler ? s_errorHandler : DefaultErrorHandler;
    }

    bool HandleError(const ErrorSource& source, const LogKV* kvs, uint32_t count)
    {
        s_errorHandlerMutex.lock_shared();

        ErrorHandlerFunc handler = s_errorHandler;
        void* userData = s_errorHandlerUserData;

        s_errorHandlerMutex.unlock_shared();

        if (handler)
            return handler(userData, source, kvs, count);

        return DefaultErrorHandler(nullptr, source, kvs, count);
    }

    template <>
    const char* AsString(ErrorType x)
    {
        switch (x)
        {
            case ErrorType::Assert: return "Assertion";
            case ErrorType::Except: return "Exception";
            case ErrorType::Verify: return "Verify";
            case ErrorType::Expect: return "Expectation";
        }

        return "<unknown>";
    }
}

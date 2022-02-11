// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/debug.h"
#include "he/core/enum_ops.h"
#include "he/core/log.h"

#include "fmt/core.h"

#include <cstdlib>

namespace he
{
    static ErrorHandlerFunc s_errorHandler = nullptr;

    bool DefaultErrorHandler(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg)
    {
        const LogSource source{ he::LogLevel::Error, line, file, funcName, "app_error" };
        const LogKV kvs[] =
        {
            HE_KV(type, "{}", type),
            HE_KV(expr, expression),
            HE_MSG(msg ? msg : ""),
        };
        he::Log(source, kvs, HE_LENGTH_OF(kvs));

        // TODO: Platform-specific handlers (popup for win32)
        switch (type)
        {
            case ErrorType::Assert:
            case ErrorType::Except: //exit(-1); break;
            case ErrorType::Verify:
            case ErrorType::Expect: break;
        }

        return true;
    }

    void SetErrorHandler(ErrorHandlerFunc handler)
    {
        s_errorHandler = handler;
    }

    ErrorHandlerFunc GetErrorHandler()
    {
        return s_errorHandler ? s_errorHandler : DefaultErrorHandler;
    }

    bool HandleError(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg)
    {
        if (s_errorHandler)
            return s_errorHandler(type, file, line, funcName, expression, msg);

        return DefaultErrorHandler(type, file, line, funcName, expression, msg);
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

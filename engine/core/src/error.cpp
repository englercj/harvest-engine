#include "he/core/error.h"

#include "he/core/debug.h"
#include "he/core/log.h"

#include "fmt/format.h"

namespace he
{
    static ErrorHandlerFunc g_errorHandler = nullptr;

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

    bool DefaultErrorHandler(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg)
    {
        const LogSource source{ he::LogLevel::Error, line, file, funcName };
        const LogKV kvs[] =
        {
            HE_KV(type, type),
            HE_KV(expr, expression),
            HE_MSG(msg),
        };
        he::Log(source, kvs, HE_LENGTH_OF(kvs));

        // TODO: Platform-specific handlers (popup for win32)

        return true;
    }

    void SetErrorHandler(ErrorHandlerFunc handler)
    {
        g_errorHandler = handler;
    }

    bool HandleError(ErrorType type, const char* file, const uint32_t line, const char* funcName, const char* expression, const char* msg)
    {
        if (g_errorHandler)
            return g_errorHandler(type, file, line, funcName, expression, msg);

        return DefaultErrorHandler(type, file, line, funcName, expression, msg);
    }
}

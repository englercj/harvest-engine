// Copyright Chad Engler

#include "he/core/error.h"
#include "he/core/types.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm/lib_core.wasm.h"

namespace he
{
    bool _PlatformErrorHandler(const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        String errorMsg = AsString(source.kind);
        FormatTo(errorMsg, "\n{}", FmtJoin(kvs, kvs + count, "\n"));
        FormatTo(errorMsg, "\nsource.file = {}\nsource.line = {}\nsource.funcName = {}", source.file, source.line, source.funcName);
        heWASM_Alert(errorMsg.Data());
        heWASM_Debugger();
        return true;
    }
}

#endif

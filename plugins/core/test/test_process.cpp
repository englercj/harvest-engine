// Copyright Chad Engler

#include "he/core/process.h"

#include "he/core/result_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, process, GetEnv_SetEnv)
{
    constexpr char VarName[] = "THIS_ENV_VAR_DOESNT_EXIST_I_HOPE";
    constexpr char VarValue[] = "testing";

    const String emptyEnv = GetEnv(VarName);
    HE_EXPECT(emptyEnv.IsEmpty());

    const Result r = SetEnv(VarName, VarValue);
    HE_EXPECT(r, r);

    const String realEnv = GetEnv(VarName);
    HE_EXPECT_EQ(realEnv, VarValue);

    const Result r2 = SetEnv(VarName, nullptr);
    HE_EXPECT(r2, r2);

    const String emptyEnv2 = GetEnv(VarName);
    HE_EXPECT(emptyEnv2.IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, process, GetCurrentProcessId)
{
    const uint32_t pid = GetCurrentProcessId();
    HE_EXPECT_NE(pid, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, process, IsProcessRunning)
{
    const uint32_t pid = GetCurrentProcessId();
    HE_EXPECT(IsProcessRunning(pid));
}

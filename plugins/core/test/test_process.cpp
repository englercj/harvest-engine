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

    String emptyEnv;
    Result r = GetEnv(VarName, emptyEnv);
    HE_EXPECT(!r, r);
    HE_EXPECT(emptyEnv.IsEmpty());

    r = SetEnv(VarName, VarValue);
    HE_EXPECT(r, r);

    String realEnv;
    r = GetEnv(VarName, realEnv);
    HE_EXPECT(r, r);
    HE_EXPECT_EQ(realEnv, VarValue);

    r = SetEnv(VarName, nullptr);
    HE_EXPECT(r, r);

    String emptyEnv2;
    r = GetEnv(VarName, emptyEnv2);
    HE_EXPECT(!r, r);
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

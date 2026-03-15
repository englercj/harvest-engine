---
name: harvest-unit-tests
description: Author, update, explain, build, and run Harvest Engine unit tests that use `he/core/test.h` and the `he_test_runner` executable. Use when Codex needs to add or modify tests under `plugins/*/test`, explain Harvest's `HE_TEST` or `HE_EXPECT*` macros, run or filter the custom test runner, or wire new `*_tests` static libraries into the `engine/tests` group so they are linked into the runner.
---

# Harvest Unit Tests

## Build and run the runner

Regenerate generated projects first if `.build/projects/he_test_runner.vcxproj` is missing or stale:

```powershell
./hemake.sh generate-projects vs2026
```

Build the runner with Visual Studio MSBuild, not `dotnet msbuild`. The generated C++ project imports Visual Studio C++ targets.

Verified Windows command from this repo:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    ".build\projects\he_test_runner.vcxproj" `
    /t:Build `
    /p:Configuration="Debug Win64" `
    /p:Platform=x64 `
    /m:1
```

Expect the binary at:

```text
.build/win64-debug/bin/he_test_runner.exe
```

Run the full suite:

```powershell
& ".build\win64-debug\bin\he_test_runner.exe"
```

Run a subset by substring match against the fully qualified test name `module:suite:test`:

```powershell
& ".build\win64-debug\bin\he_test_runner.exe" --filter "core:toml_writer"
& ".build\win64-debug\bin\he_test_runner.exe" --filter "toml_writer:value_boolean"
```

Show per-test timings:

```powershell
& ".build\win64-debug\bin\he_test_runner.exe" --times
```

The available CLI flags are:

- `-h`, `--help`: print usage text.
- `-t`, `--times`: print per-test timings in milliseconds.
- `-f`, `--filter <value>`: run only tests whose `module:suite:test` name contains the substring.

## Write tests with the framework

Write engine tests in plugin test folders, usually with files named `test_<subject>.cpp`.

Common locations in this repo:

- `plugins/core/test`
- `plugins/math/test`
- `plugins/sqlite/test`
- `plugins/schema/schema/test`
- `plugins/schema/schema_compiler/test`

Follow the repo style guide:

- Name files `test_<header-or-source-name>.cpp`.
- Name tests as `HE_TEST(module, suite, name)` where `module` omits the `he_` prefix and `suite` is usually the file-under-test name.
- Include `"he/core/test.h"` plus the headers under test.

Use `HE_TEST` for simple stateless tests:

```cpp
#include "he/core/test.h"

using namespace he;

HE_TEST(core, string, Append)
{
    String value = "ab";
    value += "cd";
    HE_EXPECT_EQ(value, "abcd");
}
```

Use `HE_TEST_F` when setup, teardown, helpers, or shared state belong in a fixture:

```cpp
class ExampleFixture : public TestFixture
{
public:
    void Before() override
    {
        // Allocate resources or initialize state here.
    }

    void After() override
    {
        // Clean up here.
    }
};

HE_TEST_F(core, example, Works, ExampleFixture)
{
    HE_EXPECT(true);
}
```

Prefer `HE_TEST_F` only when the fixture adds value. Most tests in this repo use plain `HE_TEST`.

## Use the right macros

Use these definition macros:

- `HE_TEST(module, suite, name)`: define a test class derived from `::he::TestFixture`.
- `HE_TEST_F(module, suite, name, fixture)`: define a test class derived from a custom fixture.

Use these expectation macros inside the test body:

- `HE_EXPECT(expr, ...)`: assert a boolean condition and keep running the test. Pass extra values to log failure context.
- `HE_EXPECT_EQ`, `HE_EXPECT_NE`, `HE_EXPECT_LT`, `HE_EXPECT_LE`, `HE_EXPECT_GT`, `HE_EXPECT_GE`: use for comparisons. These forward operands into failure logs.
- `HE_EXPECT_EQ_STR`, `HE_EXPECT_NE_STR`: use for null-terminated string comparisons.
- `HE_EXPECT_EQ_MEM`, `HE_EXPECT_NE_MEM`: use for raw memory comparisons.
- `HE_EXPECT_EQ_PTR`, `HE_EXPECT_NE_PTR`: use for pointer identity checks.
- `HE_EXPECT_EQ_ULP(a, b, diff, ...)`: use for floating-point comparisons in ULPs.
- `HE_EXPECT_ERROR(Kind, ...)`: assert that the wrapped code reports exactly one error of the specified `he::ErrorKind`.
- `HE_EXPECT_ASSERT(...)`: assert that the wrapped code triggers `ErrorKind::Assert`.
- `HE_EXPECT_VERIFY(...)`: assert that the wrapped code triggers `ErrorKind::Verify`.

Keep these caveats in mind:

- `HE_EXPECT*` failures are non-fatal. The test keeps running, and the runner reports the accumulated failure count at the end.
- Several comparison macros may evaluate operands more than once. Do not pass expressions with side effects.
- Use ordinary `HE_ASSERT` and `HE_VERIFY` in engine code under test. Use `HE_EXPECT_ASSERT` and `HE_EXPECT_VERIFY` only in the test that validates those failures.

## Understand how registration and execution work

The framework is macro-based. `HE_TEST` and `HE_TEST_F` expand into a concrete test class with a static inline registration flag. That registration path calls `::he::internal::RegisterTest<T>()`, constructs one static instance of the test class, and pushes its `TestFixture*` into `::he::GetAllTests()`.

`TestFixture` provides this lifecycle:

- `Before()`: run setup.
- `Run()`: default implementation calls `TestBody()`.
- `After()`: run teardown.
- `GetTestInfo()`: return module, suite, name, file, and line metadata generated by the macro.

`RunAllTests()` sorts the registered tests by module, suite, and test name, applies the optional substring filter, calls `Before()`, `Run()`, and `After()`, and records totals for runs, expectations, and failures.

Expectation failures are converted into `he_test` log events by the scoped error handler in `plugins/core/src/test.cpp`. The runner-specific sink in `plugins/core/test_runner/main.cpp` formats those failures and optional timing records for console output.

## Wire test libraries into the runner

Create one static library module per plugin test suite in that plugin's `he_plugin.kdl`. Put it in `group="engine/tests"` and make it depend on the module under test.

Existing examples:

- `plugins/core/he_plugin.kdl`: `he_core_tests`
- `plugins/math/he_plugin.kdl`: `he_math_tests`
- `plugins/sqlite/he_plugin.kdl`: `he_sqlite_tests`
- `plugins/schema/he_plugin.kdl`: `he_schema_tests`, `he_schema_compiler_tests`

Follow this pattern:

```kdl
module he_myplugin_tests kind=lib_static group="engine/tests" {
    files { "test/**" }
    include_dirs { "src" } // only if test code needs private impl headers

    public {
        dependencies { he_myplugin }
    }
}
```

The runner is declared in `plugins/core/he_plugin.kdl` and links every static library in `group="engine/tests"` through a `:foreach module ... kind=lib_static` generator with `whole_archive=#true`.

That `whole_archive` flag matters. Without it, the linker can discard object files whose only effect is static test registration, and the tests never appear in `GetAllTests()`. The generated `.build/projects/he_test_runner.vcxproj` turns that into `/WHOLEARCHIVE:` linker options for each `*_tests.lib`.

When adding a new plugin test library:

1. Add the `*_tests` module to that plugin's `he_plugin.kdl`.
2. Keep it in `group="engine/tests"`.
3. Regenerate projects with `./hemake.sh generate-projects vs2026`.
4. Rebuild `he_test_runner`.

If the group and kind match, the runner picks the test library up automatically. You do not need to hand-edit the runner project.

## Inspect the source of truth when unsure

Use these files as the authoritative reference:

- `plugins/core/include/he/core/test.h`: test macros, fixture base class, counters, and public API.
- `plugins/core/src/test.cpp`: registration, sorting, filtering, error handling, and execution flow.
- `plugins/core/test_runner/main.cpp`: runner CLI, logging, and console formatting.
- `plugins/core/he_plugin.kdl`: `he_core_tests` and `he_test_runner` module definitions.
- `plugins/math/he_plugin.kdl`
- `plugins/sqlite/he_plugin.kdl`
- `plugins/schema/he_plugin.kdl`
- `docs/styleguides/cpp_style_guide.md`: test naming and file naming conventions.
- `docs/he_project_kdl/node_modifiers.md`: explanation of the `:foreach` generator used by the runner.

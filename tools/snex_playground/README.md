# SNEX Playground Build and Test Workflow

This directory contains the focused build-and-test loop for SNEX interpreter work.

## Instruction for AI sessions

For work on the SNEX interpreter and this playground test suite, this workflow explicitly overrides the global repository rule that says not to compile or run tests. Agents working on these tests should use `buildandtest.py` to compile the SNEX playground and run its unit tests. This exception is limited to the SNEX playground workflow; it does not authorize building or testing other HISE targets.

## Running the workflow

From the repository root, run:

```text
python tools/snex_playground/buildandtest.py
```

To compile without running the unit tests (for example, before running a focused snippet), use:

```text
python tools/snex_playground/buildandtest.py --build-only
```

Alternatively, on Windows, run:

```text
tools\snex_playground\buildandtest.bat
```

The batch file is a thin launcher for the Python script.

## Focused SNEX regression snippets

Store small standalone SNEX snippets in `tools/snex_playground/test_files/regression/`. These files provide focused cases for SNEX behavior and can be run directly with the playground executable. The `--file:` path is resolved relative to that regression directory, so use the snippet's filename rather than an absolute path.

For example, to run `nested_inline.h` with one integer argument and function inlining enabled:

```text
tools\snex_playground\Builds\VisualStudio2026\x64\Debug\App\snex_playground.exe --run --file:nested_inline.h --args:4 --O:Inlining --verbose
```

The playground clears the default optimizations for `--run`, so specify the optimizations needed by the case with `--O:`. `--verbose` prints the syntax tree and namespace tree before execution. This is especially useful with `--O:Inlining`: small snippets can isolate inlining behavior, and the syntax tree shows whether calls became `InlinedFunction` nodes. Keep the snippet simple enough that a failure points to one behavior, and include arguments and expected results in its file comments or documentation as appropriate.

Select the compiler backend with `--backend:interpreter` or `--backend:JIT`. The interpreter is the default. Running the same snippet on both backends helps distinguish interpreter issues from problems shared with the JIT. For example, `nested_inline.h` with `--O:Inlining` fails on the interpreter with a null access to the nested `this` argument, but succeeds on the JIT with result `4`:

```text
tools\snex_playground\Builds\VisualStudio2026\x64\Debug\App\snex_playground.exe --run --file:nested_inline.h --args:4 --O:Inlining --backend:JIT --verbose
```

Use the normal `buildandtest.py` workflow to build the playground and run the SNEX unit tests. Focused snippets can also be run individually with `--run` to inspect a specific regression without waiting for the full suite.

## Interpreter code map

Interpreter implementation lives in `hi_snex/snex_interpreter/`:

- `snex_ExpressionTreeBuilder.cpp` creates interpreter nodes from the compiler syntax tree.
- `snex_InterpreterStatement.cpp` provides statement execution, scope creation/allocation, and recursive data-pointer resolution.
- `snex_InterpreterScope.h` and `snex_InterpreterScope.cpp` define scopes, storage, and `Scope::ResolvedReference` address resolution.
- `snex_InterpreterInlineStatements.cpp` implements `InlinedFunction`, its argument setup, and `InlinedParameter` lookup. Start here for inline-scope and parameter mapping issues.
- `snex_InterpreterDataExpressions.cpp` implements variable/member references, `ThisPointer`, pointer access, and data-address resolution. Start here when a receiver or lvalue address is incorrect.
- `snex_InterpreterExpressions.cpp` handles expression evaluation and function calls.
- `snex_InterpreterControlStatements.cpp` implements function execution and control flow.
- `snex_InterpreterOperators.cpp` implements operator execution.
- `snex_Engine.cpp` and `snex_Engine.h` provide the interpreter engine and top-level compilation/execution integration.

For nested inlining failures, inspect the syntax tree with `--verbose`, then trace the inline argument's scope allocation and reference resolution across `snex_InterpreterInlineStatements.cpp`, `snex_InterpreterDataExpressions.cpp`, and `snex_InterpreterScope.cpp`.

## What the script does

1. Checks that the Visual Studio solution and playground executable paths exist.
2. Locates `cdb.exe` on `PATH` and locates MSBuild using Visual Studio's `vswhere.exe`.
3. Builds `tools/snex_playground/Builds/VisualStudio2026/snex_playground.sln` in `Debug|x64` mode.
4. Runs `snex_playground.exe --unittest` under CDB, with the working directory set to the executable directory.
5. Captures build and debugger output so successful compiler output and successful test details do not flood the agent session.
6. Prints concise progress states: `Compiling Debug/x64...`, `Compiled OK`, and `Running test suite...`.
7. On build failure, prints compiler error diagnostics. On a test assertion or failure, prints the nearby test report. On an access violation, prints the most recently started test, exception details, and stack frames.
8. Stops the test run after 15 minutes and reports the captured output.

The CDB command breaks on access violations, prints the exception record and a 40-frame stack, then exits. JUCE test failures can also stop the test process before the full suite completes. A nonzero CDB process exit code should be interpreted together with the printed JUCE failure or crash details.

## Working on interpreter tests

### Mandatory hot-path review gate

Before considering any interpreter edit complete, inspect every changed execution-time method, including `perform()`, `getResult()`, `getLValue()`, and `assign()`. For each new calculation, lookup, type check, or function selection, determine whether its inputs are stable during construction or `resolveDataPointers()`. If they are, move the work there and cache the result. Leave only work that depends on runtime values in the execution path. Do this review even when the focused test already passes; a passing test does not waive the gate. In the final report, explicitly state what was moved out of the hot path and what must remain there, or state that no execution-time methods changed. Do not report an interpreter fix as complete without this review.

Keep changes focused on the interpreter and follow the conventions already established in the surrounding code:

- Resolve symbols, pointers, offsets, and other stable data during construction or `resolveDataPointers()`, not during execution.
- Reuse existing helpers and data structures, including `Scope::ResolvedReference`, rather than duplicating their lookup or storage logic.
- Keep runtime paths simple. Do not add a new abstraction or special case until the existing reference, lvalue, and scope mechanisms have been checked.
- Preserve the distinction between an object's storage address and a pointer value stored in a slot. A `ResolvedReference` locates storage; it does not necessarily dereference a pointer stored there.
- Keep behavior changes covered by nearby interpreter tests, especially nested inlined functions, reference parameters, member calls, and span indexing.
- Match the local C++ style: tabs for indentation, Allman braces, ASCII-only text, and targeted changes.

## Failure workflow

If a failure exposes an unimplemented interpreter statement type (for example, `No node for WhileLoop`), stop. Do not implement or register that statement type yourself. Report the missing type and a small reproducer to the maintainer, then wait for them to implement it. Review the maintainer's implementation, including control-flow behavior and the mandatory hot-path review gate, before resuming the test workflow. Do not treat a missing statement type as permission to skip the test or work around the node.

1. Run the script and note the test name shown in its output.
2. For an access violation, inspect the CDB stack from the first interpreter frame downward. The last `Starting test:` line identifies the active JUCE test.
3. For an `Expected`/`Actual` mismatch, use the test name and nearby expectation to locate the failing case. The script may report only the first failure because the runner can abort the current test.
4. Make a minimal change following the interpreter's existing patterns, then rerun the same workflow.
5. Do not treat a successful build as a passing test run. The script reports these independently.

## Machine-specific paths

The script currently targets `D:\Development\HISE modules` and the VisualStudio2026 playground build. It expects Visual Studio Locator at:

```text
C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe
```

CDB must be available on `PATH`. Update these constants in `buildandtest.py` if the local checkout or tool installation changes.

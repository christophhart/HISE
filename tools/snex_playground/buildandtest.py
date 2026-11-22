import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(r"D:\Development\HISE modules")
PLAYGROUND_DIR = ROOT / "tools" / "snex_playground"
SOLUTION = PLAYGROUND_DIR / "Builds" / "VisualStudio2026" / "snex_playground.sln"
EXE = PLAYGROUND_DIR / "Builds" / "VisualStudio2026" / "x64" / "Debug" / "App" / "snex_playground.exe"
VSWHERE = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")


def find_msbuild():
    if not VSWHERE.is_file():
        raise RuntimeError(f"Visual Studio Locator not found: {VSWHERE}")

    result = subprocess.run(
        [str(VSWHERE), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild", "-property", "installationPath"],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.returncode != 0 or not result.stdout.strip():
        raise RuntimeError("vswhere could not locate a Visual Studio installation with MSBuild")

    msbuild = Path(result.stdout.strip()) / "MSBuild" / "Current" / "Bin" / "MSBuild.exe"
    if not msbuild.is_file():
        raise RuntimeError(f"MSBuild not found: {msbuild}")
    return msbuild


def report_build_failure(output):
    lines = output.splitlines()
    diagnostic = re.compile(r"\berror\b|: error", re.IGNORECASE)
    indices = [i for i, line in enumerate(lines) if diagnostic.search(line)]
    if indices:
        selected = set()
        for i in indices:
            selected.update(range(max(0, i - 1), min(len(lines), i + 2)))
        for i in sorted(selected):
            print(lines[i])
    else:
        print("Build failed. Last build output:")
        print("\n".join(lines[-30:]))


def report_test_failure(output):
    lines = output.splitlines()
    crash_lines = [i for i, line in enumerate(lines) if "Access violation" in line]
    if crash_lines:
        last_crash = crash_lines[-1]
        starting_tests = [
            line for line in lines[:last_crash]
            if "Starting test:" in line
        ]
        if starting_tests:
            print(starting_tests[-1].strip())

        print("Access violation and exception details:")
        start = next(
            (i for i in range(last_crash, len(lines)) if "ExceptionAddress:" in lines[i]),
            last_crash,
        )
        for line in lines[start:]:
            if line.strip() == "quit:":
                break
            if re.search(r"ExceptionAddress:|ExceptionCode:|Parameter\[|Attempt to read|RetAddr|snex_playground!|KERNEL32!|ntdll!", line):
                print(line.rstrip())
        return

    failure = re.compile(r"Expected:|Actual:|FAILED|!!! Test|jassert|Assertion|test.*failed", re.IGNORECASE)
    indices = [i for i, line in enumerate(lines) if failure.search(line)]
    if indices:
        last_failure = indices[0]
        starting_tests = [line for line in lines[:last_failure] if "Starting test:" in line]
        if starting_tests:
            print(starting_tests[-1].strip())

        selected = set()
        for i in indices:
            selected.update(range(max(0, i - 3), min(len(lines), i + 4)))
        for i in sorted(selected):
            print(lines[i])
        return

    print("cdb did not produce a recognized test failure or crash report.")
    print("Last debugger output:")
    print("\n".join(lines[-40:]))


def main():
    if not SOLUTION.is_file():
        print(f"Solution not found: {SOLUTION}", file=sys.stderr)
        return 2
    if not EXE.is_file():
        print(f"Playground executable not found: {EXE}", file=sys.stderr)
        return 2

    cdb = shutil.which("cdb.exe") or shutil.which("cdb")
    if not cdb:
        print("cdb was not found on PATH", file=sys.stderr)
        return 2

    try:
        msbuild = find_msbuild()
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    print("Compiling Debug/x64...", flush=True)
    build = subprocess.run(
        [str(msbuild), str(SOLUTION), "/m", "/nologo", "/v:q", "/p:Configuration=Debug", "/p:Platform=x64"],
        cwd=str(PLAYGROUND_DIR),
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if build.returncode != 0:
        print("Debug build failed:")
        report_build_failure(build.stdout + "\n" + build.stderr)
        return build.returncode

    print("Compiled OK", flush=True)
    print("Running test suite...", flush=True)
    try:
        test = subprocess.run(
            [
                cdb,
                "-c",
                'sxe -c ".exr -1; .ecxr; kb 40; q" av; g',
                str(EXE),
                "--unittest",
            ],
            cwd=str(EXE.parent),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=900,
        )
    except subprocess.TimeoutExpired as exc:
        print("SNEX unit tests exceeded the 15-minute timeout.")
        partial_output = exc.stdout or ""
        if isinstance(partial_output, bytes):
            partial_output = partial_output.decode("utf-8", errors="replace")
        report_test_failure(partial_output)
        return 124

    output = test.stdout + "\n" + test.stderr
    failure = re.search(
        r"Access violation|ExceptionCode:\s*c0000005|Expected:|Actual:|FAILED!!|!!! Test",
        output,
        re.IGNORECASE,
    )

    if test.returncode != 0 or failure:
        print(f"SNEX unit tests failed under cdb (debugger exit code {test.returncode}):")
        report_test_failure(output)
        return test.returncode or 1

    print("Debug build succeeded. SNEX unit tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

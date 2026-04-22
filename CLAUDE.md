# AGENTS.md - HISE Modules Repository

## ABSOLUTE RULES

**NO EM-DASHES. NO SMART QUOTES. NO NON-ASCII CHARACTERS. EVER.** All text output - code, comments, documentation, strings, commit messages - must be ASCII-only. Use regular dashes (-), straight quotes, and three dots (...). This rule is non-negotiable and applies to every file in this repository without exception.

## Project Overview

HISE (Hart Instruments Sampling Engine) is a cross-platform C++17 audio framework built on JUCE for creating virtual instruments (VST/AU/AAX plugins and standalone apps). This repository contains the core modules layer (`hi_core`, `hi_backend`, `hi_scripting`, `hi_dsp_library`, etc.).

## Build System

HISE uses **Projucer** (JUCE's project tool) to generate IDE projects from `.jucer` files. There is no CMake build at the project level.

**IMPORTANT: Never attempt to build HISE yourself. The maintainer (Christoph) always builds. Never touch git either -- Christoph handles all commits.**

### Build Configurations

| Config | Purpose | Key Defines |
|--------|---------|-------------|
| Debug | Development | Standard debug |
| Release | Production (with LTO) | Optimized |
| CI | CI builds + unit tests | `HI_RUN_UNIT_TESTS=1`, `HISE_CI=1` |
| Minimal Build | Debug without optional modules | No Faust/Loris/rLottie/RTNeural |

Builds are done manually by the maintainer using Projucer-generated IDE projects (Visual Studio / Xcode). Unit tests are run by the maintainer in the debugger after each feature iteration. Never attempt to build or run tests yourself.

## Build Configurations (Backend vs Frontend vs DLL)

HISE has three build targets with mutually exclusive preprocessor configurations:

| Build Target | `USE_BACKEND` | `USE_FRONTEND` | `HI_EXPORT_AS_PROJECT_DLL` |
|--------------|---------------|----------------|---------------------------|
| **HISE IDE** | 1 | 0 | 0 |
| **Exported Plugin** | 0 | 1 | 0 |
| **Project DLL** | 0 | 0 | 1 |

- **Backend** (`BackendProcessor`, `hi_backend` module): Full IDE with editors, console, export tools, REST API
- **Frontend** (`FrontendProcessor`, `hi_frontend` module): Exported plugins with embedded presets, minimal footprint
- **Project DLL**: Custom C++ DSP code loaded by the HISE IDE at runtime; at export time this code is baked directly into the plugin binary

IDE-specific code (editors, debug output, development tools) must be guarded with `#if USE_BACKEND` to avoid binary bloat and runtime overhead in exported plugins. See `guidelines/development/build-configurations.md` for full details.

## Code Style

### Language & Framework

- **C++17** with JUCE framework
- **Unity builds**: Each module has a top-level header (e.g., `hi_core.h`) and one or more `.cpp` files that `#include` all source files. Larger modules use multiple translation units (e.g., `hi_scripting_02.cpp`) to avoid compiler out-of-memory issues, or to isolate third-party code that is incompatible with unity builds (e.g., `hi_loris_12.cpp`). Do NOT add individual `.cpp` files to build systems -- add `#include` directives in the module's unity `.cpp` file.
- All code lives in the `hise` namespace with `using namespace juce;`

### Naming Conventions

| Element | Convention | Examples |
|---------|-----------|----------|
| Classes/Structs | PascalCase | `MainController`, `RestServer` |
| Methods/Functions | camelCase | `setupUIUpdater()`, `handleRequest()` |
| Member variables | camelCase, no prefix | `sampleRate`, `blockSize` |
| Constants | kPascalCase | `kSampleRate`, `kBlockSize` |
| Macros/Defines | ALL_CAPS | `HI_RUN_UNIT_TESTS`, `SIGNAL_COLOUR` |
| Template params | Single uppercase or PascalCase | `T`, `NV`, `NumVoices` |
| Identifiers (JUCE) | `DECLARE_ID(camelCase)` | `DECLARE_ID(moduleId)` |

### Formatting

Default JUCE coding style with HISE-specific modifications:
- **Tabs** for indentation (not spaces)
- **Allman/break brace style** (opening brace on its own line)
- **120-character** max line length
- No member variable prefixes (`m_`, `_`, etc.)

### Includes

- Never put test code in headers (static registration causes duplicate test runs)
- Test files are `.cpp` included inside `#if HI_RUN_UNIT_TESTS` guards in unity build files
- Module headers include other module headers and all internal headers

### Error Handling & Memory

- `jassert(condition)` for debug assertions; `jassertfalse` for unconditional debug assertion
- `req->fail(statusCode, "msg")` for REST API errors
- `if (auto x = ...)` for RAII lock/guard patterns with `operator bool()`
- `std::unique_ptr<T>` or `ScopedPointer<T>` for owned objects
- `ReferenceCountedObjectPtr<T>` for JUCE reference-counted objects
- Always add `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)` to component classes

### Threading

HISE uses a lock-free audio architecture. The `MainController::KillStateHandler` coordinates safe cross-thread operations by suspending audio processing (killing voices, outputting silence) rather than blocking. Use `killVoicesAndCall()` for any operation that modifies audio-thread-accessible data. Never block on the audio thread -- no locks, no allocations, no I/O. See `guidelines/development/threading-system.md` for full details including lock priority, `SimpleReadWriteLock`, and `AudioThreadGuard`.

### Comments & Documentation

```cpp
/** Doxygen-style for classes and public methods */
// Inline comments for implementation details
```

Test documentation uses a three-part format:
```cpp
/** Setup: [test configuration]
 *  Scenario: [what happens]
 *  Expected: [verified behavior]
 */
```

### UI Code Conventions

- Fonts: `GLOBAL_FONT()`, `GLOBAL_BOLD_FONT()`
- Colors: `Colour(SIGNAL_COLOUR)` (accent), `HISE_OK_COLOUR`, `HISE_WARNING_COLOUR`, `HISE_ERROR_COLOUR`
- Background: `Colour(0xFF333333)`, Text: `Colours::white.withAlpha(0.8f)`
- Standard padding: 4px; between elements: 8px
- Use `GlobalHiseLookAndFeel` for styling; subclass it for custom overrides

## Architecture Guidelines

1. **Research before acting** -- understand existing code and the architectural "why" before proposing changes
2. **Extend existing patterns** rather than inventing new abstractions
3. **Don't over-engineer** -- start with minimal, targeted fixes
4. **Separate logic verification from implementation** -- understand, verify design, then implement
5. **Verify changes end-to-end** -- watch for unnecessary casts, workarounds, pattern violations
6. **Comment deviations** -- explain why, when, and safety when deviating from patterns
7. **Challenge proposals** -- don't assume suggestions are optimal; voice concerns explicitly

## Key Directories

| Directory | Purpose |
|-----------|---------|
| `hi_core/` | Core: MainController, DSP base, event buffers |
| `hi_backend/` | Backend IDE, REST API server, AI tools (`USE_BACKEND` only) |
| `hi_scripting/` | HISEScript engine, ScriptNode, JavaScript API |
| `hi_dsp_library/` | DSP nodes, filters, effects, unit tests |
| `hi_streaming/` | Audio streaming, time stretching |
| `hi_tools/` | Utilities: markdown, CSS, binary data |
| `hi_frontend/` | Exported plugin runtime (`USE_FRONTEND` only) |
| `hi_snex/` | SNEX JIT compiler |
| `guidelines/` | AI-focused development docs (style, API, architecture) |
| `tools/auto_build/` | CI scripts, build automation |

## REST API Development

When working on the REST API (`hi_backend/backend/ai_tools/`):

1. Add route to `ApiRoute` enum in `RestHelpers.h`
2. Add metadata in `RestHelpers.cpp` `getRouteMetadata()` (in enum order)
3. Declare handler in `RestHelpers.h`, implement in `RestHelpers.cpp`
4. Add switch case in `BackendProcessor.cpp` `onAsyncRequest()`
5. Add unit test in `ServerUnitTests.cpp` (auto-verified by `verifyAllEndpointsTested()`)

## Further Reading

Detailed guidelines in `guidelines/`: `development/build-configurations.md` (backend/frontend/DLL architecture), `development/threading-system.md` (threading system), `development/cpp-architecture.md` (architecture), `development/unit-testing.md` (test patterns), `development/voice-setter-system.md` (UIUpdater/voice protection), `style/cpp-ui.md` (UI conventions), `style/hisescript.md` (HISEScript quirks), `api/rest-api.md` (full REST API spec), `api/rest-api-development.md` (adding endpoints).

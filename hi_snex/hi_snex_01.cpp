#define ASMJIT_STATIC 1
#define ASMJIT_EMBED 1

#include "hi_snex.h"

#if HISE_INCLUDE_SNEX

#if HISE_INCLUDE_SNEX_X64_CODEGEN || SNEX_MIR_BACKEND

#if SNEX_ASMJIT_BACKEND
#include "src/asmjit/asmjit.h"
#endif

#include "asmjit_definitions.h"

#endif

#include "snex_parser/snex_jit_TokenIterator.h"
#include "snex_parser/snex_jit_PreProcessor.h"

#include "snex_core/snex_jit_ComplexTypeLibrary.h"
#include "snex_library/snex_jit_ApiClasses.h"
#include "snex_core/snex_jit_ClassScope.h"
#include "snex_jit/snex_jit_AssemblyRegister.h"
#include "snex_core/snex_jit_FunctionScope.h"
#include "snex_core/snex_jit_BaseCompiler.h"

#include "unit_test/snex_jit_UnitTestCase.cpp"
#include "unit_test/snex_jit_IndexTest.cpp"
#include "unit_test/snex_jit_UnitTests.cpp"
#include "api/SnexApi.cpp"



#include "snex_components/snex_DebugTools.cpp"
#include "snex_components/snex_WorkbenchData.cpp"
#include "snex_components/snex_ExtraComponents.cpp"
#include "snex_components/snex_JitPlayground.cpp"

#endif

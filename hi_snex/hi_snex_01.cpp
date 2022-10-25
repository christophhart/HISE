#define ASMJIT_STATIC 1
#define ASMJIT_EMBED 1

#include "hi_snex.h"

#if HISE_INCLUDE_SNEX

#if HISE_INCLUDE_SNEX_X64_CODEGEN




#include "src/asmjit/asmjit.h"



namespace asmjit
{
using X86Gp = x86::Gp;
using X86Reg = x86::Reg;
using X86Mem = x86::Mem;
using X86Xmm = x86::Xmm;
using X86Gpq = x86::Gpq;
using X86Compiler = x86::Compiler;
using Runtime = JitRuntime;
using FuncSignatureX = FuncSignatureBuilder;
using CodeEmitter = x86::Emitter;
}

#endif

using String = juce::String;


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

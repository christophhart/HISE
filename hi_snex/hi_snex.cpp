#include "hi_snex.h"

#if HISE_INCLUDE_SNEX

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

using String = juce::String;

#include "snex_jit/snex_jit_TokenIterator.h"

#include "snex_jit/snex_jit_ComplexTypeLibrary.h"
#include "snex_jit/snex_jit_ApiClasses.h"
#include "snex_jit/snex_jit_ClassScope.h"
#include "snex_jit/snex_jit_AssemblyRegister.h"
#include "snex_jit/snex_jit_FunctionScope.h"
#include "snex_jit/snex_jit_BaseCompiler.h"
#include "snex_jit/snex_jit_OperationsBase.h"
#include "snex_jit/snex_jit_Parser.h"
#include "snex_jit/snex_jit_CodeGenerator.h"
#include "snex_jit/snex_jit_OptimizationPasses.h"
#include "snex_jit/snex_jit_SyntaxTreeWalker.h"
#include "snex_jit/snex_jit_Operations.h"
#include "snex_jit/snex_jit_FunctionParser.h"

#include "snex_core/snex_Types.cpp"
#include "snex_core/snex_TypeHelpers.cpp"
#include "snex_core/snex_DynamicType.cpp"


#include "snex_jit/snex_jit_Functions.cpp"
#include "snex_jit/snex_jit_NamespaceHandler.cpp"
#include "snex_jit/snex_jit_ComplexTypeLibrary.cpp"
#include "snex_jit/snex_jit_ApiClasses.cpp"
#include "snex_jit/snex_jit_AssemblyRegister.cpp"
#include "snex_jit/snex_jit_FunctionScope.cpp"
#include "snex_jit/snex_jit_ClassScope.cpp"
#include "snex_jit/snex_jit_BaseCompiler.cpp"


#include "snex_jit/snex_jit_Parser.cpp"
#include "snex_jit/snex_jit_OperationsBase.cpp"
#include "snex_jit/snex_jit_BaseScope.cpp"
#include "snex_jit/snex_jit_GlobalScope.cpp"
#include "snex_jit/snex_jit_JitCallableObject.cpp"

#include "snex_jit/snex_jit_OptimizationPasses.cpp"
#include "snex_jit/snex_jit_CodeGenerator.cpp"
#include "snex_jit/snex_jit_SyntaxTreeWalker.cpp"

#include "snex_jit/snex_jit_Operations.cpp"
#include "snex_jit/snex_jit_FunctionParser.cpp"

#include "snex_jit/snex_jit_JitCompiledFunctionClass.cpp"
#include "snex_jit/snex_jit_JitCompiler.cpp"

#include "snex_jit/snex_jit_UnitTests.cpp"

#include "snex_core/snex_CallbackCollection.cpp"
#include "snex_components/snex_JitPlayground.cpp"
#endif

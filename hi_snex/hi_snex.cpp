#include "hi_snex.h"

#include "src/asmjit/asmjit.h"

#include "snex_jit/snex_jit_TokenIterator.h"

#include "snex_jit/snex_jit_ApiClasses.h"
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
#include "snex_jit/snex_jit_ApiClasses.cpp"
#include "snex_jit/snex_jit_AssemblyRegister.cpp"
#include "snex_jit/snex_jit_FunctionScope.cpp"
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
#include "snex_components/snex_JitPlayground.cpp"



#include "hi_snex.h"

#include "snex_core/snex_Types.cpp"
#include "snex_core/snex_TypeHelpers.cpp"
#include "snex_core/snex_DynamicType.cpp"

#include "src/asmjit/asmjit.h"

#define INCLUDE_BUFFERS 0
#define INCLUDE_GLOBALS 1
#define INCLUDE_CONDITIONALS 1
#define INCLUDE_FUNCTION_CALLS 1

#include "snex_jit/TokenIterator.h"

#include "snex_jit/GlobalBase.h"
#include "snex_jit/FunctionInfo.h"
#include "snex_jit/JitFunctions.cpp"

#include "snex_jit/JitScope.h"

#include "snex_jit/AssemblyRegister.h"
#include "snex_jit/BaseCompiler.h"
#include "snex_jit/snex_jit_Parser.h"
#include "snex_jit/CompilerPassCodeGeneration.h"
#include "snex_jit/CompilerPassRegisterAllocation.h"
#include "snex_jit/NewFunctionParser.h"

#include "snex_jit/AssemblyRegister.cpp"
#include "snex_jit/BaseCompiler.cpp"
#include "snex_jit/NewStatements.cpp"

#include "snex_jit/snex_jit_BaseScope.cpp"
#include "snex_jit/snex_jit_GlobalScope.cpp"
#include "snex_jit/snex_jit_JitCallableObject.cpp"

#include "snex_jit/CompilerPassParsing.h"
#include "snex_jit/CompilerPassParsing.cpp"
#include "snex_jit/CompilerPassSymbolResolving.cpp"
#include "snex_jit/CompilerPassOptimization.cpp"
#include "snex_jit/NewAssemblyHelpers.cpp"
#include "snex_jit/CompilerPassRegisterAllocation.cpp"
#include "snex_jit/CompilerPassCodeGeneration.cpp"
#include "snex_jit/NewExpressionParser.cpp"
#include "snex_jit/snex_jit_Parser.cpp"

#include "snex_jit/snex_jit_JitCompiledFunctionClass.cpp"
#include "snex_jit/snex_jit_JitCompiler.cpp"

#include "snex_jit/NewFunctionParser.cpp"

#include "snex_jit/JitUnitTests.cpp"

#include "snex_components/snex_JitPlayground.cpp"



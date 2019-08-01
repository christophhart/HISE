#include "hnode_jit.h"

#include "hnode_core/hnode_Types.cpp"
#include "hnode_core/hnode_TypeHelpers.cpp"
#include "hnode_core/hnode_DynamicType.cpp"

#include "src/asmjit/asmjit.h"

#define INCLUDE_BUFFERS 0
#define INCLUDE_GLOBALS 1
#define INCLUDE_CONDITIONALS 1
#define INCLUDE_FUNCTION_CALLS 1

#include "hnode_jit/TokenIterator.h"

#include "hnode_jit/GlobalBase.h"
#include "hnode_jit/FunctionInfo.h"
#include "hnode_jit/JitFunctions.cpp"

#include "hnode_jit/JitScope.h"

#include "hnode_jit/AssemblyRegister.h"
#include "hnode_jit/BaseCompiler.h"
#include "hnode_jit/hnode_jit_Parser.h"
#include "hnode_jit/CompilerPassCodeGeneration.h"
#include "hnode_jit/CompilerPassRegisterAllocation.h"
#include "hnode_jit/NewFunctionParser.h"

#include "hnode_jit/AssemblyRegister.cpp"
#include "hnode_jit/BaseCompiler.cpp"
#include "hnode_jit/NewStatements.cpp"

#include "hnode_jit/hnode_jit_BaseScope.cpp"
#include "hnode_jit/hnode_jit_GlobalScope.cpp"
#include "hnode_jit/hnode_jit_JitCallableObject.cpp"

#include "hnode_jit/CompilerPassParsing.h"
#include "hnode_jit/CompilerPassParsing.cpp"
#include "hnode_jit/CompilerPassSymbolResolving.cpp"
#include "hnode_jit/CompilerPassOptimization.cpp"
#include "hnode_jit/NewAssemblyHelpers.cpp"
#include "hnode_jit/CompilerPassRegisterAllocation.cpp"
#include "hnode_jit/CompilerPassCodeGeneration.cpp"
#include "hnode_jit/NewExpressionParser.cpp"
#include "hnode_jit/hnode_jit_Parser.cpp"

#include "hnode_jit/hnode_jit_JitCompiledFunctionClass.cpp"
#include "hnode_jit/hnode_jit_JitCompiler.cpp"

#include "hnode_jit/NewFunctionParser.cpp"

#include "hnode_jit/JitUnitTests.cpp"

#include "jit_components/hnode_JitPlayground.cpp"



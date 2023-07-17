
#define ASMJIT_STATIC 1
#define ASMJIT_EMBED 1

#include "hi_snex.h"

#if JUCE_MAC
#pragma clang diagnostic ignored "-Weverything"
#endif

#if HISE_INCLUDE_SNEX

#if HISE_INCLUDE_SNEX_X64_CODEGEN || SNEX_MIR_BACKEND

#if SNEX_ASMJIT_BACKEND
#include "src/asmjit/asmjit.h"
#endif

#include "asmjit_definitions.h"

#endif

using String = juce::String;

#include "snex_parser/snex_jit_TokenIterator.h"

#include "snex_core/snex_jit_ComplexTypeLibrary.h"
#include "snex_library/snex_jit_ApiClasses.h"
#include "snex_core/snex_jit_ClassScope.h"
#include "snex_jit/snex_jit_AssemblyRegister.h"
#include "snex_core/snex_jit_FunctionScope.h"
#include "snex_core/snex_jit_BaseCompiler.h"
#include "snex_jit/snex_jit_OperationsBase.h"

#include "snex_parser/snex_jit_SymbolParser.h"
#include "snex_parser/snex_jit_TypeParser.h"
#include "snex_parser/snex_jit_BlockParser.h"
#include "snex_parser/snex_jit_ClassParser.h"

#include "snex_jit/snex_jit_CodeGenerator.h"
#include "snex_jit/snex_jit_OptimizationPasses.h"
#include "snex_jit/snex_jit_SyntaxTreeWalker.h"

#include "snex_jit/snex_jit_OperationsSymbols.h"
#include "snex_jit/snex_jit_OperationsFunction.h"
#include "snex_jit/snex_jit_OperationsOperators.h"
#include "snex_jit/snex_jit_OperationsBranching.h"

#include "snex_jit/snex_jit_OperationsObjects.h"
#include "snex_jit/snex_jit_OperationsTemplates.h"
#include "snex_parser/snex_jit_FunctionParser.h"
#include "snex_jit/snex_jit_TemplateClassBuilder.h"
#include "snex_library/snex_jit_ContainerTypeLibrary.h"
#include "snex_library/snex_jit_ParameterTypeLibrary.h"
#include "snex_library/snex_jit_WrapperTypeLibrary.h"
#include "snex_library/snex_jit_NodeLibrary.h"
#include "snex_library/snex_jit_IndexLibrary.h"

#include "snex_core/snex_jit_TypeInfo.cpp"
#include "snex_core/snex_jit_TemplateParameter.cpp"
#include "snex_core/snex_jit_Inliner.cpp"
#include "snex_public/snex_jit_FunctionData.cpp"
#include "snex_core/snex_jit_ComplexType.cpp" // SyntaxTreeInlineData is defined in FunctionData.cpp
#include "snex_core/snex_jit_FunctionClass.cpp"

#include "snex_parser/snex_jit_PreProcessor.cpp"

#include "snex_cpp_builder/snex_jit_CppBuilder.cpp"
#include "snex_cpp_builder/snex_jit_ValueTreeBuilder.cpp"

#include "snex_parser/snex_jit_SymbolParser.cpp"
#include "snex_parser/snex_jit_TypeParser.cpp"
#include "snex_parser/snex_jit_BlockParser.cpp"
#include "snex_parser/snex_jit_ClassParser.cpp"

#include "snex_library/snex_jit_NativeDspFunctions.cpp"
#include "snex_core/snex_jit_NamespaceHandler.cpp"

#include "snex_core/snex_jit_ComplexTypeLibrary.cpp"
#include "snex_library/snex_jit_ApiClasses.cpp"
#include "snex_jit/snex_jit_AssemblyRegister.cpp"
#include "snex_core/snex_jit_FunctionScope.cpp"
#include "snex_core/snex_jit_ClassScope.cpp"
#include "snex_core/snex_jit_BaseCompiler.cpp"


#include "snex_jit/snex_jit_OperationsBase.cpp"
#include "snex_core/snex_jit_BaseScope.cpp"
#include "snex_public/snex_jit_GlobalScope.cpp"
#include "snex_core/snex_jit_JitCallableObject.cpp"

#include "snex_jit/snex_jit_OptimizationPasses.cpp"
#include "snex_jit/snex_jit_CodeGenerator.cpp"
#include "snex_jit/snex_jit_SyntaxTreeWalker.cpp"

#include "snex_jit/snex_jit_OperationsBranching.cpp"
#include "snex_jit/snex_jit_OperationsSymbols.cpp"
#include "snex_jit/snex_jit_OperationsFunction.cpp"
#include "snex_jit/snex_jit_OperationsOperators.cpp"
#include "snex_jit/snex_jit_OperationsObjects.cpp"
#include "snex_jit/snex_jit_OperationsTemplates.cpp"
#include "snex_parser/snex_jit_FunctionParser.cpp"

#include "snex_core/snex_jit_JitCompiledFunctionClass.cpp"
#include "snex_public/snex_jit_JitCompiler.cpp"

#include "snex_jit/snex_jit_TemplateClassBuilder.cpp"
#include "snex_library/snex_CallbackCollection.cpp"

#include "snex_library/snex_jit_ExternalComplexTypeLibrary.cpp"
#include "snex_library/snex_jit_ContainerTypeLibrary.cpp"
#include "snex_library/snex_jit_ParameterTypeLibrary.cpp"
#include "snex_library/snex_jit_WrapperTypeLibrary.cpp"
#include "snex_library/snex_jit_NodeLibrary.cpp"
#include "snex_library/snex_jit_IndexLibrary.cpp"
#include "snex_library/snex_ExternalObjects.cpp"

#include "snex_public/snex_jit_JitCompiledNode.cpp"




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










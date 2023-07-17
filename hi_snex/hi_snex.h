/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_snex
  vendor:           HISE
  version:          1.0
  name:             Snex JIT compiler
  description:      A (more or less) C compiler based on ASMJit
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:     juce_core
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

***************************************************************************** */

#pragma once


#define REMOVE_REUSABLE_REG 0




#include "AppConfig.h"



/** Config: SNEX_ENABLE_SIMD

Enables SIMD processing for consecutive float spans (not functional yet). 
*/
#ifndef SNEX_ENABLE_SIMD
#define SNEX_ENABLE_SIMD 0
#endif



#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"



/** Config: HISE_INCLUDE_SNEX
 
Set to 0 to disable SNEX compilation (default on iOS).
*/
#ifndef HISE_INCLUDE_SNEX
#if defined (__arm__) || defined (__arm64__)
#define HISE_INCLUDE_SNEX 0
#else
#if USE_BACKEND
#define HISE_INCLUDE_SNEX 1
#else
#define HISE_INCLUDE_SNEX 0
#endif
#endif
#endif

/** Config: SNEX_STANDALONE_PLAYGROUND
 
 Enables the playground.
*/
#ifndef SNEX_STANDALONE_PLAYGROUND
#define SNEX_STANDALONE_PLAYGROUND 0
#endif


#ifndef SNEX_MIR_BACKEND
#define SNEX_MIR_BACKEND 1
#endif

/** The SNEX compiler is only available on x64 builds so this preprocessor will allow compiling HISE on ARM withouth the JIT compiler. */
#ifndef HISE_INCLUDE_SNEX_X64_CODEGEN
#if JUCE_ARM
#define HISE_INCLUDE_SNEX_X64_CODEGEN 0
#else
#define HISE_INCLUDE_SNEX_X64_CODEGEN HISE_INCLUDE_SNEX
#endif
#endif


/** Config: SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP

Set to 1 if you want the memory address to be included in the data dump string. 
*/
#ifndef SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP
#define SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP 0
#endif

#include "../hi_lac/hi_lac.h"
#include "../hi_dsp_library/hi_dsp_library.h"


#if !HISE_INCLUDE_SNEX_X64_CODEGEN

namespace asmjit {
namespace x86 {

struct Compiler
{
    
};

} // namespace x86
} // namespace asmjit

#endif

#define HNODE_JIT_OPERATORS(X) \
    X(semicolon,     ";")        X(dot,          ".")       X(comma,        ",") \
    X(openParen,     "(")        X(closeParen,   ")")       X(openBrace,    "{")    X(closeBrace, "}") \
    X(openBracket,   "[")        X(closeBracket, "]")       X(double_colon, "::")   X(colon,        ":")    X(question,   "?") \
    X(typeEquals,    "===")      X(equals,       "==")      X(assign_,       "=") \
    X(typeNotEquals, "!==")      X(notEquals,    "!=")      X(logicalNot,   "!") \
    X(plusEquals,    "+=")       X(plusplus,     "++")      X(plus,         "+") X(pointer_, "->") \
    X(minusEquals,   "-=")       X(minusminus,   "--")      X(minus,        "-") \
    X(timesEquals,   "*=")       X(times,        "*")       X(divideEquals, "/=")   X(divide,     "/") \
    X(moduloEquals,  "%=")       X(modulo,       "%")       X(xorEquals,    "^=")   X(bitwiseXor, "^") \
    X(andEquals,     "&=")       X(logicalAnd,   "&&")      X(bitwiseAnd,   "&") \
    X(orEquals,      "|=")       X(logicalOr,    "||")      X(bitwiseOr,    "|")  \
     X(lessThanOrEqual,  "<=")  X(lessThan,   "<")  		X(destructor,   "~") \
      X(greaterThanOrEqual, ">=")  X(greaterThan,  ">")	    X(syntax_tree_variable, "$") 

#define HNODE_JIT_KEYWORDS(X) \
    X(float_,      "float")      X(int_, "int")     X(double_,  "double")   X(bool_, "bool") \
    X(return_, "return")		X(true_,  "true")   X(false_,    "false")	X(const_, "const") \
	X(void_, "void")			X(public_, "public")	X(private_, "private") \
	X(class_, "class")			X(for_, "for")      X(enum_, "enum") \
	X(if_, "if")				X(else_, "else")	X(protected_, "protected") \
	X(auto_, "auto")			X(struct_, "struct")	\
	X(using_, "using")		    X(static_, "static")	X(break_, "break") X(continue_, "continue")			X(namespace_, "namespace") \
	X(template_, "template")    X(typename_, "typename") X(while_, "while") \
	X(__internal_property, "__internal_property"); X(this_, "this"); X(operator_, "operator")

namespace JitTokens
{
#define DECLARE_HNODE_JIT_TOKEN(name, str)  static const char* const name = str;
	HNODE_JIT_KEYWORDS(DECLARE_HNODE_JIT_TOKEN)
		HNODE_JIT_OPERATORS(DECLARE_HNODE_JIT_TOKEN)
		DECLARE_HNODE_JIT_TOKEN(eof, "$eof")
	DECLARE_HNODE_JIT_TOKEN(literal, "$literal")
	DECLARE_HNODE_JIT_TOKEN(identifier, "$identifier")
}


namespace snex
{
    namespace jit
    {
#define DECLARE_ID(x) static const juce::String x(#x);

        namespace OptimizationIds
        {
            DECLARE_ID(SmallObjectOptimisation);
            DECLARE_ID(ConstantFolding);
            DECLARE_ID(Inlining);
            DECLARE_ID(AutoVectorisation);
            DECLARE_ID(DeadCodeElimination);
            DECLARE_ID(BinaryOpOptimisation);
            DECLARE_ID(LoopOptimisation);
            DECLARE_ID(AsmOptimisation)
            DECLARE_ID(NoSafeChecks);

#if HISE_INCLUDE_SNEX
			struct Helpers
			{

				static StringArray getDefaultIds()
				{
#if SNEX_MIR_BACKEND
					return { BinaryOpOptimisation, ConstantFolding, DeadCodeElimination };
#else
					return { BinaryOpOptimisation, ConstantFolding, DeadCodeElimination, Inlining, LoopOptimisation, AsmOptimisation, NoSafeChecks };
#endif
				}

				static StringArray getAllIds()
				{
					return { BinaryOpOptimisation, ConstantFolding, DeadCodeElimination, Inlining, LoopOptimisation, AsmOptimisation, NoSafeChecks };
				}
			};
#endif
        }

#undef DECLARE_ID
    }
}

#if HISE_INCLUDE_SNEX




#define SNEX_ASMJIT_BACKEND !SNEX_MIR_BACKEND


#if SNEX_ASMJIT_BACKEND
#define USE_ASMJIT_NAMESPACE using namespace asmjit;
#define ASMJIT_ONLY(x) x
#else
#define USE_ASMJIT_NAMESPACE
#define ASMJIT_ONLY(x)
#endif


#ifndef SNEX_INCLUDE_NMD_ASSEMBLY
#if defined (__arm__) || defined (__arm64__)
#define SNEX_INCLUDE_NMD_ASSEMBLY 0
#else
#define SNEX_INCLUDE_NMD_ASSEMBLY 1
#endif
#endif


#define JIT_MEMBER_WRAPPER_0(R, C, N)					  static R N(void* o) { return static_cast<C*>(o)->N(); };
#define JIT_MEMBER_WRAPPER_1(R, C, N, T1)				  static R N(void* o, T1 a1) { return static_cast<C*>(o)->N(a1); };
#define JIT_MEMBER_WRAPPER_2(R, C, N, T1, T2)			  static R N(void* o, T1 a1, T2 a2) { return static_cast<C*>(o)->N(a1, a2); };
#define JIT_MEMBER_WRAPPER_3(R, C, N, T1, T2, T3)		  static R N(void* o, T1 a1, T2 a2, T3 a3) { return static_cast<C*>(o)->N(a1, a2, a3); };
#define JIT_MEMBER_WRAPPER_4(R, C, N, T1, T2, T3, T4)	  static R N(void* o, T1 a1, T2 a2, T3 a3, T4 a4) { return static_cast<C*>(o)->N(a1, a2, a3, a4); };
#define JIT_MEMBER_WRAPPER_5(R, C, N, T1, T2, T3, T4, T5) static R N(void* o, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) { return static_cast<C*>(o)->N(a1, a2, a3, a4, a5); };



namespace snex
{
	struct ApiHelpers
	{
		enum DebugObjectTypes
		{
			LocalFunction = 9000,
			ApiCall,
			Template,
			Constants,
			BasicTypes,
			numDebugObjectTypes
		};

		static void getColourAndLetterForType(int type, Colour& colour, char& letter)
		{
			auto typedType = (Types::ID)type; // type;

			if (typedType < Types::ID::Dynamic)
			{
				colour = Types::Helpers::getColourForType(typedType);
				letter = Types::Helpers::getTypeChar(typedType);
			}

			if (type == ApiHelpers::DebugObjectTypes::Template)
			{
				colour = Colours::yellow.withSaturation(0.3f);
				letter = 'T';
			}

			if (type == ApiHelpers::DebugObjectTypes::Constants)
			{
				colour = Colours::blanchedalmond;
				letter = 'C';
			}

			if (type == ApiHelpers::DebugObjectTypes::BasicTypes)
			{
				colour = Colours::white;
				letter = 'T';
			}


			if (type == ApiHelpers::DebugObjectTypes::ApiCall)
			{
				colour = Colours::aqua;
				letter = 'A';
			}

			if (type == ApiHelpers::DebugObjectTypes::LocalFunction)
			{
				colour = Colours::dodgerblue;
				letter = 'F';
			}
		}
	};
}

#include "snex_core/snex_jit_ComplexType.h"
#include "snex_core/snex_jit_TypeInfo.h"
#include "snex_core/snex_jit_TemplateParameter.h"
#include "snex_core/snex_jit_Inliner.h"
#include "snex_public/snex_jit_FunctionData.h"

#include "snex_core/snex_jit_FunctionClass.h"
#include "snex_core/snex_jit_NamespaceHandler.h"
#include "snex_core/snex_jit_BaseScope.h"
#include "snex_public/snex_jit_GlobalScope.h"
#include "snex_mir/snex_MirObject.h"
#include "snex_core/snex_jit_JitCallableObject.h"
#include "snex_core/snex_jit_JitCompiledFunctionClass.h"
#include "snex_public/snex_jit_JitCompiler.h"
#include "snex_cpp_builder/snex_jit_CppBuilder.h"
#include <set>
#include "snex_cpp_builder/snex_jit_ValueTreeBuilder.h"

#include "snex_parser/snex_jit_PreProcessor.h"

namespace snex {
	namespace jit {
		using namespace juce;

#if HNODE_BOOL_IS_NOT_INT
		using BooleanType = unsigned char
#else
		using BooleanType = int;
#endif

		using PointerType = uint64_t;


		typedef uint64_t AddressType;
	} // end namespace jit
} // end namespace snex



#include "api/SnexApi.h"

#include "snex_library/snex_CallbackCollection.h"
#include "snex_library/snex_ExternalObjects.h"
#include "snex_library/snex_jit_ExternalComplexTypeLibrary.h"
#include "snex_public/snex_jit_JitCompiledNode.h"
#include "snex_library/snex_jit_NativeDspFunctions.h"


#include "snex_components/snex_WorkbenchData.h"
#include "snex_components/snex_ExtraComponents.h"

#include "unit_test/snex_jit_UnitTestCase.h"
#include "snex_components/snex_JitPlayground.h"
#include "snex_components/snex_DebugTools.h"
#endif

#if HISE_INCLUDE_SNEX
using SnexDebugHandler = snex::jit::DebugHandler;
using SnexExpressionPtr = snex::JitExpression::Ptr;
#else
struct SnexDebugHandler
{
    virtual ~SnexDebugHandler() {}
    virtual void logMessage(int level, const juce::String& s) {};
};

using SnexExpressionPtr = void*;
#endif

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

#pragma once


#include <typeindex>

#define HNODE_JIT_MAX_NUM_FUNCTION_PARAMETERS 5

#include "JitFunctions.h"
#include "hnode_jit_BaseScope.h"
#include "hnode_jit_GlobalScope.h"
#include "hnode_jit_JitCallableObject.h"
#include "hnode_jit_JitCompiledFunctionClass.h"
#include "hnode_jit_JitCompiler.h"

namespace hnode {
namespace jit {
using namespace juce;

using TypeInfo = std::type_index;

#if HNODE_BOOL_IS_NOT_INT
using BooleanType = unsigned char
#else
using BooleanType = int;
#endif

using PointerType = uint64_t;

#if JUCE_64BIT
typedef uint64_t AddressType;
#else
typedef uint32_t AddressType;
#endif





#if 0
/** The scope object that holds all global values and compiled functions. 
*
*	After compilation, this object will contain all functions and storage for the global variables of one instance.
*
*/
class JITScope
{
public:

	/** Creates a new scope. */
	JITScope(GlobalScope* globalMemoryPool=nullptr);

	~JITScope();

	void setName(const Identifier& name_)
	{
		name = name_;
	}

	Identifier getName() { return name; };

	String dumpAssembly();

	int isBufferOverflow(int globalIndex) const;

	Result getCompiledFunction(FunkyFunctionData& functionToSearch);

	typedef ReferenceCountedObjectPtr<JITScope> Ptr;

	class Pimpl;

private:

	Identifier name;

	friend class GlobalParser;

	ScopedPointer<Pimpl> pimpl;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JITScope)
};
#endif



#if 0

class JITCompiler : public DynamicObject
{
public:

	

	Identifier getModuleName() { return Identifier("Example"); };

	/** Creates a compiler for the given code.
	*
	*	It just preprocesses the code on creation. In order to actually compile, call compileAndReturnScope()
	*/
	JITCompiler(const String& codeToCompile);

	/** Destroys the compiler. The lifetime of created scopes are independent. */
	~JITCompiler();

	/** Compiles the code that was passed in during construction and returns a HiseJITScope object that contains the global variables and functions. */
	JITScope* compileAndReturnScope(GlobalScope* globalMemoryPool = nullptr) const;

	JITScope* compileOneLineExpressionAndReturnScope(const Array<Types::ID>& typeList, GlobalScope* globalMemoryPool = nullptr) const;

	/** Checks if the compilation went smooth. */
	bool wasCompiledOK() const;

	/** Get the error message for compilation errors. */
	String getErrorMessage() const;

	/** Returns the code the compiler will be using to create scopes. */
	String getCode(bool getPreprocessedCode) const;

private:

	class Pimpl;

	ScopedPointer<Pimpl> pimpl;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JITCompiler)
};
#endif







} // end namespace jit
} // end namespace hnode


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

namespace snex {
namespace jit {
using namespace juce;



/** A JIT compiler for a C language subset based on AsmJIT.

	It is supposed to be used as "scripting" language for the inner loop of a DSP routine. It offers about 70% - 80% performance of
	native C++ performance (clang -O3)!

	Features:

	- supported types: float, double, int, buffer (a native float array with safe checks)
	- strictly typed, parser throws an error for type mismatches (enforces casts)
	- define functions which can be called from C++ or from other JITted functions
	- use common inbuilt functions like powf, sin, cos, etc.
	- use local variables without runtime penalties
	- call functions with max three parameters
	- simple branching using the a ? b : c logic

	You can give it a C code snippet as string and it will compile and return instances of scopes that can be used independently.

	Example:

	C code:

		float x = 1.2f;

		int calculateSomething(double input, int shouldDoubleOutput)
		{
			const float x1 = (float)input * x;
			const float x2 = shouldDoubleOutput > 1 ? x1 * 2.0f : x1;
			return (int)x2;
		};

	C++ side:

		GlobalScope pool;
		Compiler compiler(pool);

		String code = "float member = 8.0f; float square(float input){ member = input; return (float)input * input; }";

		if (auto obj = compiler.compileJitObject(code))
		{
			auto f = obj["square"];

			auto ptr = obj.getVariablePtr("member");

			DBG(ptr->toFloat()); // 8.0f

			auto returnValue = f.call<float>(12.0f);
			DBG(returnValue); // 144.0f

			DBG(ptr->toFloat()); // 12.0f
		}
*/
class Compiler: public ReferenceCountedObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<Compiler>;

	

	~Compiler();
	Compiler(GlobalScope& memoryPool);

	void setDebugHandler(DebugHandler* newHandler, bool useLineNumbersInErrorMessage=true);

	JitObject compileJitObject(const juce::String& code);

	NamespaceHandler& parseWithoutCompilation(const juce::String& code);

	/** Compile a class that you want to use from C++. */
	template <class T> T* compileJitClass(const juce::String& code, const String& classId)
	{
		auto obj = compileJitObject(code);
		auto typePtr = getComplexType(NamespacedIdentifier::fromString(classId));
		return new T(std::move(obj), typePtr);
	};

	Result getCompileResult();

	juce::String getAssemblyCode();
	juce::String dumpSyntaxTree() const;
	juce::String dumpNamespaceTree() const;
	juce::String getLastCompiledCode() { return lastCode; }

	/** This registers an external object as complex type.

	If a similar type already exists, it returns the pointer to this type object,
	so make sure you use the return value of this function for further processing. */
	ComplexType::Ptr registerExternalComplexType(ComplexType::Ptr t);

	ComplexType::Ptr getComplexType(const NamespacedIdentifier& s, const Array<TemplateParameter>& tp = {}, bool checkPreExisting=false);

	void addConstant(const NamespacedIdentifier& s, const VariableStorage& v);
	void addTemplateClass(const TemplateObject& c);

	NamespaceHandler& getNamespaceHandler();
	NamespaceHandler::Ptr getNamespaceHandlerReference() { return handler; }
	FunctionClass::Ptr getInbuiltFunctionClass();
	void initInbuildFunctions();

	static int compileCount;

	void reset();

	bool allowInlining() const;

	GlobalScope& getGlobalScope() { return memory; }
	const GlobalScope& getGlobalScope() const { return memory; }

private:

	NamespaceHandler::Ptr handler;
	juce::String lastCode;
	juce::String preprocessedCode;
	ClassCompiler* compiler = nullptr;
	GlobalScope& memory;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compiler);
	JUCE_DECLARE_WEAK_REFERENCEABLE(Compiler);
};


}
}

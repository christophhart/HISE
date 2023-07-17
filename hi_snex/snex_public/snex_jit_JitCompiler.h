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

	Result getCompileResult();

	juce::String getAssemblyCode();
	ValueTree getAST() const;
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

	FunctionClass::Map getFunctionMap();

private:

	Result cr;

	String assembly;
	NamespaceHandler::Ptr handler;
	juce::String lastCode;
	juce::String preprocessedCode;
	ClassCompiler* compiler = nullptr;
	GlobalScope& memory;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Compiler);
	JUCE_DECLARE_WEAK_REFERENCEABLE(Compiler);
};

/** This class will take a stringified init value list and converts it to a memory block.

	It also allows iterating over each element and perform an operation based on its type and value.
*/
struct InitValueParser
{
	InitValueParser(const String& v) :
		input(v)
	{}

	/** Pass in a function that will be called for each element in the initialiser list.

		You can only call this when you've created the object with a Base64 string.

		The function you pass in must have the prototype void(uint32 offset, Types::ID type, const VariableStorage&)

		- offset: will contain the byte offset from the start
		- type: the value type - either int, double or float
		- value: the value.

		Note that this also supports dynamic initialisation. In this case the value will be of type Types::ID::Pointer (while the type will contain the actual value type)
		with the address pointing to the index in the child elements  (so 0x00000002 will point to the third child element).
		This can be used to dynamically initialise the data.

		You can take a look at the dumpContent() for an example usage of this function.
	 */
	void forEach(const std::function<void(uint32 offset, Types::ID type, const VariableStorage& value)>& f) const;

	uint32 getNumBytesRequired() const;

	String dumpContent() const;

	String getB64() const;

	String input;
};

struct SyntaxTreeExtractor
{

	static ValueTree getSyntaxTree(const String& b64);

	/** Extracts the AST from the compiler and returns a compressed Base64 string. */
	static String getBase64SyntaxTree(ValueTree v);

    static String getBase64DataLayout(const Array<ValueTree>& dataLayouts);
    
    static Array<ValueTree> getDataLayoutTrees(const String& b64);
    
	static bool isBase64Tree(const String& code);

private:

	static void removeLineInfo(ValueTree& v);
};


}
}

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

namespace hnode {
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
	- call functions with max two parameters
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
class Compiler
{
public:

	struct Tokeniser : public juce::CodeTokeniser
	{
		int readNextToken(CodeDocument::Iterator& source) override;

		CodeEditorComponent::ColourScheme getDefaultColourScheme() override
		{
			CodeEditorComponent::ColourScheme scheme;

			scheme.set("Error", Colour(0xFFCC6666));
			scheme.set("Warning", Colour(0xFFFFFF66));
			scheme.set("Pass", Colour(0xFF66AA66));
			scheme.set("Process", Colours::white);
			scheme.set("VerboseProcess", Colours::lightgrey);
			scheme.set("AsmJit", Colours::grey);

			return scheme;
		}
	};

	struct DebugHandler
	{
		virtual ~DebugHandler() {};

		virtual void logMessage(const String& s) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(DebugHandler);
	};

	~Compiler();
	Compiler(GlobalScope& memoryPool);

	void setDebugHandler(DebugHandler* newHandler);

	JitObject compileJitObject(const String& code);

	Result getCompileResult();

	String getAssemblyCode();

	String getLastCompiledCode() { return lastCode; }

private:

	String lastCode;
	ClassCompiler* compiler;
};


}
}
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


namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;


Compiler::Compiler(GlobalScope& memoryPool)
{
	compiler = new ClassCompiler(&memoryPool, handler);
	memoryPool.registerFunctionsToNamespaceHandler(handler);
}

Compiler::~Compiler()
{
	delete compiler;
}


juce::String Compiler::getAssemblyCode()
{
	return compiler->assembly;
}



juce::String Compiler::dumpSyntaxTree() const
{
	if (compiler->syntaxTree != nullptr)
	{
		return dynamic_cast<SyntaxTree*>(compiler->syntaxTree.get())->dump();
	}

	return {};
}

juce::String Compiler::dumpNamespaceTree() const
{
	return compiler->namespaceHandler.dump();
}

ComplexType::Ptr Compiler::registerExternalComplexType(ComplexType::Ptr t)
{
	return compiler->namespaceHandler.registerComplexTypeOrReturnExisting(t);
}

ComplexType::Ptr Compiler::getComplexType(const NamespacedIdentifier& s)
{
	return compiler->namespaceHandler.getComplexType(s);
	return nullptr;
}

void Compiler::registerVariadicType(VariadicSubType::Ptr p)
{
	compiler->namespaceHandler.addVariadicType(p);
}

juce::Result Compiler::getCompileResult()
{
	return compiler->getLastResult();
}


JitObject Compiler::compileJitObject(const juce::String& code)
{
	lastCode = code;
	return JitObject(compiler->compileAndGetScope(code));
}


void Compiler::setDebugHandler(DebugHandler* newHandler)
{
	compiler->setDebugHandler(newHandler);
	compiler->parentScope->getGlobalScope()->addDebugHandler(newHandler);
}



}
}
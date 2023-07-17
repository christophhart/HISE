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
USE_ASMJIT_NAMESPACE;


class ClassCompiler final : public BaseCompiler
{
public:

	ClassCompiler(BaseScope* parentScope_, NamespaceHandler& handler, const NamespacedIdentifier& classInstanceId = {}) :
		BaseCompiler(handler),
		instanceId(classInstanceId),
		parentScope(parentScope_),
		lastResult(Result::ok())
	{
		if (auto gs = parentScope->getGlobalScope())
		{
			const auto& optList = gs->getOptimizationPassList();

			if (!optList.isEmpty())
			{
				OptimizationFactory f;

				for (const auto& id : optList)
					addOptimization(f.createOptimization(id), id);
			}
		}

		newScope = new AsmJitFunctionCollection(parentScope, classInstanceId);
	};

	virtual ~ClassCompiler()
	{
		syntaxTree = nullptr;
	}

	void setFunctionCompiler(AsmJitX86Compiler* cc)
	{
		asmCompiler = cc;
	}

	AsmJitRuntime* getRuntime()
	{
		if (parentRuntime != nullptr)
			return parentRuntime;

		return newScope->pimpl->runtime;
	}

	bool parseOnly = false;
	AsmJitRuntime* parentRuntime = nullptr;

	AsmJitFunctionCollection* compileAndGetScope(const ParserHelpers::CodeLocation& classStart, int length)
	{
		ClassParser parser(this, classStart, length);

		if (newScope == nullptr)
		{
			newScope = new AsmJitFunctionCollection(parentScope, instanceId);
		}

		newScope->pimpl->handler = &namespaceHandler;

		try
		{
			parser.currentScope = newScope->pimpl;

			setCurrentPass(BaseCompiler::Parsing);

			NamespaceHandler::ScopedNamespaceSetter sns(namespaceHandler, Identifier());

			syntaxTree = parser.parseStatementList();

			auto sTree = dynamic_cast<SyntaxTree*>(syntaxTree.get());

			executePass(ComplexTypeParsing, newScope->pimpl, sTree);

			newScope->pimpl->getRootData()->finalise();

			executePass(DataAllocation, newScope->pimpl, sTree);
			executePass(DataInitialisation, newScope->pimpl, sTree);
			executePass(PreSymbolOptimization, newScope->pimpl, sTree);
			executePass(ResolvingSymbols, newScope->pimpl, sTree);
			executePass(TypeCheck, newScope->pimpl, sTree);

			if (parseOnly)
			{
				lastResult = Result::ok();
				return nullptr;
			}

			executePass(PostSymbolOptimization, newScope->pimpl, sTree);

			executePass(FunctionTemplateParsing, newScope->pimpl, sTree);
			executePass(FunctionParsing, newScope->pimpl, sTree);

			// Optimize now

			executePass(FunctionCompilation, newScope->pimpl, sTree);

			if (lastResult.wasOk())
				lastResult = newScope->pimpl->getRootData()->callRootConstructors();
		}
		catch (ParserHelpers::Error& e)
		{
			syntaxTree = nullptr;

			auto m = e.toString(useCodeSnippetInErrorMessage() ? ParserHelpers::Error::Format::CodeExample : ParserHelpers::Error::Format::LineNumbers);

			logMessage(BaseCompiler::Error, m);
			lastResult = Result::fail(m);
		}

		return newScope.release();
	}

	AsmJitFunctionCollection* compileAndGetScope(const juce::String& code)
	{


		ParserHelpers::CodeLocation loc(code.getCharPointer(), code.getCharPointer());

		return compileAndGetScope(loc, code.length());
	}

	Result getLastResult() { return lastResult; }

	ScopedPointer<AsmJitFunctionCollection> newScope;

	AsmJitX86Compiler* asmCompiler;

	juce::String assembly;

	Result lastResult;

	BaseScope* parentScope;
	NamespacedIdentifier instanceId;

	Operations::Statement::Ptr syntaxTree;
};



}
}
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
USE_ASMJIT_NAMESPACE;



class BaseCompiler
{
public:

	struct OptimisationSucess
	{
		juce::String message;
	};

	struct DeadCodeException
	{
		DeadCodeException(ParserHelpers::CodeLocation l) : location(l) {};

		ParserHelpers::CodeLocation location;
	};

	enum MessageType
	{
		Error,
		Warning,
		PassMessage,
		ProcessMessage,
		VerboseProcessMessage,
		AsmJitMessage,
		ValueDump,
		ValueName,
		Blink,
		numMessageTypes
	};

	BaseCompiler(NamespaceHandler& handler);

	virtual ~BaseCompiler() {};

	enum Pass
	{
		Parsing,
		ComplexTypeParsing,
		PreSymbolOptimization,
		DataAllocation,
		DataInitialisation,
		ResolvingSymbols,
		TypeCheck,
		PostSymbolOptimization,
		FunctionTemplateParsing,
		FunctionParsing,
		FunctionCompilation,
		RegisterAllocation,
		CodeGeneration,
		numPasses
	};

	struct ScopedUnsafeBoundChecker
	{
		ScopedUnsafeBoundChecker(BaseCompiler* c):
			compiler(c)
		{
			wasAllowed = compiler->unsafeIndexAllowed;
			compiler->unsafeIndexAllowed = true;
		}

		~ScopedUnsafeBoundChecker()
		{
			compiler->unsafeIndexAllowed = wasAllowed;
		}

		BaseCompiler* compiler;
		bool wasAllowed = false;
	};

	struct ScopedPassSwitcher
	{
		ScopedPassSwitcher(BaseCompiler* compiler, Pass newPass):
			c(compiler)
		{
			oldPass = c->getCurrentPass();
			c->setCurrentPass(newPass);
		}

		~ScopedPassSwitcher()
		{
			c->setCurrentPass(oldPass);
		}

		WeakReference<BaseCompiler> c;
		BaseCompiler::Pass oldPass = numPasses;
	};

	struct OptimizationPassBase
	{
		virtual ~OptimizationPassBase() {};
		virtual juce::String getName() const = 0;

		/** This will get called before each statement so you can reset the state. */
		virtual void reset() {};
	};

	void setDebugHandler(DebugHandler* l)
	{
		debugHandler = l;
	}

	bool hasLogger() const { return debugHandler != nullptr; }

	void logMessage(MessageType level, const juce::String& s)
	{
		if (!hasLogger())
			return;

		if (level > verbosity)
			return;

		if (debugHandler != nullptr)
			debugHandler->logMessage(level, s);
	}

	void setMessageLevel(MessageType maxVerbosity)
	{
		verbosity = maxVerbosity;
	}

	static bool isOptimizationPass(Pass p)
	{
		return p == PreSymbolOptimization ||
			p == PostSymbolOptimization;
	}

	void setCurrentPass(Pass p)
	{
		currentPass = p;

		switch (currentPass)
		{
		case Parsing:					    logMessage(PassMessage, "Parsing class statements"); break;
		case FunctionParsing:				logMessage(PassMessage, "Parsing Functions"); break;
		case PreSymbolOptimization:			logMessage(PassMessage, "Optimization Stage 1"); break;
		case PostSymbolOptimization:		logMessage(PassMessage, "Optimization Stage 2"); break;
		case ResolvingSymbols:				logMessage(PassMessage, "Resolving symbols"); break;
		case RegisterAllocation:			logMessage(PassMessage, "Allocating Registers"); break;
		case TypeCheck:						logMessage(PassMessage, "Checking Types"); break;
		case FunctionCompilation:			logMessage(PassMessage, "Compiling Functions"); break;
		case CodeGeneration:				logMessage(PassMessage, "Generating assembly code"); break;
		case numPasses:
		default:
			break;
		}
	}

	FunctionClass* getInbuiltFunctionClass()
	{
		return inbuildFunctions.get();
	}

	Pass getCurrentPass() { return currentPass; }

	void executePass(Pass p, BaseScope* scope, ReferenceCountedObject* statement);

	void executeScopedPass(Pass p, BaseScope* scope, ReferenceCountedObject* statement);

	void executeOptimization(ReferenceCountedObject* statement, BaseScope* scope);
	
	void optimize(ReferenceCountedObject* statement, BaseScope* scope, bool useExistingPasses=true);

	void addOptimization(OptimizationPassBase* newPass, const String& id = String())
	{
		if (newPass != nullptr)
		{
			passes.add(newPass);
			optimisationIds.add(newPass->getName());
		}
		else if (id.isNotEmpty())
			optimisationIds.add(id);
	}

	StringArray getOptimizations() const;

	AssemblyRegister::Ptr getRegFromPool(BaseScope* scope, TypeInfo type)
	{
		return registerPool.getNextFreeRegister(scope, type);
	}

	AssemblyRegisterPool registerPool;

	

	NamespaceHandler& namespaceHandler;

	Types::ID getRegisterType(const TypeInfo& t) const;

	TypeInfo convertToNativeTypeIfPossible(const TypeInfo& t) const;

	bool fitsIntoNativeRegister(ComplexType* t) const;

	bool allowSmallObjectOptimisation() const;

	void setInbuildFunctions();

	bool allowUnsafeIndexes() const
	{
		return unsafeIndexAllowed;
	}

	MathFunctions& getMathFunctionClass();

	void setUseCodeInErrorMessage(bool shouldIncludeCodeSnippet)
	{
		useCodeSnippet = shouldIncludeCodeSnippet;
	}

	bool useCodeSnippetInErrorMessage() const { return useCodeSnippet; }

	bool& isProcessingInlineFunction()
	{
		return inlineProcessFlag;
	}

private:

	bool inlineProcessFlag = false;

	bool useCodeSnippet = false;

	FunctionClass::Ptr mathFunctions;

	bool unsafeIndexAllowed = false;

	FunctionClass::Ptr inbuildFunctions;

    OptimizationPassBase* currentOptimization = nullptr;
    
	OwnedArray<OptimizationPassBase> passes;
	Array<Identifier> optimisationIds;

	MessageType verbosity = numMessageTypes;

	Pass currentPass;

	WeakReference<DebugHandler> debugHandler;

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(BaseCompiler)
};

}
}

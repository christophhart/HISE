/*
  ==============================================================================

    hi_native_jit.cpp
    Created: 8 Mar 2017 12:09:56am
    Author:  Christoph

  ==============================================================================
*/

#include "hi_jit_compiler.h"

using namespace juce;

#if JUCE_64BIT
#include "TokenIterator.h"
#include "Parser.h"
#include "FunctionParserBase.h"
#include "FunctionParser.h"
#include "GlobalParser.h"
#include "Pimpls.cpp"
#else
struct PreprocessorParser
{
	PreprocessorParser(const String& code) {};
	String process() { return String(); };
};
#endif

class NativeJITCompiler::Pimpl
{
public:

	Pimpl(const String& codeToCompile) :
		preprocessor(codeToCompile),
		code(preprocessor.process()),
		compiledOK(false)
	{

	};

	~Pimpl() {}

	NativeJITScope* compileAndReturnScope()
	{
#if JUCE_64BIT
		ScopedPointer<NativeJITScope> scope = new NativeJITScope();

		GlobalParser globalParser(code, scope);

		try
		{
			globalParser.parseStatementList();
		}
		catch (ParserHelpers::CodeLocation::Error e)
		{
			errorMessage = "Line " + String(getLineNumberForError(e.offsetFromStart)) + ": " + e.errorMessage;

			compiledOK = false;
			return nullptr;
		}

		compiledOK = true;
		errorMessage = String();
		return scope.release();
#else
		errorMessage = "NativeJIT currently only supports 64bit architecture";
		compiledOK = false;
		return nullptr;
#endif

	}

	bool wasCompiledOK() const { return compiledOK; };
	String getErrorMessage() const { return errorMessage; };

private:

	int getLineNumberForError(int charactersFromStart)
	{
		int line = 1;

		for (int i = 0; i < jmin<int>(charactersFromStart, code.length()); i++)
		{
			if (code[i] == '\n')
			{
				line++;
			}
		}

		return line;
	}

	PreprocessorParser preprocessor;
	const String code;
	bool compiledOK;
	String errorMessage;
};



NativeJITCompiler::NativeJITCompiler(const String& codeToCompile)
{
	pimpl = new Pimpl(codeToCompile);
}

NativeJITCompiler::~NativeJITCompiler()
{
	pimpl = nullptr;
}


NativeJITScope* NativeJITCompiler::compileAndReturnScope() const
{
	return pimpl->compileAndReturnScope();
}

bool NativeJITCompiler::wasCompiledOK() const
{
	return pimpl->wasCompiledOK();
}

String NativeJITCompiler::getErrorMessage() const
{
	return pimpl->getErrorMessage();
}

#if JUCE_64BIT

NativeJITScope::NativeJITScope()
{
	pimpl = new Pimpl();
}

NativeJITScope::~NativeJITScope()
{
	pimpl = nullptr;
}

int NativeJITScope::getNumGlobalVariables() const
{
	return pimpl->globals.size();
}

var NativeJITScope::getGlobalVariableValue(int globalIndex) const
{
	if (globalIndex < getNumGlobalVariables())
	{
		auto* g = pimpl->globals[globalIndex];

		if (NativeJITTypeHelpers::matchesType<float>(g->getType())) return var(GlobalBase::get<float>(g));
		if (NativeJITTypeHelpers::matchesType<double>(g->getType())) return var(GlobalBase::get<double>(g));
		if (NativeJITTypeHelpers::matchesType<int>(g->getType())) return var(GlobalBase::get<int>(g));
		if (NativeJITTypeHelpers::matchesType<Buffer*>(g->getType())) return GlobalBase::get<Buffer*>(g)->buffer->getNumSamples();
	}

	return var();
}

TypeInfo NativeJITScope::getGlobalVariableType(int globalIndex) const
{
	if (globalIndex < getNumGlobalVariables())
	{
		return pimpl->globals[globalIndex]->getType();
	}

	return typeid(void);
}

Identifier NativeJITScope::getGlobalVariableName(int globalIndex) const
{
	if (globalIndex < getNumGlobalVariables())
	{
		return pimpl->globals[globalIndex]->id;
	}

	return Identifier();
}

template <typename ReturnType, typename...ParameterTypes> ReturnType(*NativeJITScope::getCompiledFunction(const Identifier& id))(ParameterTypes...)
{
	int parameterAmount = sizeof...(ParameterTypes);

	if (parameterAmount == 0)
	{
		return pimpl->getCompiledFunction0<ReturnType, ParameterTypes...>(id);
	}
	else if (parameterAmount == 1)
	{
		return pimpl->getCompiledFunction1<ReturnType, ParameterTypes...>(id);
	}
	else if (parameterAmount == 2)
	{
		return pimpl->getCompiledFunction2<ReturnType, ParameterTypes...>(id);
	}

	return nullptr;
}

#else

NativeJITScope::NativeJITScope() {}
NativeJITScope::~NativeJITScope() {}
int NativeJITScope::getNumGlobalVariables() const { return 0; }
var NativeJITScope::getGlobalVariableValue(int /*globalIndex*/) const { return var(); }
TypeInfo NativeJITScope::getGlobalVariableType(int globalIndex) const { return typeid(void); }
Identifier NativeJITScope::getGlobalVariableName(int globalIndex) const { return Identifier(); }
template <typename ReturnType, typename...ParameterTypes> ReturnType(*NativeJITScope::getCompiledFunction(const Identifier& id))(ParameterTypes...) { return nullptr; }

#endif

NativeJITDspModule::NativeJITDspModule(const NativeJITCompiler* compiler)
{
	scope = compiler->compileAndReturnScope();

	static const Identifier proc("process");
	static const Identifier prep("prepareToPlay");
	static const Identifier init_("init");

	compiledOk = compiler->wasCompiledOK();

	if (compiledOk)
	{
		pf = scope->getCompiledFunction<float, float>(proc);
		initf = scope->getCompiledFunction<int>(init_);
		pp = scope->getCompiledFunction<int, double, int>(prep);

		allFunctionsDefined = pf != nullptr && pp != nullptr && initf != nullptr;
	}
}

void NativeJITDspModule::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (allOK())
	{
		pp(sampleRate, samplesPerBlock);
	}
}

void NativeJITDspModule::init()
{
	if (allOK())
	{
		initf();
	}
}

NativeJITScope* NativeJITDspModule::getScope()
{
	return scope;
}

const NativeJITScope* NativeJITDspModule::getScope() const
{
	return scope;
}

void NativeJITDspModule::processBlock(float* data, int numSamples)
{
	if (allOK())
	{
		for (int i = 0; i < numSamples; i++)
		{
			data[i] = pf(data[i]);
		}
	}
}

bool NativeJITDspModule::allOK() const
{
	return compiledOk && allFunctionsDefined;
}



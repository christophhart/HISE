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
    bool shouldUseSafeBufferFunctions() {return true;};
};
#endif

class NativeJITCompiler::Pimpl
{
public:

	Pimpl(const String& codeToCompile) :
		preprocessor(codeToCompile),
		code(preprocessor.process()),
		compiledOK(false),
		useSafeFunctions(preprocessor.shouldUseSafeBufferFunctions())
	{

	};

	~Pimpl() {}

	NativeJITScope* compileAndReturnScope()
	{
#if JUCE_64BIT
		ScopedPointer<NativeJITScope> scope = new NativeJITScope();

		GlobalParser globalParser(code, scope, useSafeFunctions);

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

	bool useSafeFunctions;
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
		if (NativeJITTypeHelpers::matchesType<Buffer*>(g->getType())) return var("Buffer"); // GlobalBase::get<Buffer*>(g)->getObject()->size;
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


void NativeJITScope::setGlobalVariable(const juce::Identifier& id, juce::var value)
{
	pimpl->setGlobalVariable(id, value);
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


int NativeJITScope::isBufferOverflow(int globalIndex) const
{
	return pimpl->globals[globalIndex]->hasOverflowError();
}


#else

NativeJITScope::NativeJITScope() {}
NativeJITScope::~NativeJITScope() {}
int NativeJITScope::getNumGlobalVariables() const { return 0; }
var NativeJITScope::getGlobalVariableValue(int /*globalIndex*/) const { return var(); }
TypeInfo NativeJITScope::getGlobalVariableType(int globalIndex) const { return typeid(void); }
Identifier NativeJITScope::getGlobalVariableName(int globalIndex) const { return Identifier(); }
template <typename ReturnType, typename...ParameterTypes> ReturnType(*NativeJITScope::getCompiledFunction(const Identifier& id))(ParameterTypes...) { return nullptr; }
int NativeJITScope::isBufferOverflow(int globalIndex) const { return -1; }

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

		if (overFlowCheckEnabled)
		{
			overflowIndex = -1;

			for (int i = 0; i < scope->getNumGlobalVariables(); i++)
			{
				overflowIndex = jmax<int>(overflowIndex, scope->isBufferOverflow(i));
				if (overflowIndex != -1)
				{
					throw String("Buffer overflow for " + scope->getGlobalVariableName(i) + " at index " + String(overflowIndex));
				}
			}
		}
	}
}

bool NativeJITDspModule::allOK() const
{
	return compiledOk && allFunctionsDefined;
}


void NativeJITDspModule::enableOverflowCheck(bool shouldCheckForOverflow)
{
	overFlowCheckEnabled = shouldCheckForOverflow;
	overflowIndex = -1;
}


template <typename T> class NativeJITTestCase
{
public:
    
    NativeJITTestCase(const String& stringToTest):
    code(stringToTest)
    {
        compiler = new NativeJITCompiler(code);
        scope = compiler->compileAndReturnScope();
    }
    
    T getResult(T input)
    {
        if(!compiler->wasCompiledOK())
        {
            DBG(compiler->getErrorMessage());
            return T();
        }
        
        auto t = scope->getCompiledFunction<T, T>("test");
        
        return t(input);
    };
    
    void setup()
    {
        if(!compiler->wasCompiledOK())
        {
            DBG(compiler->getErrorMessage());
            return;
        }
        
        auto t = scope->getCompiledFunction<T, T>("setup");
        
        t(1.0f);
    }
    
private:
    
    String code;
    
    ScopedPointer<NativeJITCompiler> compiler;
    ScopedPointer<NativeJITScope> scope;
    
};




#define CREATE_TEST(x) test = new NativeJITTestCase<float>(x);
#define CREATE_TYPED_TEST(x) test = new NativeJITTestCase<T>(x);
#define CREATE_TEST_SETUP(x) test = new NativeJITTestCase<float>(x); test->setup();

#define EXPECT(testName, input, result) expectEquals<float>(test->getResult(input), result, testName);
#define EXPECT_TYPED(testName, input, result) expectEquals<T>(test->getResult(input), result, testName);

#define T_A + getLiteral<T>(a) +
#define T_B + getLiteral<T>(b) +
#define T_1 + getLiteral<T>(1.0) +
#define T_0 + getLiteral<T>(0.0) +

class NativeJITUnitTest: public UnitTest
{
public:
    
    NativeJITUnitTest(): UnitTest("NativeJIT UnitTest") {}
    
    void runTest() override
    {
        testOperations<float>();
        testOperations<double>();
        testOperations<int>();
        
        testGlobals();
        
        testFunctionCalls();
        
    }
    
private:
    
    void testGlobals()
    {
        beginTest("Testing Global variables");
        
        ScopedPointer<NativeJITTestCase<float>> test;
        
        
        CREATE_TEST("float x; float test(float i){ x=7.0f; return x; };")
        EXPECT("Global float", 2.0f, 7.0f)
        
        CREATE_TEST("float x=2.0f;float test(float i){return x*2.0f;};")
        EXPECT("Global float with operation", 2.0f, 4.0f)
        
        CREATE_TEST("int x=2;float test(float i){return (float)x;};")
        EXPECT("Global cast", 2.0f, 2.0f)
        
        CREATE_TEST_SETUP("Buffer b(11);float setup(float i){b[10]=3.0f;return 1.0f;};float test(float i){return b[10];};")
        EXPECT("Buffer test", 2.0f, 3.0f)
        
        CREATE_TEST("int c=0;float test(float i){c+=1;c+=1;c+=1;return (float)c;};")
        EXPECT("Incremental", 0.0f, 3.0f)
        
        CREATE_TEST("Buffer b(2); float test(float i){ return b[7];};")
        EXPECT("Buffer Overflow", 0.0f, 0.0f)
    }
    
    template <typename T> String getTypeName() const
    {
        if(NativeJITTypeHelpers::is<T, float>()) return "float";
        if(NativeJITTypeHelpers::is<T, double>()) return "double";
        if(NativeJITTypeHelpers::is<T, int>()) return "int";
        if(NativeJITTypeHelpers::is<T, Buffer*>()) return "Buffer";
    }
    
    template <typename T> String getTestSignature()
    {
        return getTypeName<T>() + " test(" + getTypeName<T>() + " input){%BODY%};";
    }
    
    template <typename T> String getTestFunction(const String& body)
    {
        String x = getTestSignature<T>();
        return x.replace("%BODY%", body);
    }
    
    template <typename T> String getLiteral(double value)
    {
        if(NativeJITTypeHelpers::is<T, float>()) return String((float)value) + ".f";
        if(NativeJITTypeHelpers::is<T, double>()) return String((double)value, 5);
        if(NativeJITTypeHelpers::is<T, int>()) return String((int)value);
    }
    
    
    template <typename T> void testOperations()
    {
        beginTest("Testing operations for " + NativeJITTypeHelpers::getTypeName<T>());
        
        Random r;
        
        double a = (double)r.nextInt(25) * (r.nextBool() ? 1.0 : -1.0);
        double b = (double)r.nextInt(62) * (r.nextBool() ? 1.0 : -1.0);
        
        ScopedPointer<NativeJITTestCase<T>> test;
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " + " T_B ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Addition", T(), (T)(a+b));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " - " T_B ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Subtraction", T(), (T)(a-b));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " * " T_B ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Multiplication", T(), (T)a*(T)b);
        
        if(NativeJITTypeHelpers::is<T, int>())
        {
            CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " % " T_B ";"));
            EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Modulo", 0, (int)a%(int)b);
        }
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " / " T_B ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Division", T(), (T)(a/b));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " > " T_B " ? " T_1 ":" T_0 ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Conditional", T(), (T)(a>b?1:0));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " > " T_B ") ? " T_1 ":" T_0 ");"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Conditional with Parenthesis", T(), (T)(((T)a>(T)b)?1:0));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " + " T_B ") * " T_A ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Parenthesis", T(), ((T)a+(T)b)*(T)a );
    }
    
    void testFunctionCalls()
    {
        beginTest("Function Calls");
        
        ScopedPointer<NativeJITTestCase<float>> test;
        
        Random r;
        
        const float v = r.nextFloat() * 122.0f * r.nextBool() ? 1.0f : -1.0f;
        
        CREATE_TEST(getTestFunction<float>("return sinf(input);"))
        EXPECT("sinf", v, sinf(v));
        
        CREATE_TEST("float square(float input){return input*input;}; float test(float input){ return square(input);};")
        EXPECT("JIT Function call", v, v*v);
        
        CREATE_TEST("float a(){return 2.0f;}; float b(){ return 4.0f;}; float test(float input){ const float x = input > 50.0f ? a() : b(); return x;};")
        EXPECT("JIT Conditional function call", v, v > 50.0f ? 2.0f : 4.0f);
    }
};

static NativeJITUnitTest njut;

#undef CREATE_TEST
#undef CREATE_TEST_SETUP
#undef EXPECT
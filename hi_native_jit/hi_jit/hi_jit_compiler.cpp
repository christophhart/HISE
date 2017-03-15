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
		unprocessedCode(codeToCompile),
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
		catch (String e)
		{
			errorMessage = e;
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

	String getCode(bool getPreprocessedCode) const
	{
		return getPreprocessedCode ? code : unprocessedCode;
	};

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

	const String unprocessedCode;

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

String NativeJITCompiler::getCode(bool getPreprocessedCode) const
{
	return pimpl->getCode(getPreprocessedCode);
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
    return pimpl != nullptr ? pimpl->globals.size() : 0;
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


void NativeJITScope::setGlobalVariable(const juce::Identifier& id, const juce::var& value)
{
	pimpl->setGlobalVariable(id, value);
}


template <typename ReturnType, typename...ParameterTypes> ReturnType(*NativeJITScope::getCompiledFunction(const Identifier& id))(ParameterTypes...)
{
	int parameterAmount = sizeof...(ParameterTypes);

	try
	{
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
	}
	catch (String e)
	{
		DBG(e);
		return nullptr;
	}
	

	return nullptr;
}


int NativeJITScope::isBufferOverflow(int globalIndex) const
{
	return pimpl->globals[globalIndex]->hasOverflowError();
}


bool NativeJITScope::hasProperty(const Identifier& propertyName) const
{
	for (int i = 0; i < pimpl->globals.size(); i++)
	{
		if (pimpl->globals[i]->id == propertyName)
			return true;
	}

	return false;
}


const var& NativeJITScope::getProperty(const Identifier& propertyName) const
{
	for (int i = 0; i < getNumGlobalVariables(); i++)
	{
		if (getGlobalVariableName(i) == propertyName)
		{
			cachedValues.set(propertyName, getGlobalVariableValue(i));
			return cachedValues[propertyName];
		}
			
	}

	return var();
}


void NativeJITScope::setProperty(const Identifier& propertyName, const var& newValue)
{
	setGlobalVariable(propertyName, newValue);
}


void NativeJITScope::removeProperty(const Identifier& propertyName)
{
	// Can't remove properties from a Native JIT scope
	jassertfalse;
}


bool NativeJITScope::hasMethod(const Identifier& methodName) const
{
	return pimpl->getCompiledBaseFunction(methodName) != nullptr;
}


var NativeJITScope::invokeMethod(Identifier methodName, const var::NativeFunctionArgs& args)
{
	BaseFunction* b = pimpl->getCompiledBaseFunction(methodName);

	const int numArgs = args.numArguments;

	TypeInfo returnType = b->getReturnType();

	TypeInfo parameter1Type = b->getTypeForParameter(0);

	TypeInfo parameter2Type = b->getTypeForParameter(1);

	const var& arg1 = args.numArguments > 0 ? args.arguments[0] : var();
	const var& arg2 = args.numArguments > 1 ? args.arguments[1] : var();

	switch (numArgs)
	{
	case 0:
	{
#define MATCH_TYPE_AND_RETURN(x) if (TYPE_MATCH(x, returnType)) { auto f = pimpl->getCompiledFunction0<x>(methodName); return var(f()); }

		MATCH_TYPE_AND_RETURN(int);
		MATCH_TYPE_AND_RETURN(double);
		MATCH_TYPE_AND_RETURN(float);
		MATCH_TYPE_AND_RETURN(Buffer*);
		MATCH_TYPE_AND_RETURN(BooleanType);

#undef MATCH_TYPE_AND_RETURN
	}
	case 1:
	{

#define MATCH_TYPE_AND_RETURN(rt, p1) if (TYPE_MATCH(rt, returnType) && TYPE_MATCH(p1, parameter1Type)) { auto f = pimpl->getCompiledFunction1<rt, p1>(methodName); return f((p1)arg1); }

		MATCH_TYPE_AND_RETURN(int, int);
		MATCH_TYPE_AND_RETURN(int, double);
		MATCH_TYPE_AND_RETURN(int, float);
		
		MATCH_TYPE_AND_RETURN(double, int);
		MATCH_TYPE_AND_RETURN(double, double);
		MATCH_TYPE_AND_RETURN(double, float);

		MATCH_TYPE_AND_RETURN(float, int);
		MATCH_TYPE_AND_RETURN(float, double);
		MATCH_TYPE_AND_RETURN(float, float);

#undef MATCH_TYPE_AND_RETURN
	}

	}
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

bool NativeJITScope::hasProperty(const Identifier& propertyName) const { return false; }
const var& NativeJITScope::getProperty(const Identifier& propertyName) const { return var(); }
void NativeJITScope::setProperty(const Identifier& propertyName, const var& newValue) { }
void NativeJITScope::removeProperty(const Identifier& propertyName) { }
bool NativeJITScope::hasMethod(const Identifier& methodName) const { return false; }
var NativeJITScope::invokeMethod(Identifier methodName, const var::NativeFunctionArgs& args) { return var(); }


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

#if JUCE_64BIT

template <typename T> class NativeJITTestCase
{
public:
    
    NativeJITTestCase(const String& stringToTest):
    code(stringToTest)
    {
        compiler = new NativeJITCompiler(code);
        scope = compiler->compileAndReturnScope();
    }

	~NativeJITTestCase()
	{
		compiler = nullptr;
		scope = nullptr;
	}
    
    T getResult(T input)
    {
        if(!compiler->wasCompiledOK())
        {
            DBG(compiler->getErrorMessage());
            return T();
        }
        
        auto t = scope->getCompiledFunction<T, T>("test");
        
		if (t != nullptr)
		{
			return t(input);
		}
    };
    
	bool wasOK()
	{
		return compiler->wasCompiledOK();
	}

    void setup()
    {
        if(!compiler->wasCompiledOK())
        {
            DBG(compiler->getErrorMessage());
            return;
        }
        
        auto t = scope->getCompiledFunction<T, T>("setup");
        
		if (t != nullptr)
		{
			t(1.0f);
		}
    }
    
    String code;
    ScopedPointer<NativeJITCompiler> compiler;
    ScopedPointer<NativeJITScope> scope;
    
};


class NativeJITTestModule
{
public:

	void setGlobals(const String& t)
	{
		globals = t;
	}

	void setInitBody(const String& body)
	{
		initBody = body;
	};

	void setPrepareToPlayBody(const String& body)
	{
		prepareToPlayBody = body;
	}

	void setProcessBody(const String& body)
	{
		processBody = body;
	}

	void setCode(const String& code_)
	{
		code = code_;
		compiler = new NativeJITCompiler(code);
	}

	void merge()
	{
		code = String();
		code << globals << "\n";
		code << "void init() {\n\t";
		code << initBody;
		code << "\n};\n\nvoid prepareToPlay(double sampleRate, int blockSize) {\n\t";
		code << prepareToPlayBody;
		code << "\n};\n\nfloat process(float input) {\n\t";
		code << processBody;
		code << "\n};";

		compiler = new NativeJITCompiler(code);
	}

	void createModule()
	{
		module = new NativeJITDspModule(compiler);
	}

	void process(VariantBuffer& b)
	{
		if (compiler != nullptr)
		{
			if (module == nullptr) createModule();

			module->init();
			module->prepareToPlay(44100.0, b.size);

			double start = Time::getMillisecondCounterHiRes();
			module->processBlock(b.buffer.getWritePointer(0), b.size);
			double end = Time::getMillisecondCounterHiRes();

			executionTime = end - start;
		}
	}

	String globals;
	String initBody;
	String prepareToPlayBody;
	String processBody = "return 1.0f;";
	String code;
	ScopedPointer<NativeJITCompiler> compiler = nullptr;

	ScopedPointer<NativeJITDspModule> module;

	double executionTime;

};


#define CREATE_TEST(x) test = new NativeJITTestCase<float>(x);
#define CREATE_TYPED_TEST(x) test = new NativeJITTestCase<T>(x);
#define CREATE_TEST_SETUP(x) test = new NativeJITTestCase<float>(x); test->setup();

#define EXPECT(testName, input, result) expect(test->wasOK(), String(testName) + String(" parsing")); expectEquals<float>(test->getResult(input), result, testName);

#define EXPECT_TYPED(testName, input, result) expectEquals<T>(test->getResult(input), result, testName);

#define CREATE_BOOL_TEST(x) test = new NativeJITTestCase<BooleanType>(String("bool test(bool input){ ") + String(x));


#define EXPECT_BOOL(name, result) expect(test->wasOK(), String(name) + String(" parsing")); expect(test->getResult(0) == result, name);
#define VAR_BUFFER_TEST_SIZE 8192

#define ADD_CODE_LINE(x) code << x << "\n"

#define T_A + getLiteral<T>(a) +
#define T_B + getLiteral<T>(b) +
#define T_1 + getLiteral<T>(1.0) +
#define T_0 + getLiteral<T>(0.0) +

#define START_BENCHMARK const double start = Time::getMillisecondCounterHiRes();
#define STOP_BENCHMARK_AND_LOG const double end = Time::getMillisecondCounterHiRes(); logPerformanceMessage(m->executionTime, end - start);

class NativeJITUnitTest: public UnitTest
{
public:
    
    NativeJITUnitTest(): UnitTest("NativeJIT UnitTest") {}
    
    void runTest() override
    {
		

        testOperations<float>();
        testOperations<double>();
        testOperations<int>();
        
		testCompareOperators<float>();
		testCompareOperators<double>();
		testCompareOperators<int>();

		testBigFunctionBuffer();

		testLogicalOperations();

        testGlobals();
        
        testFunctionCalls();
        testDoubleFunctionCalls();
        
		testTernaryOperator();
		
		testComplexExpressions();

		testDspModules();

		testDynamicObjectProperties();
		testDynamicObjectFunctionCalls();
    }
    
private:
    
	void testLogicalOperations()
	{
		beginTest("Testing logic operations");

		ScopedPointer<NativeJITTestCase<float>> test;

		CREATE_TEST("float x; float test(float i){ return (true && false) ? 12.0f : 4.0f; };");
		EXPECT("And with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x; float test(float i){ return true && false ? 12.0f : 4.0f; };");
		EXPECT("And without parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x; float test(float i){ return true && true && false ? 12.0f : 4.0f; };");
		EXPECT("Two Ands", 2.0f, 4.0f);
		
		CREATE_TEST("float x; float test(float i){ return true || false ? 12.0f : 4.0f; };");
		EXPECT("Or", 2.0f, 12.0f);

		CREATE_TEST("float x; float test(float i){ return (false || false) && true  ? 12.0f : 4.0f; };");
		EXPECT("Or with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x; float test(float i){ return false || false && true ? 12.0f : 4.0f; };");
		EXPECT("Or with parenthesis", 2.0f, 4.0f);

	}

    void testGlobals()
    {
        beginTest("Testing Global variables");
        
        ScopedPointer<NativeJITTestCase<float>> test;
        
        
		CREATE_TEST("float x; float test(float i){ x=7.0f; return x; };");
		EXPECT("Global float", 2.0f, 7.0f);

        CREATE_TEST("float x; float test(float i){ x=-7.0f; return x; };");
        EXPECT("Global negative float", 2.0f, -7.0f);
        
        CREATE_TEST("float x=-7.0f; float test(float i){ return x; };");
        EXPECT("Global negative float definition", 2.0f, -7.0f);
		
        CREATE_TEST_SETUP("double x = 2.0; float setup(float i){x = 26.0; return 1.0f; }; float test(float i){ return (float)x;};");
        EXPECT("Global set & get from different functions", 2.0f, 26.0f);
        
		CREATE_TEST("bool x; float test(float i){ x=true; return x ? 1.0f : 6.0f; };");
		EXPECT("Global bool", 2.0f, 1.0f);

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
		if (NativeJITTypeHelpers::is<T, BooleanType>()) return "bool";
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
		if (NativeJITTypeHelpers::is<T, float>())
		{
			String rs = String((float)value);

			return rs.containsChar('.') ? rs + "f" : rs + ".f";
		}
        if(NativeJITTypeHelpers::is<T, double>()) return String((double)value, 5);
        if(NativeJITTypeHelpers::is<T, int>()) return String((int)value);
    }
    
    template <typename T> String getGlobalDefinition(double value)
    {
        const String valueString = getLiteral<T>(value);
        
        return getTypeName<T>() + " x = " + valueString + ";";
    }
    
    template <typename T> void testOperations()
    {
        beginTest("Testing operations for " + NativeJITTypeHelpers::getTypeName<T>());
        
        Random r;
        
        double a = (double)r.nextInt(25) * (r.nextBool() ? 1.0 : -1.0);
        double b = (double)r.nextInt(62) * (r.nextBool() ? 1.0 : -1.0);
        
		if (b == 0.0) b = 55.0;

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
            
            CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x %= " T_B "; return x;};");
            EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " %= operator", T(), (int)a % (int)b);
        }
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " / " T_B ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Division", T(), (T)(a/b));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " > " T_B " ? " T_1 ":" T_0 ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Conditional", T(), (T)(a>b?1:0));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " > " T_B ") ? " T_1 ":" T_0 ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Conditional with Parenthesis", T(), (T)(((T)a>(T)b)?1:0));
        
        CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " + " T_B ") * " T_A ";"));
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " Parenthesis", T(), ((T)a+(T)b)*(T)a );
        
        CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x *= " T_B "; return x;};");
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " *= operator", T(), (T)a * (T)b);
        
        CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x /= " T_B "; return x;};");
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " /= operator", T(), (T)a / (T)b);
        
        CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x += " T_B "; return x;};");
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " += operator", T(), (T)a + (T)b);
        
        CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x -= " T_B "; return x;};");
        EXPECT_TYPED(NativeJITTypeHelpers::getTypeName<T>() + " -= operator", T(), (T)a - (T)b);
    }

	template <typename T> void testCompareOperators()
	{
		beginTest("Testing compare operators for " + NativeJITTypeHelpers::getTypeName<T>());

		ScopedPointer<NativeJITTestCase<BooleanType>> test;

		Random r;

		double a = (double)r.nextInt(25) * (r.nextBool() ? 1.0 : -1.0);
		double b = (double)r.nextInt(62) * (r.nextBool() ? 1.0 : -1.0);

		CREATE_BOOL_TEST("return " T_A " > " T_B "; };");
		EXPECT_BOOL("Greater than", a > b);

		CREATE_BOOL_TEST("return " T_A " < " T_B "; };");
		EXPECT_BOOL("Less than", a < b);

		CREATE_BOOL_TEST("return " T_A " >= " T_B "; };");
		EXPECT_BOOL("Greater or equal than", a >= b);

		CREATE_BOOL_TEST("return " T_A " <= " T_B "; };");
		EXPECT_BOOL("Less or equal than", a <= b);

		CREATE_BOOL_TEST("return " T_A " == " T_B "; };");
		EXPECT("Equal", T(), a == b ? 1 : 0);

		CREATE_BOOL_TEST("return " T_A " != " T_B "; };");
		EXPECT("Not equal", T(), a != b ? 1 : 0);

		

	}
    
	void testComplexExpressions()
	{
		beginTest("Testing complex expressions");

		ScopedPointer<NativeJITTestCase<float>> test;

		Random r;

		float input = r.nextFloat() * 125.0f - 80.0f;

		CREATE_TEST("float test(float input){ return (float)(int)(8 > 5 ? (9.0*(double)input) : 1.23+(double)(2.0f*input)); };");
		EXPECT("Complex expression 1", input, (float)(int)(8 > 5 ? (9.0*(double)input) : 1.23 + (double)(2.0f*input)));
	}

    void testFunctionCalls()
    {
        beginTest("Function Calls");
        
        ScopedPointer<NativeJITTestCase<float>> test;
        
        Random r;
        
        const float v = r.nextFloat() * 122.0f * r.nextBool() ? 1.0f : -1.0f;
        
        CREATE_TEST("float square(float input){return input*input;}; float test(float input){ return square(input);};")
        EXPECT("JIT Function call", v, v*v);
        
        CREATE_TEST("float a(){return 2.0f;}; float b(){ return 4.0f;}; float test(float input){ const float x = input > 50.0f ? a() : b(); return x;};")
        EXPECT("JIT Conditional function call", v, v > 50.0f ? 2.0f : 4.0f);

		CREATE_TEST("bool isBigger(int a){return a > 0;}; float test(float input){return isBigger(4) ? 12.0f : 4.0f; };");
		EXPECT("bool function", 2.0f, 12.0f);

		CREATE_TEST("int getIfTrue(bool isTrue){return true ? 1 : 0;}; float test(float input) { return getIfTrue(true) == 1 ? 12.0f : 4.0f; }; ");
		EXPECT("bool parameter", 2.0f, 12.0f);

		CREATE_TEST_SETUP("float x = 0.0f; void calculateX(float newX) { x = newX * 2.0f; }; float setup(float input) { calculateX(4.0f); return 1.0f; }; float test(float input) { return x; };");
		EXPECT("JIT function call with global parameter", 0.0f, 8.0f);

    }

    void testDoubleFunctionCalls()
    {
        beginTest("Double Function Calls");
        
        ScopedPointer<NativeJITTestCase<double>> test;
        
        #define T double
        
        Random r;
        
        const double v = (double)(r.nextFloat() * 122.0f * r.nextBool() ? 1.0f : -1.0f);

        
        CREATE_TYPED_TEST(getTestFunction<double>("return sin(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("sin", v, sin(v));
        CREATE_TYPED_TEST(getTestFunction<double>("return cos(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("cos", v, cos(v));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return tan(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("tan", v, tan(v));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return atan(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("atan", v, atan(v));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return atanh(input);"));
        expectCompileOK(test->compiler);
        EXPECT_TYPED("atanh", v, atanh(v));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return pow(input, 2.0);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("pow", v, pow(v, 2.0));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return sqrt(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("sqrt", v, sqrt(v));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return abs(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("abs", v, abs(v));
        
        CREATE_TYPED_TEST(getTestFunction<double>("return exp(input);"))
        expectCompileOK(test->compiler);
        EXPECT_TYPED("exp", v, exp(v));
        
#undef T

    }
    
	void testTernaryOperator()
	{

		beginTest("Test ternary operator");

		ScopedPointer<NativeJITTestCase<float>> test;

		CREATE_TEST("float test(float input){ return (true ? false : true) ? 12.0f : 4.0f; }; ");
		EXPECT("Nested ternary operator", 0.0f, 4.0f);

	}

	void testDspModules()
	{
		beginTest("Test DSP modules");

		testDspSimpleGain();
		testDspSimpleSine();
		testDspSimpleLP();
		testDspSimpleLP2();
		testDspAdditiveSynthesis();
		testDspSaturator();
		testDspExternalGain();

	}

	void testDspSimpleGain()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		Random r;

		float a = r.nextFloat() * 2.0f - 1.0f;
		float b = r.nextFloat() * 2.0f - 1.0f;

		beginTest("Test Gain with numbers " + String(a) + " and " + String(b));

		m->setGlobals("float x = " + getLiteral<float>(b) + ";");
		m->setProcessBody("return input * x;");
		m->merge();

		VariantBuffer b1 = VariantBuffer(VAR_BUFFER_TEST_SIZE);

		a >> b1;
		m->process(b1);

		VariantBuffer b2 = VariantBuffer(VAR_BUFFER_TEST_SIZE);

		a >> b2;
		
		START_BENCHMARK;
		b2 * b;
		STOP_BENCHMARK_AND_LOG;

		expectCompileOK(m->compiler);

		expectBufferWithSameValues(b1, b2);
	}
	
	void testDspExternalGain()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		Random r;

		float a = r.nextFloat() * 2.0f - 1.0f;
		
		beginTest("Test Setting Gain as DynamicObject property " + String(a));

		m->setGlobals("float x;");
		m->setProcessBody("return input * x;");
		m->merge();

		VariantBuffer b1(VAR_BUFFER_TEST_SIZE);
		VariantBuffer b2(VAR_BUFFER_TEST_SIZE);

		fillBufferWithNoise(b1);
		b1 >> b2;
		
		m->createModule();

		var scope = var(m->module->getScope());
		scope.getDynamicObject()->setProperty("x", a);


		m->process(b1);

		START_BENCHMARK;
		b2 * a;
		STOP_BENCHMARK_AND_LOG;

		expectCompileOK(m->compiler);

		expectBufferWithSameValues(b1, b2);
	}

	void logPerformanceMessage(double jitTime, double nativeTime)
	{
		double percentage = (nativeTime / jitTime) * 100.0;

		const double bufferTimeMS = (double)VAR_BUFFER_TEST_SIZE / 44100.0;

		const double realtime = 1000.0 * bufferTimeMS / jitTime;

		logMessage("JIT Performance: " + String(percentage, 2) + "% of native code (" + String(realtime, 1) + " x realtime performance))");
	}

	void testDspSimpleSine()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		beginTest("Test sine wave generator");

		m->setGlobals("double uptime = 0.0;\ndouble uptimeDelta = 0.1;");
		m->setProcessBody("    const float v = sinf((float)uptime);\n    uptime += uptimeDelta;\n    return v;\n");
		m->merge();

		VariantBuffer b1 = VariantBuffer(VAR_BUFFER_TEST_SIZE);
		b1 >> b1;
		m->process(b1);
		expectCompileOK(m->compiler);

		VariantBuffer b2 = VariantBuffer(VAR_BUFFER_TEST_SIZE);

		START_BENCHMARK
		double uptime = 0.0;
		double uptimeDelta = 0.1;

		for (int i = 0; i < b2.size; i++)
		{
			b2[i] = sinf((float)uptime);
			uptime += uptimeDelta;
		}
		STOP_BENCHMARK_AND_LOG;

		expectBufferWithSameValues(b1, b2);
	}

	void testDspSaturator()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		Random r;

		float a = r.nextFloat();
		

		beginTest("Test Saturation with gain " + String(a));

		m->setGlobals("float saturationAmount;\nfloat k;");

		String initBody;
		
		initBody << "    saturationAmount = 0.8f;\n";
		initBody << "    k = 2.0f * saturationAmount / (1.0f - saturationAmount);\n";
		
		m->setInitBody(initBody);
		


		m->setProcessBody("return (1.0f + k) * input / (1.0f + k * fabsf(input));");
		m->merge();

		VariantBuffer b1(VAR_BUFFER_TEST_SIZE);
		VariantBuffer b2(VAR_BUFFER_TEST_SIZE);

		fillBufferWithNoise(b1);
		b1 >> b2;

		m->process(b1);

		const float saturationAmount = 0.8f;
		const float k = 2 * saturationAmount / (1.0f - saturationAmount);

		START_BENCHMARK;
		for (int i = 0; i < b2.size; i++)
		{
			b2[i] = (1.0f + k) * b2[i] / (1.0f + k * fabsf(b2[i]));
		}
		STOP_BENCHMARK_AND_LOG;

		expectCompileOK(m->compiler);

		expectBufferWithSameValues(b1, b2);
		

		

	}

	void testDspAdditiveSynthesis()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		beginTest("Testing additive synthesis with 4 sine waves");

		m->setGlobals("double uptime = 0.0;\ndouble uptimeDelta = 0.1;");
		
		
		String body;
		body << "    const float uptimeFloat = (float)uptime;\n";
		body << "    const float v1 = 1.0f * sinf(uptimeFloat);\n";
		body << "    const float v2 = 0.8f * sinf(2.0f * uptimeFloat);\n";
		body << "    const float v3 = 0.6f * sinf(3.0f * uptimeFloat);\n";
		body << "    const float v4 = 0.4f * sinf(4.0f * uptimeFloat);\n";
		body << "    uptime += uptimeDelta;\n";
		body << "    return v1 + v2 + v3 + v4;";

		m->setProcessBody(body);
		m->merge();

		VariantBuffer b1 = VariantBuffer(VAR_BUFFER_TEST_SIZE);
		b1 >> b1;
		m->process(b1);
		expectCompileOK(m->compiler);

		VariantBuffer b2 = VariantBuffer(VAR_BUFFER_TEST_SIZE);

		START_BENCHMARK;
		double uptime = 0.0;
		double uptimeDelta = 0.1;

		for (int i = 0; i < b2.size; i++)
		{
			const float uptimeFloat = (float)uptime;
			const float v1 = 1.0f * sinf(uptimeFloat);
			const float v2 = 0.8f * sinf(2.0f * uptimeFloat);
			const float v3 = 0.6f * sinf(3.0f * uptimeFloat);
			const float v4 = 0.4f * sinf(4.0f * uptimeFloat);
			uptime += uptimeDelta;
			b2[i] = v1 + v2 + v3 + v4;
		}
		STOP_BENCHMARK_AND_LOG;

		expectBufferWithSameValues(b1, b2);
	}

	void testDspSimpleLP()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		beginTest("Test Simple LP");

		m->setGlobals("float a; \nfloat invA; \nfloat lastValue = 0.0f;");
		m->setInitBody("    a = 0.95f;\n    invA = 1.0f - a;");
		m->setProcessBody("    const float thisValue = a * lastValue + invA * input;\n    lastValue = thisValue;\n    return thisValue;");
		m->merge();

		VariantBuffer b1 = VariantBuffer(VAR_BUFFER_TEST_SIZE);
		VariantBuffer b2 = VariantBuffer(VAR_BUFFER_TEST_SIZE);

		fillBufferWithNoise(b1);

		b1 >> b2;

		m->process(b1);
		expectCompileOK(m->compiler);

		START_BENCHMARK;

		float a = 0.95f;
		float invA = 1.0f - a;
		float lastValue = 0.0f;

		for (int i = 0; i < b2.size; i++)
		{
			const float thisValue = a * lastValue + invA * b2[i];
			lastValue = thisValue;
			b2[i] = thisValue;
		}
		STOP_BENCHMARK_AND_LOG;

		expectBufferWithSameValues(b1, b2);
	}

	void testDspSimpleLP2()
	{
		ScopedPointer<NativeJITTestModule> m = new NativeJITTestModule();

		beginTest("Test Simple LP2");

		String code;

		ADD_CODE_LINE("float _g0 = 0.0f;");
		ADD_CODE_LINE("float _y0 = 0.0f;");
		ADD_CODE_LINE("float freq = 1.4f;");
		ADD_CODE_LINE("");
		ADD_CODE_LINE("void init()");
		ADD_CODE_LINE("{");
		ADD_CODE_LINE("    ");
		ADD_CODE_LINE("};");
		ADD_CODE_LINE("");
		ADD_CODE_LINE("void prepareToPlay(double sampleRate, int blockSize)");
		ADD_CODE_LINE("{");
		ADD_CODE_LINE("    freq = expf(-0.5f);");
		ADD_CODE_LINE("};");
		ADD_CODE_LINE("");
		ADD_CODE_LINE("float process(float input)");
		ADD_CODE_LINE("{");
		ADD_CODE_LINE("	const float g1 = freq;");
		ADD_CODE_LINE("	const float y1 = _g0 * g1 * (_y0 - input) + input;");
		ADD_CODE_LINE("	_g0 = g1;");
		ADD_CODE_LINE("	_y0 = y1;");
		ADD_CODE_LINE("	return y1;");
		ADD_CODE_LINE("};");

		m->setCode(code);

		VariantBuffer b1 = VariantBuffer(VAR_BUFFER_TEST_SIZE);
		VariantBuffer b2 = VariantBuffer(VAR_BUFFER_TEST_SIZE);

		fillBufferWithNoise(b1);

		b1 >> b2;

		m->process(b1);
		expectCompileOK(m->compiler);

		float _g0 = 0.0f;
		float _y0 = 0.0f;
		float freq = expf(-0.5f);

		START_BENCHMARK;

		for(int i = 0; i < b2.size; i++)
		{

			const float g1 = freq;
			const float y1 = _g0 * g1 * (_y0 - b2[i]) + b2[i];
			_g0 = g1;
			_y0 = y1;
			b2[i] = y1;
		};

		STOP_BENCHMARK_AND_LOG;

		expectBufferWithSameValues(b1, b2);
	}

	void fillBufferWithNoise(VariantBuffer& b)
	{
		Random r;

		for (int i = 0; i < b.size; i++)
		{
			b[i] = r.nextFloat() * 2.0f - 1.0f;
		}
	}

	void expectCompileOK(NativeJITCompiler* compiler)
	{
		expect(compiler->wasCompiledOK(), compiler->getErrorMessage() + "\nFunction Code:\n\n" + compiler->getCode(true));
	}

	void expectBufferWithSameValues(const VariantBuffer& b, float expectedValue)
	{
		int index = -1;

		for (int i = 0; i < b.size; i++)
		{
			if ((b[i] - expectedValue) > 0.0001f)
			{
				index = i;
				break;
			}
		}

		String message;

		if (index != -1)
		{
			message = "Buffer value mismatch at " + String(index) + ": " + String(b[index]) + ". Expected: " + String(expectedValue);
		}

		expect(index == -1, message);
	}

	void expectBufferWithSameValues(const VariantBuffer& actual, const VariantBuffer &expected)
	{
		if (actual.size != expected.size)
		{
			// Buffer size mismatch
			jassertfalse;
			return;
		}

		int index = -1;

		for (int i = 0; i < actual.size; i++)
		{
			if (fabs(expected[i] - actual[i]) > 0.0001f)
			{
				index = i;
				break;
			}
		}

		String message;

		if (index != -1)
		{
			message = "Buffer value mismatch at " + String(index) + ": " + String(actual[index]) + ". Expected: " + String(expected[index]);
		}

		expect(index == -1, message);
	}

	void testDynamicObjectProperties()
	{
		beginTest("DynamicObject methods for NativeJIT scopes");

		String code;

		ADD_CODE_LINE("int a = -2;");
		ADD_CODE_LINE("float b = 24.0f;");
		ADD_CODE_LINE("double c = 14.0;");
		ADD_CODE_LINE("Buffer buffer;");
		ADD_CODE_LINE("int getA() { return a; };");
		ADD_CODE_LINE("float getB(int input) { return input > 1 ? b : b / 2.0f; };");
		ADD_CODE_LINE("double getC() { return c; };");
		ADD_CODE_LINE("void setA(int newValue) { a = newValue; };");
		ADD_CODE_LINE("void setB(float newValue) { b = newValue; };");
		ADD_CODE_LINE("void setC(double newValue) { c = newValue; };");
		ADD_CODE_LINE("float getBufferValue(int index) { return buffer[index]; };");
		ADD_CODE_LINE("float getInterpolatedValue(float i) {");
		ADD_CODE_LINE("    const int index = (int)i;");
		ADD_CODE_LINE("    const float alpha = i - (float)index;");
		ADD_CODE_LINE("    const float invAlpha = 1.0f - alpha;");
		ADD_CODE_LINE("    return alpha * buffer[index] + invAlpha * buffer[index + 1];");
		ADD_CODE_LINE("};");

		ScopedPointer<NativeJITCompiler> compiler = new NativeJITCompiler(code);

		var scope = compiler->compileAndReturnScope();

		expectCompileOK(compiler);

		expectEquals<int>(scope["a"], -2, "int property");
		expectEquals<float>(scope["b"], 24.0f, "float property");
		expectEquals<double>(scope["c"], 14.0, "double property");

		expectEquals<int>(scope.call("getA"), -2, "int getMethod");
		expectEquals<float>(scope.call("getB", 2), 24.0f, "float getMethod");
		expectEquals<float>(scope.call("getB", 0), 24.0f / 2.0f, "float getMethod");
		expectEquals<double>(scope.call("getC"), 14.0f, "double getMethod");

		scope.getDynamicObject()->setProperty("a", -6);
		scope.getDynamicObject()->setProperty("b", -40.2f);
		scope.getDynamicObject()->setProperty("c", -62.0);

		expectEquals<int>(scope["a"], -6, "int property after change");
		expectEquals<float>(scope["b"], -40.2f, "float property after change");
		expectEquals<double>(scope["c"], -62.0, "double property after change");

		scope.call("setA", 7);
		scope.call("setB", 8);
		scope.call("setC", 9);

		expectEquals<int>(scope["a"], 7, "int property after set method");
		expectEquals<float>(scope["b"], (float)8, "float property after setMethod");
		expectEquals<double>(scope["c"], (double)9, "double property after setMethod");

		VariantBuffer::Ptr bf = new VariantBuffer(250);
		bf->setSample(12, 0.6f);
		scope.getDynamicObject()->setProperty("buffer", var(bf));

		expectEquals<float>(scope.call("getBufferValue", 12), 0.6f, "Buffer Access");

		expectEquals<float>(scope.call("getBufferValue"), 0.0f, "Buffer Access without argument");

		expectEquals<float>(scope.call("getInterpolatedValue", 12.5), 0.3f, "JITted Linear interpolation");

	}



	void testDynamicObjectFunctionCalls()
	{
		beginTest("Calling functions via DynamicObject interface");
	}



	void testBigFunctionBuffer()
	{
		String code;

		ADD_CODE_LINE("int get1() { return 1; };\n");
		ADD_CODE_LINE("int get2() { return 1; };\n");
		ADD_CODE_LINE("int get3() { return 1; };\n");
		ADD_CODE_LINE("int get4() { return 1; };\n");
		ADD_CODE_LINE("int get5() { return 1; };\n");
		ADD_CODE_LINE("int get6() { return 1; };\n");
		ADD_CODE_LINE("int get7() { return 1; };\n");
		ADD_CODE_LINE("int get8() { return 1; };\n");
		ADD_CODE_LINE("int get9() { return 1; };\n");
		ADD_CODE_LINE("float test(float input)\n");
		ADD_CODE_LINE("{\n");
		ADD_CODE_LINE("    const int x = get1() + get2() + get3() + get4() + get5();\n");
		ADD_CODE_LINE("    const int y = get6() + get7() + get8() + get9();\n");
		ADD_CODE_LINE("    return (float)(x+y);\n");
		ADD_CODE_LINE("};");

		ScopedPointer<NativeJITCompiler> compiler = new NativeJITCompiler(code);

		ScopedPointer<NativeJITScope> scope = compiler->compileAndReturnScope();

		expectCompileOK(compiler);

		float result = scope->getCompiledFunction<float, float>("test")(2.0f);

		expectEquals(result, 9.0f, "Testing reallocation of Function buffers");
	}
};

static NativeJITUnitTest njut;

#undef CREATE_TEST
#undef CREATE_TEST_SETUP
#undef EXPECT

#endif
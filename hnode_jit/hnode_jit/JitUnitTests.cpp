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

/** TODO: Add tests for:

- complex function calls
- > 20 variables
- complex nested expressions
- scope related variable resolving (eg float x = 12.0f; { float x = x; };
*/

#pragma once

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

template <typename T, typename ReturnType=T> class HiseJITTestCase
{
public:

	HiseJITTestCase(const String& stringToTest) :
		code(stringToTest)
	{
		compiler = new Compiler(memory);
	}

	~HiseJITTestCase()
	{
		
	}

	ReturnType getResult(T input, ReturnType expected)
	{
		if (!initialised)
			setup();

		if (auto f = func["test"])
		{
			ReturnType v = f.call<ReturnType>(input);

			if (!(v == expected))
			{
				DBG("Failed assembly");
				DBG(compiler->getAssemblyCode());
			}

			return v;
		}

		return ReturnType();
	};

	bool wasOK()
	{
		return compiler->getCompileResult().wasOk();
	}

	void setup()
	{
		DBG(code);

		func = compiler->compileJitObject(code);

		if (!wasOK())
			DBG(compiler->getCompileResult().getErrorMessage());

		if (auto f = func["setup"])
			f.callVoid();

		initialised = true;
	}

	String code;

	bool initialised = false;

	jit::GlobalScope memory;
	ScopedPointer<Compiler> compiler;
	JitObject func;



};

class JITTestModule
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

	}

	void createModule()
	{
	}


	String globals;
	String initBody;
	String prepareToPlayBody;
	String processBody = "return 1.0f;";
	String code;

	double executionTime;
};



#define CREATE_TEST(x) test = new HiseJITTestCase<float>(x);
#define CREATE_TYPED_TEST(x) test = new HiseJITTestCase<T>(x);
#define CREATE_TEST_SETUP(x) test = new HiseJITTestCase<float>(x); test->setup();

#define EXPECT(testName, input, result) expect(test->wasOK(), String(testName) + String(" parsing")); expectEquals<float>(test->getResult(input, result), result, testName);

#define EXPECT_TYPED(testName, input, result) expect(test->wasOK(), String(testName) + String(" parsing")); expectEquals<T>(test->getResult(input, result), result, testName);

#define GET_TYPE(T) Types::Helpers::getTypeNameFromTypeId<T>()

#define CREATE_BOOL_TEST(x) test = new HiseJITTestCase<int>(String("int test(int input){ ") + String(x));


#define EXPECT_BOOL(name, result) expect(test->wasOK(), String(name) + String(" parsing")); expect(test->getResult(0, result) == result, name);
#define VAR_BUFFER_TEST_SIZE 8192

#define ADD_CODE_LINE(x) code << x << "\n"

#define T_A + getLiteral<T>(a) +
#define T_B + getLiteral<T>(b) +
#define T_1 + getLiteral<T>(1.0) +
#define T_0 + getLiteral<T>(0.0) +

#define START_BENCHMARK const double start = Time::getMillisecondCounterHiRes();
#define STOP_BENCHMARK_AND_LOG const double end = Time::getMillisecondCounterHiRes(); logPerformanceMessage(m->executionTime, end - start);




class HiseJITUnitTest : public UnitTest
{
public:

	HiseJITUnitTest() : UnitTest("HiseJIT UnitTest") {}

	void runTest() override
	{
		
		

		testParser();

		testSimpleIntOperations();
		

		testOperations<float>();
		testOperations<double>();
		testOperations<int>();
		
		testCompareOperators<double>();
		testCompareOperators<int>();
		testCompareOperators<float>();
		testTernaryOperator();
		testComplexExpressions();
		testGlobals();
		testFunctionCalls();
		testDoubleFunctionCalls();
		testBigFunctionBuffer();
		testLogicalOperations();


		testScopes();

		testBlocks();

		testEventSetters();
		testEvents();


		testDspModules();

		//testDynamicObjectProperties();
		//testDynamicObjectFunctionCalls();
	}


private:

	void expectCompileOK(Compiler* compiler)
	{
		auto r = compiler->getCompileResult();

		expect(r.wasOk(), r.getErrorMessage() + "\nFunction Code:\n\n" + compiler->getLastCompiledCode());
	}

	void expectAllFunctionsDefined(JITTestModule* m)
	{
	}
	void testEventSetters()
	{
		using T = HiseEvent;

		ScopedPointer<HiseJITTestCase<T>> test;
		HiseEvent e(HiseEvent::Type::NoteOn, 49, 127, 10);

		CREATE_TYPED_TEST("event test(event in){in.setNoteNumber(80); return in; }");
	}

	void testBlocks()
	{
		using T = block;

		beginTest("Testing blocks");

		ScopedPointer<HiseJITTestCase<T>> test;
		AudioSampleBuffer b(1, 512);
		b.clear();
		block bl(b.getWritePointer(0), 512);
		block bl2(b.getWritePointer(0), 512);

		CREATE_TYPED_TEST("block test(int in2, block in){ return in; };");

		test->setup();

		auto rb = test->func["test"].call<block>(2, bl);

		expectEquals<uint64>(reinterpret_cast<uint64>(bl.getData()), reinterpret_cast<uint64>(rb.getData()), "simple block return");

		bl[0] = 0.86f;
		bl2[128] = 0.92f;

		CREATE_TYPED_TEST("float test(block in, block in2){ return in[0] + in2[128]; };");

		test->setup();

		auto rb2 = test->func["test"].call<float>(bl, bl2);

		expectEquals<float>(rb2, 0.86f + 0.92f, "Adding two block values");

		CREATE_TYPED_TEST("float test(block in){ in[1] = 124.0f; return 1.0f; };");

		test->setup();
		test->func["test"].call<float>(bl);

		expectEquals<float>(bl[1], 124.0f, "Setting block value");

		expect(false, "This test crashes, so fix it once we need block iteration...");

#if 0
		CREATE_TYPED_TEST("void test(block in){ loop_block(sample: in){ sample = 2.0f; }}");
		test->setup();

		auto f = test->func["test"];
		
		f.callVoid(bl);


		for (int i = 0; i < bl.size(); i++)
		{
			expectEquals<float>(bl[i], 2.0f, "Setting all values");
		}
#endif

	}

	void testEvents()
	{
		beginTest("Testing HiseEvents in JIT");

		using Event2IntTest = HiseJITTestCase<HiseEvent, int>;

		HiseEvent testEvent = HiseEvent(HiseEvent::Type::NoteOn, 59, 127, 1);
		HiseEvent testEvent2 = HiseEvent(HiseEvent::Type::NoteOn, 61, 127, 1);

		ScopedPointer<Event2IntTest> test;

		test = new Event2IntTest("int test(event in){ return in.getNoteNumber(); }");
		EXPECT("getNoteNumber", testEvent, 59);

		test = new Event2IntTest("int test(event in){ in.setNoteNumber(40); return in.getNoteNumber(); }");
		EXPECT("setNoteNumber", testEvent, 40);

		test = new Event2IntTest("int test(event in){ return in.getNoteNumber() > 64 ? 17 : 13; }");
		EXPECT("getNoteNumber arithmetic", testEvent, 13);

		test = new Event2IntTest("int test(event in1, event in2){ return in1.getNoteNumber() > in2.getNoteNumber() ? 17 : 13; }");

	}

	void testParser()
	{
		beginTest("Testing Parser");

		ScopedPointer<HiseJITTestCase<int>> test;

		test = new HiseJITTestCase<int>("float x = 1.0f;;");
		expectCompileOK(test->compiler);
	}

	void testSimpleIntOperations()
	{
		beginTest("Testing simple integer operations");

		ScopedPointer<HiseJITTestCase<int>> test;

		test = new HiseJITTestCase<int>("int test(int input){ int x = 6; return x;};");
		expectCompileOK(test->compiler);
		EXPECT("local int variable", 0, 6);

		test = new HiseJITTestCase<int>("int x = 0; int test(int input){ x = input; return x;};");
		expectCompileOK(test->compiler);
		EXPECT("int assignment", 6, 6);

		test = new HiseJITTestCase<int>("int x = 2; int test(int input){ x = -5; return x;};");
		expectCompileOK(test->compiler);
		EXPECT("negative int assignment", 0, -5);

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { x++; return x; }");
		expectCompileOK(test->compiler);
		EXPECT("post int increment", 0, 13);

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { return x++; }");
		expectCompileOK(test->compiler);
		EXPECT("post increment as return", 0, 12);

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { ++x; return x; }");
		expectCompileOK(test->compiler);
		EXPECT("post int increment", 0, 13);

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { return ++x; }");
		expectCompileOK(test->compiler);
		EXPECT("post increment as return", 0, 13);
	}

	void testScopes()
	{
		beginTest("Testing variable scopes");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float test(float in) {{return 2.0f;}}; ");
		EXPECT("Empty scope", 12.0f, 2.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) {{ float x = x; x *= 1000.0f; } return x; }");

		EXPECT("Overwrite with local variable", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) {{ x *= 1000.0f; } return x; }");

		EXPECT("Change global in sub scope", 12.0f, 1000.0f);

		CREATE_TEST("float test(float input){ float x1 = 12.0f; float x2 = 12.0f; float x3 = 12.0f; float x4 = 12.0f; float x5 = 12.0f; float x6 = 12.0f; float x7 = 12.0f;float x8 = 12.0f; float x9 = 12.0f; float x10 = 12.0f; float x11 = 12.0f; float x12 = 12.0f; return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10 + x11 + x12; }");

		EXPECT("12 variables", 12.0f, 144.0f);

		CREATE_TEST("float test(float in) { float x = 8.0f; float y = 0.0f; { float x = x + 9.0f; y = x; } return y; }");

		EXPECT("Save scoped variable to local variable", 12.0f, 17.0f);
	}

	void testLogicalOperations()
	{
		beginTest("Testing logic operations");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float x = 0.0f; float test(float i){ return (true && false) ? 12.0f : 4.0f; };");
		EXPECT("And with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return true && false ? 12.0f : 4.0f; };");
		EXPECT("And without parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return true && true && false ? 12.0f : 4.0f; };");
		EXPECT("Two Ands", 2.0f, 4.0f);

		CREATE_TEST("float x = 1.0f; float test(float i){ return true || false ? 12.0f : 4.0f; };");
		EXPECT("Or", 2.0f, 12.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return (false || false) && true  ? 12.0f : 4.0f; };");
		EXPECT("Or with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return false || false && true ? 12.0f : 4.0f; };");
		EXPECT("Or with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ 0 && change(); return x;}");
		EXPECT("Short circuit of && operation", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ 1 || change(); return x;}");
		EXPECT("Short circuit of || operation", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ int c = change(); 0 && c; return x;}");
		EXPECT("Don't short circuit variable expression with &&", 12.0f, 5.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ int c = change(); 1 || c; return x;}");
		EXPECT("Don't short circuit variable expression with ||", 12.0f, 5.0f);

		auto ce = [](float input)
		{
			return (12.0f > input) ? 
				   (input * 2.0f) : 
				   (input >= 20.0f && (float)(int)input != input ? 5.0f : 19.0f);
		};

		Random r;
		float value = r.nextFloat() * 24.0f;

		CREATE_TEST("float test(float input){return (12.0f > input) ? (input * 2.0f) : (input >= 20.0f && (float)(int)input != input ? 5.0f : 19.0f);}");
		EXPECT("Complex expression", value, ce(value));
	}

	void testGlobals()
	{
		beginTest("Testing Global variables");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float x = 0.0f; float test(float i){ x=7.0f; return x; };");
		EXPECT("Global float", 2.0f, 7.0f);

		CREATE_TEST("float x=0.0f; float test(float i){ x=-7.0f; return x; };");
		EXPECT("Global negative float", 2.0f, -7.0f);

		CREATE_TEST("float x=-7.0f; float test(float i){ return x; };");
		EXPECT("Global negative float definition", 2.0f, -7.0f);

		CREATE_TEST_SETUP("double x = 2.0; void setup(){x = 26.0; }; float test(float i){ return (float)x;};");
		EXPECT("Global set & get from different functions", 2.0f, 26.0f);
		
		CREATE_TEST("float x=2.0f;float test(float i){return x*2.0f;};")
		EXPECT("Global float with operation", 2.0f, 4.0f)

		CREATE_TEST("int x=2;float test(float i){return (float)x;};")
		EXPECT("Global cast", 2.0f, 2.0f)

#if INCLUDE_BUFFERS
		CREATE_TEST_SETUP("Buffer b(11);float setup(float i){b[10]=3.0f;return 1.0f;};float test(float i){return b[10];};")
		EXPECT("Buffer test", 2.0f, 3.0f)
#endif

		CREATE_TEST("int c=0;float test(float i){c+=1;c+=1;c+=1;return (float)c;};")
		EXPECT("Incremental", 0.0f, 3.0f)

#if INCLUDE_BUFFERS
		CREATE_TEST("Buffer b(2); float test(float i){ return b[7];};")
		EXPECT("Buffer Overflow", 0.0f, 0.0f)
#endif
	}

	template <typename T> String getTypeName() const
	{
		return Types::Helpers::getTypeNameFromTypeId<T>();
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
		VariableStorage v(Types::Helpers::getTypeFromTypeId<T>(), value);

		return Types::Helpers::getCppValueString(v);
	}

	template <typename T> String getGlobalDefinition(double value)
	{
		const String valueString = getLiteral<T>(value);

		return getTypeName<T>() + " x = " + valueString + ";";
	}



	template <typename T> void testOperations()
	{
		auto type = Types::Helpers::getTypeFromTypeId<T>();

		beginTest("Testing operations for " + Types::Helpers::getTypeName(type));

		Random r;

		double a = (double)r.nextInt(25) *(r.nextBool() ? 1.0 : -1.0);
		double b = (double)r.nextInt(62) *(r.nextBool() ? 1.0 : -1.0);

		if (b == 0.0) b = 55.0;

		ScopedPointer<HiseJITTestCase<T>> test;

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " + " T_B ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Addition", T(), (T)(a + b));

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " - " T_B ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Subtraction", T(), (T)(a - b));

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " * " T_B ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Multiplication", T(), (T)a*(T)b);

		if (Types::Helpers::getTypeFromTypeId<T>() == Types::Integer)
		{
			double ta = a;
			double tb = b;

			a = abs(a);
			b = abs(b);

			CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " % " T_B ";"));
			EXPECT_TYPED(GET_TYPE(T) + " Modulo", 0, (int)a % (int)b);

			CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x %= " T_B "; return x;};");
			EXPECT_TYPED(GET_TYPE(T) + " %= operator", T(), (int)a % (int)b);

			a = ta;
			b = tb;
		}

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " / " T_B ";"));

		EXPECT_TYPED(GET_TYPE(T) + " Division", T(), (T)(a / b));

#if INCLUDE_CONDITIONALS
		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " > " T_B " ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Conditional", T(), (T)(a > b ? 1 : 0));

		CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " > " T_B ") ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Conditional with Parenthesis", T(), (T)(((T)a > (T)b) ? 1 : 0));


		CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " + " T_B ") * " T_A ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Parenthesis", T(), ((T)a + (T)b)*(T)a);
#endif

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x *= " T_B "; return x;};");
		EXPECT_TYPED(GET_TYPE(T) + " *= operator", T(), (T)a * (T)b);

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x /= " T_B "; return x;};");
		EXPECT_TYPED(GET_TYPE(T) + " /= operator", T(), (T)a / (T)b);

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x += " T_B "; return x;};");
		EXPECT_TYPED(GET_TYPE(T) + " += operator", T(), (T)a + (T)b);

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x -= " T_B "; return x;};");
		EXPECT_TYPED(GET_TYPE(T) + " -= operator", T(), (T)a - (T)b);
	}

	template <typename T> void testCompareOperators()
	{
		beginTest("Testing compare operators for " + GET_TYPE(T));

		ScopedPointer<HiseJITTestCase<BooleanType>> test;

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

		ScopedPointer<HiseJITTestCase<float>> test;

		Random r;

		CREATE_TEST("float test(float input){ return (float)input * input; }");
		EXPECT("Unnecessary cast", 12.0f, 144.0f);

		float input = r.nextFloat() * 125.0f - 80.0f;

		CREATE_TEST("float test(float input){ return (float)(int)(8 > 5 ? (9.0*(double)input) : 1.23+ (double)(2.0f*input)); };");
		EXPECT("Complex expression 1", input, (float)(int)(8 > 5 ? (9.0*(double)input) : 1.23 + (double)(2.0f*input)));

		input = -1.0f * r.nextFloat() * 2.0f;

		CREATE_TEST("float test(float input){ return -1.5f * Math.abs(input) + 2.0f * Math.abs(input - 1.0f);}; ");
		EXPECT("Complex expression 2", input, -1.5f * fabsf(input) + 2.0f * fabsf(input - 1.0f));

		String s;
		NewLine nl;

		s << "float test(float in)" << nl;
		s << "{" << nl;
		s << "	float x1 = Math.pow(in, 3.2f);" << nl;
		s << "	float x2 = Math.sin(x1 * in) - Math.abs(Math.cos(15.0f - in));" << nl;
		s << "	float x3 = 124.0f * Math.max((float)1.0, in);" << nl;
		s << "	x3 += x1 + x2 > 12.0f ? x1 : (float)130 + x2;" << nl;
		s << "	return x3;" << nl;
		s << "}" << nl;

		auto sExpected = [](float in)
		{
			float x1 = hmath::pow(in, 3.2f);
			float x2 = hmath::sin(x1 * in) - hmath::abs(hmath::cos(15.0f - in));
			float x3 = 124.0f * hmath::max((float)1.0, in);
			x3 += x1 + x2 > 12.0f ? x1 : (float)130 + x2;
			return x3;
		};

		CREATE_TEST(s);

		float sValue = r.nextFloat() * 100.0f;

		EXPECT("Complex Expression 3", sValue, sExpected(sValue));
	}

	void testFunctionCalls()
	{
		beginTest("Function Calls");

		ScopedPointer<HiseJITTestCase<float>> test;

		Random r;

		const float v = r.nextFloat() * 122.0f * r.nextBool() ? 1.0f : -1.0f;

		CREATE_TEST("float square(float input){return input*input;}; float test(float input){ return square(input);};")
			EXPECT("JIT Function call", v, v*v);

#if INCLUDE_CONDITIONALS
		CREATE_TEST("float a(){return 2.0f;}; float b(){ return 4.0f;}; float test(float input){ const float x = input > 50.0f ? a() : b(); return x;};")
			EXPECT("JIT Conditional function call", v, v > 50.0f ? 2.0f : 4.0f);


		CREATE_TEST("int isBigger(int a){return a > 0;}; float test(float input){return isBigger(4) ? 12.0f : 4.0f; };");
		EXPECT("int function", 2.0f, 12.0f);

		CREATE_TEST("int getIfTrue(int isTrue){return true ? 1 : 0;}; float test(float input) { return getIfTrue(true) == 1 ? 12.0f : 4.0f; }; ");
		EXPECT("int parameter", 2.0f, 12.0f);
#endif

		String x;
		NewLine nl;

		x << "float x = 0.0f;" << nl;
		x << "void calculateX(float newX) {" << nl;
		x << "x = newX * 2.0f;" << nl;
		x << "};" << nl;
		x << "void setup() {" << nl;
		x << "calculateX(4.0f);" << nl;
		x << "}; float test(float input) { return x; }; ";

		CREATE_TEST_SETUP(x);
		EXPECT("JIT function call with global parameter", 0.0f, 8.0f);

		CREATE_TEST("int sumThemAll(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8){ return i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8; } float test(float in) { return (float)sumThemAll(1, 2, 3, 4, 5, 6, 7, 8); }");

		EXPECT("Function call with 8 parameters", 20.0f, 36.0f);

	}

	void testDoubleFunctionCalls()
	{
		beginTest("Double Function Calls");

		ScopedPointer<HiseJITTestCase<double>> test;

#define T double

		Random r;

		const double v = (double)(r.nextFloat() * 122.0f * r.nextBool() ? 1.0f : -1.0f);


		CREATE_TYPED_TEST(getTestFunction<double>("return Math.sin(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("sin", v, sin(v));
		CREATE_TYPED_TEST(getTestFunction<double>("return Math.cos(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("cos", v, cos(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.tan(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("tan", v, tan(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.atan(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("atan", v, atan(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.atanh(input);"));
		expectCompileOK(test->compiler);
		EXPECT_TYPED("atanh", v, atanh(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.pow(input, 2.0);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("pow", v, pow(v, 2.0));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.sqrt(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("sqrt", fabs(v), sqrt(fabs(v))); // No negative square root

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.abs(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("fabs", v, fabs(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.exp(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("exp", v, exp(v));

#undef T

	}

	void testTernaryOperator()
	{

		beginTest("Test ternary operator");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float test(float input){ return (true ? false : true) ? 12.0f : 4.0f; }; ");
		EXPECT("Nested ternary operator", 0.0f, 4.0f);

	}


	void testDspModules()
	{
#if INCLUDE_BUFFERS
		beginTest("Test DSP modules");

		testDspSimpleGain();
		testDspSimpleSine();
		testDspSimpleLP();
		testDspSimpleLP2();
		testDspAdditiveSynthesis();
		testDspSaturator();
		testDspExternalGain();
		testSimpleDelay();
#endif

	}

#if INCLUDE_BUFFERS
	void testDspSimpleGain()
	{
		ScopedPointer<JITTestModule> m = new JITTestModule();

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

		expectAllFunctionsDefined(m);

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
		ScopedPointer<JITTestModule> m = new JITTestModule();

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

		expectAllFunctionsDefined(m);

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
		ScopedPointer<JITTestModule> m = new JITTestModule();

		beginTest("Test sine wave generator");

		m->setGlobals("double uptime = 0.0;\ndouble uptimeDelta = 0.1;");
		m->setProcessBody("    const float v = sinf((float)uptime);\n    uptime += uptimeDelta;\n    return v;\n");
		m->merge();

		VariantBuffer b1 = VariantBuffer(VAR_BUFFER_TEST_SIZE);
		b1 >> b1;
		m->process(b1);
		expectCompileOK(m->compiler);
		expectAllFunctionsDefined(m);

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
		ScopedPointer<JITTestModule> m = new JITTestModule();


		beginTest("Test Saturation");

		String code;

		ADD_CODE_LINE("float k;\n");
		ADD_CODE_LINE("float saturationAmount;\n");
		ADD_CODE_LINE("\n");
		ADD_CODE_LINE("void init()\n");
		ADD_CODE_LINE("{\n");
		ADD_CODE_LINE("	saturationAmount = 0.8f;\n");
		ADD_CODE_LINE("	k = 2.0f * saturationAmount / (1.0f - saturationAmount);\n");
		ADD_CODE_LINE("};\n");
		ADD_CODE_LINE("\n");
		ADD_CODE_LINE("void prepareToPlay(double sampleRate, int blockSize) {};\n");
		ADD_CODE_LINE("\n");
		ADD_CODE_LINE("float process(float input)\n");
		ADD_CODE_LINE("{\n");
		ADD_CODE_LINE("	return (1.0f + k) * input / (1.0f + k * fabsf(input));\n");
		ADD_CODE_LINE("};");

		m->setCode(code);

		VariantBuffer b1(VAR_BUFFER_TEST_SIZE);
		VariantBuffer b2(VAR_BUFFER_TEST_SIZE);

		fillBufferWithNoise(b1);
		b1 >> b2;

		m->process(b1);

		expectCompileOK(m->compiler);
		expectAllFunctionsDefined(m);

		const float saturationAmount = 0.8f;
		const float k = 2 * saturationAmount / (1.0f - saturationAmount);

		START_BENCHMARK;
		for (int i = 0; i < b2.size; i++)
		{
			b2[i] = (1.0f + k) * b2[i] / (1.0f + k * fabsf(b2[i]));
		}
		STOP_BENCHMARK_AND_LOG;

		expectBufferWithSameValues(b1, b2);
	}

	void testSimpleDelay()
	{
		ScopedPointer<JITTestModule> m = new JITTestModule();

		beginTest("Test Simple Delay");

		String code;

		ADD_CODE_LINE("#define SAFE 1");
		ADD_CODE_LINE("const Buffer b(8192);");
		ADD_CODE_LINE("int readIndex = 0;");
		ADD_CODE_LINE("int writeIndex = 1000;");
		ADD_CODE_LINE("void init() {};");
		ADD_CODE_LINE("void prepareToPlay(double sampleRate, int blockSize) {};");
		ADD_CODE_LINE("float process(float input)");
		ADD_CODE_LINE("{");
		ADD_CODE_LINE("    b[(readIndex + 300) % 8192] = input;");
		ADD_CODE_LINE("    const float v = b[readIndex];");
		ADD_CODE_LINE("	   ++readIndex;");
		ADD_CODE_LINE("    return v;");
		ADD_CODE_LINE("};");


		m->setCode(code);

		VariantBuffer b1(VAR_BUFFER_TEST_SIZE);
		VariantBuffer b2(VAR_BUFFER_TEST_SIZE);

		fillBufferWithNoise(b1);
		b1 >> b2;

		m->process(b1);

		expectCompileOK(m->compiler);
		expectAllFunctionsDefined(m);

		float b[8192];
		int readIndex = 0;
		FloatVectorOperations::clear(b, 8192);

		START_BENCHMARK;
		for (int i = 0; i < b2.size; i++)
		{
			b[(readIndex + 300) % 8192] = b2[i];
			b2[i] = b[readIndex];
			++readIndex;
		}
		STOP_BENCHMARK_AND_LOG;

		expectBufferWithSameValues(b1, b2);
	}

	void testDspAdditiveSynthesis()
	{
		ScopedPointer<JITTestModule> m = new JITTestModule();

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
		expectAllFunctionsDefined(m);

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
		ScopedPointer<JITTestModule> m = new JITTestModule();

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
		expectAllFunctionsDefined(m);

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
		ScopedPointer<JITTestModule> m = new JITTestModule();

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
		expectAllFunctionsDefined(m);

		float _g0 = 0.0f;
		float _y0 = 0.0f;
		float freq = expf(-0.5f);

		START_BENCHMARK;

		for (int i = 0; i < b2.size; i++)
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
#endif

	void testDynamicObjectProperties()
	{
#if 0
		beginTest("DynamicObject methods for HiseJIT scopes");

		String code;

		ADD_CODE_LINE("int a = -2;");
		ADD_CODE_LINE("float b = 24.0f;");
		ADD_CODE_LINE("double c = 14.0;");

#if INCLUDE_BUFFERS
		ADD_CODE_LINE("Buffer buffer;");
#endif

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

		ScopedPointer<JITCompiler> compiler = new JITCompiler(code);

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

#if INCLUDE_BUFFERS
		VariantBuffer::Ptr bf = new VariantBuffer(250);
		bf->setSample(12, 0.6f);
		scope.getDynamicObject()->setProperty("buffer", var(bf));


		expectEquals<float>(scope.call("getBufferValue", 12), 0.6f, "Buffer Access");

		expectEquals<float>(scope.call("getBufferValue"), 0.0f, "Buffer Access without argument");

		expectEquals<float>(scope.call("getInterpolatedValue", 12.5), 0.3f, "JITted Linear interpolation");
#endif

#endif
	}



	void testDynamicObjectFunctionCalls()
	{
		beginTest("Calling functions via DynamicObject interface");
	}

	void testBigFunctionBuffer()
	{
		beginTest("Testing big function buffer");

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
		ADD_CODE_LINE("    const int x = get1() + get2() + get3() + get4() + get5(); \n");
		ADD_CODE_LINE("    const int y = get6() + get7() + get8() + get9();\n");
		ADD_CODE_LINE("    return (float)(x+y);\n");
		ADD_CODE_LINE("};");

		GlobalScope memory;

		ScopedPointer<Compiler> compiler = new Compiler(memory);

		auto scope = compiler->compileJitObject(code);

		expectCompileOK(compiler);

		auto data = scope["test"];
		float result = data.call<float>(2.0f);

		expectEquals(result, 9.0f, "Testing reallocation of Function buffers");
	}
};


static HiseJITUnitTest njut;


#undef CREATE_TEST
#undef CREATE_TEST_SETUP
#undef EXPECT


}}
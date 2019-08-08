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

namespace snex {
namespace jit {
using namespace juce;
using namespace asmjit;

#pragma warning( push )
#pragma warning( disable : 4244)

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
			ReturnType v = f.template call<ReturnType>(input);

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
		

		func = compiler->compileJitObject(code);

#if JUCE_DEBUG
		if (!wasOK())
		{
			DBG(code);
			DBG(compiler->getCompileResult().getErrorMessage());
		}
#endif

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
		// Begin of standard tests

		testParser();
		testSimpleIntOperations();

		testOperations<float>();
		testOperations<double>();
		testOperations<int>();
		
		testCompareOperators<double>();
		testCompareOperators<int>();
		testCompareOperators<float>();

		testTernaryOperator();
		testIfStatement();
		
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
	}


private:

	void expectCompileOK(Compiler* compiler)
	{
		auto r = compiler->getCompileResult();

		expect(r.wasOk(), r.getErrorMessage() + "\nFunction Code:\n\n" + compiler->getLastCompiledCode());
	}

	void expectAllFunctionsDefined(JITTestModule* )
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

		CREATE_TYPED_TEST("int v = 0; int test(block in) { loop_block(s: in) v += 1; return v; }");
		test->setup();
		auto numSamples2 = test->func["test"].call<int>(bl);
		expectEquals<int>(numSamples2, bl.size(), "Counting samples in block");

		for (int i = 0; i < b.getNumSamples(); i++)
		{
			b.setSample(0, i, (float)(i + 1));
		}
		
		CREATE_TYPED_TEST("float v = 0.0f; float test(block in) { loop_block(s: in) v = s; return v; }");
		test->setup();
		auto numSamples3 = test->func["test"].call<float>(bl);
		expectEquals<int>(numSamples3, (float)bl.size(), "read block value into global variable");

		CREATE_TYPED_TEST("int v = 0; int test(block in) { loop_block(s: in) v = s; return v; }");
		test->setup();
		auto numSamples4 = test->func["test"].call<int>(bl);
		expectEquals<int>(numSamples4, bl.size(), "read block value with cast");

#if 0
		double uptime = 0.0;

		/** Multichannel processing callback */
		void processFrame(block frame)
		{
			loop_block(s: frame)
			{
				s = (float)Math.sin(uptime);
				uptime += 0.001;
			}
		}
#endif
		b.clear();

        CREATE_TYPED_TEST("float test(block in){ in[1] = Math.abs(in, 124.0f); return 1.0f; };");
        test->setup();
        test->func["test"].call<float>(bl);
        expectEquals<float>(bl[1], 0.0f, "Calling function with wrong signature as block assignment");

        CREATE_TYPED_TEST("float test(block in){ double x = 2.0; in[1] = Math.sin(x); return 1.0f; };");
        test->setup();
        test->func["test"].call<float>(bl);
        expectEquals<float>(bl[1], (float)hmath::sin(2.0), "Implicit cast of function call to block assignment");
        
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
        
        CREATE_TYPED_TEST("float l = 1.94f; float test(block in){ loop_block(s: in) s = 2.4f; loop_block(s: in) l = s; return l; }");
        test->setup();
        auto shouldBe24 = test->func["test"].call<float>(bl);
        expectEquals<float>(shouldBe24, 2.4f, "Setting global variable in block loop");
        
		CREATE_TYPED_TEST("int v = 0; int test(block in) { loop_block(s: in) v += 1; return v; }");
		test->setup();
		auto numSamples = test->func["test"].call<int>(bl);
		expectEquals<int>(numSamples, bl.size(), "Counting samples in block");
        
		CREATE_TYPED_TEST("void test(block in){ loop_block(sample: in){ sample = 2.0f; }}");
		test->setup();

		auto f = test->func["test"];
		
		f.callVoid(bl);

		for (int i = 0; i < bl.size(); i++)
		{
			expectEquals<float>(bl[i], 2.0f, "Setting all values");
		}
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
		expectCompileOK(test->compiler);
		EXPECT("Empty scope", 12.0f, 2.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) {{ float x = x; x *= 1000.0f; } return x; }");
		expectCompileOK(test->compiler);
		EXPECT("Overwrite with local variable", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) {{ x *= 1000.0f; } return x; }");
		expectCompileOK(test->compiler);
		EXPECT("Change global in sub scope", 12.0f, 1000.0f);

		CREATE_TEST("float test(float input){ float x1 = 12.0f; float x2 = 12.0f; float x3 = 12.0f; float x4 = 12.0f; float x5 = 12.0f; float x6 = 12.0f; float x7 = 12.0f;float x8 = 12.0f; float x9 = 12.0f; float x10 = 12.0f; float x11 = 12.0f; float x12 = 12.0f; return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10 + x11 + x12; }");
		expectCompileOK(test->compiler);
		EXPECT("12 variables", 12.0f, 144.0f);

		CREATE_TEST("float test(float in) { float x = 8.0f; float y = 0.0f; { float x = x + 9.0f; y = x; } return y; }");
		expectCompileOK(test->compiler);
		EXPECT("Save scoped variable to local variable", 12.0f, 17.0f);
	}

	void testLogicalOperations()
	{
		beginTest("Testing logic operations");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float x = 0.0f; float test(float i){ return (true && false) ? 12.0f : 4.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("And with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return true && false ? 12.0f : 4.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("And without parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return true && true && false ? 12.0f : 4.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("Two Ands", 2.0f, 4.0f);

		CREATE_TEST("float x = 1.0f; float test(float i){ return true || false ? 12.0f : 4.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("Or", 2.0f, 12.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return (false || false) && true  ? 12.0f : 4.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("Or with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ return false || false && true ? 12.0f : 4.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("Or with parenthesis", 2.0f, 4.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ 0 && change(); return x;}");
		expectCompileOK(test->compiler);
		EXPECT("Short circuit of && operation", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ 1 || change(); return x;}");
		expectCompileOK(test->compiler);
		EXPECT("Short circuit of || operation", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ int c = change(); 0 && c; return x;}");
		expectCompileOK(test->compiler);
		EXPECT("Don't short circuit variable expression with &&", 12.0f, 5.0f);

		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ int c = change(); 1 || c; return x;}");
		expectCompileOK(test->compiler);
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
		expectCompileOK(test->compiler);
		EXPECT("Complex expression", value, ce(value));
	}

	void testGlobals()
	{
		beginTest("Testing Global variables");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float x=2.0f; void setup() { x = 5; } float test(float i){return x;};")
			expectCompileOK(test->compiler);
		EXPECT("Global implicit cast", 2.0f, 5.0f);

		CREATE_TEST("float x = 0.0f; float test(float i){ x=7.0f; return x; };");
		expectCompileOK(test->compiler);
		EXPECT("Global float", 2.0f, 7.0f);

		CREATE_TEST("float x=0.0f; float test(float i){ x=-7.0f; return x; };");
		expectCompileOK(test->compiler);
		EXPECT("Global negative float", 2.0f, -7.0f);

		CREATE_TEST("float x=-7.0f; float test(float i){ return x; };");
		expectCompileOK(test->compiler);
		EXPECT("Global negative float definition", 2.0f, -7.0f);


		CREATE_TEST_SETUP("double x = 2.0; void setup(){x = 26.0; }; float test(float i){ return (float)x;};");
		expectCompileOK(test->compiler);
		EXPECT("Global set & get from different functions", 2.0f, 26.0f);
		
		CREATE_TEST("float x=2.0f;float test(float i){return x*2.0f;};")
		expectCompileOK(test->compiler);
		EXPECT("Global float with operation", 2.0f, 4.0f)

		CREATE_TEST("int x=2;float test(float i){return (float)x;};")
		expectCompileOK(test->compiler);
		EXPECT("Global cast", 2.0f, 2.0f)

		CREATE_TEST("float x=2.0f; void setup() { x = 5; } float test(float i){return x;};")
		expectCompileOK(test->compiler);
		EXPECT("Global implicit cast", 2.0f, 5.0f);

		CREATE_TEST("int c=0;float test(float i){c+=1;c+=1;c+=1;return (float)c;};")
		expectCompileOK(test->compiler);
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

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " > " T_B " ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Conditional", T(), (T)(a > b ? 1 : 0));

		CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " > " T_B ") ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Conditional with Parenthesis", T(), (T)(((T)a > (T)b) ? 1 : 0));

		CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " + " T_B ") * " T_A ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Parenthesis", T(), ((T)a + (T)b)*(T)a);

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

        CREATE_TEST("float ov(int a){ return 9.0f; } float ov(double a) { return 14.0f; } float test(float input) { return ov(5); }");
        EXPECT("function overloading", 2.0f, 9.0f);
        
		Random r;

		const float v = r.nextFloat() * 122.0f * r.nextBool() ? 1.0f : -1.0f;

		CREATE_TEST("float square(float input){return input*input;}; float test(float input){ return square(input);};")
			EXPECT("JIT Function call", v, v*v);

        CREATE_TEST("float ov(int a){ return 9.0f; } float ov(double a) { return 14.0f; } float test(float input) { return ov(5); }");
        EXPECT("function overloading", 2.0f, 9.0f);
    
		CREATE_TEST("float a(){return 2.0f;}; float b(){ return 4.0f;}; float test(float input){ const float x = input > 50.0f ? a() : b(); return x;};")
			EXPECT("JIT Conditional function call", v, v > 50.0f ? 2.0f : 4.0f);

		CREATE_TEST("int isBigger(int a){return a > 0;}; float test(float input){return isBigger(4) ? 12.0f : 4.0f; };");
		EXPECT("int function", 2.0f, 12.0f);

		CREATE_TEST("int getIfTrue(int isTrue){return true ? 1 : 0;}; float test(float input) { return getIfTrue(true) == 1 ? 12.0f : 4.0f; }; ");
		EXPECT("int parameter", 2.0f, 12.0f);

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

	void testIfStatement()
	{
		beginTest("Test if-statement");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float test(float input){ if(input > 1.0f) return 10.0f; return 2.0f; }");
		EXPECT("True branch", 4.0f, 10.0f);
		EXPECT("Fall through", 0.5f, 2.0f);

		// TODO: add more if tests
	}

	void testTernaryOperator()
	{
		beginTest("Test ternary operator");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float test(float input){ return (input > 1.0f) ? 10.0f : 2.0f; }");

		EXPECT("Simple ternary operator true branch", 4.0f, 10.0f);
		EXPECT("Simple ternary operator false branch", -24.9f, 2.0f);

		CREATE_TEST("float test(float input){ return (true ? false : true) ? 12.0f : 4.0f; }; ");
		EXPECT("Nested ternary operator", 0.0f, 4.0f);
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

#pragma warning( pop)

}}



template <typename T> class HiseJITTestCase
{
public:

	HiseJITTestCase(const String& stringToTest) :
		code(stringToTest)
	{
		compiler = new HiseJITCompiler(code, false);
		scope = compiler->compileAndReturnScope();
	}

	~HiseJITTestCase()
	{
		compiler = nullptr;
		scope = nullptr;
	}

	T getResult(T input)
	{
		if (!compiler->wasCompiledOK())
		{
			DBG(compiler->getErrorMessage());
			return T();
		}

		auto t = scope->getCompiledFunction<T, T>("test");

		if (t != nullptr)
		{
			return t(input);
		}

		return T();
	};

	bool wasOK()
	{
		return compiler->wasCompiledOK();
	}

	void setup()
	{
		if (!compiler->wasCompiledOK())
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
	ScopedPointer<HiseJITCompiler> compiler;
	ScopedPointer<HiseJITScope> scope;

};

class HiseJITTestModule
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
		compiler = new HiseJITCompiler(code, false);
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

		compiler = new HiseJITCompiler(code, false);
	}

	void createModule()
	{
		module = new HiseJITDspModule(compiler);
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
	ScopedPointer<HiseJITCompiler> compiler = nullptr;

	ScopedPointer<HiseJITDspModule> module;

	double executionTime;

};



#define CREATE_TEST(x) test = new HiseJITTestCase<float>(x);
#define CREATE_TYPED_TEST(x) test = new HiseJITTestCase<T>(x);
#define CREATE_TEST_SETUP(x) test = new HiseJITTestCase<float>(x); test->setup();

#define EXPECT(testName, input, result) expect(test->wasOK(), String(testName) + String(" parsing")); expectEquals<float>(test->getResult(input), result, testName);

#define EXPECT_TYPED(testName, input, result) expect(test->wasOK(), String(testName) + String(" parsing")); expectEquals<T>(test->getResult(input), result, testName);

#define CREATE_BOOL_TEST(x) test = new HiseJITTestCase<BooleanType>(String("bool test(bool input){ ") + String(x));


#define EXPECT_BOOL(name, result) expect(test->wasOK(), String(name) + String(" parsing")); expect(test->getResult(0) == result, name);
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
		testSimpleIntOperations();

		testOperations<float>();
		testOperations<double>();
		testOperations<int>();

		testCompareOperators<float>();
		testCompareOperators<double>();
		testCompareOperators<int>();

		testLogicalOperations();

		testBigFunctionBuffer();

		testGlobals();

		testFunctionCalls();
		testDoubleFunctionCalls();

		testTernaryOperator();

		testComplexExpressions();

		testDspModules();

		//testDynamicObjectProperties();
		//testDynamicObjectFunctionCalls();
	}

private:

	void testSimpleIntOperations()
	{
		beginTest("Testing simple integer operations");

		ScopedPointer<HiseJITTestCase<int>> test;

		test = new HiseJITTestCase<int>("int x; int test(int input){ x = input; return x;};");
		expectCompileOK(test->compiler);
		EXPECT("int assignment", 6, 6);

		test = new HiseJITTestCase<int>("int x; int test(int input){ x = -5; return x;};");
		expectCompileOK(test->compiler);
		EXPECT("negative int assignment", 0, -5);

	}

	void testLogicalOperations()
	{
		beginTest("Testing logic operations");

		ScopedPointer<HiseJITTestCase<float>> test;

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

		ScopedPointer<HiseJITTestCase<float>> test;

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
		if (HiseJITTypeHelpers::is<T, float>()) return "float";
		if (HiseJITTypeHelpers::is<T, double>()) return "double";
		if (HiseJITTypeHelpers::is<T, int>()) return "int";
#if INCLUDE_BUFFERS
		if (HiseJITTypeHelpers::is<T, Buffer*>()) return "Buffer";
#endif

#if INCLUDE_CONDITIONALS
		if (HiseJITTypeHelpers::is<T, BooleanType>()) return "bool";
#endif

		return String();
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
		if (HiseJITTypeHelpers::is<T, float>())
		{
			String rs = String((float)value);

			return rs.containsChar('.') ? rs + "f" : rs + ".f";
		}
		if (HiseJITTypeHelpers::is<T, double>()) return String((double)value, 5);
		if (HiseJITTypeHelpers::is<T, int>()) return String((int)value);

		return String();
	}

	template <typename T> String getGlobalDefinition(double value)
	{
		const String valueString = getLiteral<T>(value);

		return getTypeName<T>() + " x = " + valueString + ";";
	}



	template <typename T> void testOperations()
	{
		beginTest("Testing operations for " + HiseJITTypeHelpers::getTypeName<T>());

		Random r;

		double a = (double)r.nextInt(25) *(r.nextBool() ? 1.0 : -1.0);
		double b = (double)r.nextInt(62) *(r.nextBool() ? 1.0 : -1.0);

		if (b == 0.0) b = 55.0;

		ScopedPointer<HiseJITTestCase<T>> test;

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " + " T_B ";"));
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Addition", T(), (T)(a + b));

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " - " T_B ";"));
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Subtraction", T(), (T)(a - b));

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " * " T_B ";"));
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Multiplication", T(), (T)a*(T)b);

		if (HiseJITTypeHelpers::is<T, int>())
		{
			double ta = a;
			double tb = b;

			a = abs(a);
			b = abs(b);

			CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " % " T_B ";"));
			EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Modulo", 0, (int)a % (int)b);

			CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x %= " T_B "; return x;};");
			EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " %= operator", T(), (int)a % (int)b);

			a = ta;
			b = tb;
		}

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " / " T_B ";"));

		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Division", T(), (T)(a / b));

#if INCLUDE_CONDITIONALS
		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " > " T_B " ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Conditional", T(), (T)(a > b ? 1 : 0));

		CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " > " T_B ") ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Conditional with Parenthesis", T(), (T)(((T)a > (T)b) ? 1 : 0));


		CREATE_TYPED_TEST(getTestFunction<T>("return (" T_A " + " T_B ") * " T_A ";"));
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " Parenthesis", T(), ((T)a + (T)b)*(T)a);
#endif

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x *= " T_B "; return x;};");
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " *= operator", T(), (T)a * (T)b);

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x /= " T_B "; return x;};");
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " /= operator", T(), (T)a / (T)b);

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x += " T_B "; return x;};");
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " += operator", T(), (T)a + (T)b);

		CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x -= " T_B "; return x;};");
		EXPECT_TYPED(HiseJITTypeHelpers::getTypeName<T>() + " -= operator", T(), (T)a - (T)b);
	}

	template <typename T> void testCompareOperators()
	{
		beginTest("Testing compare operators for " + HiseJITTypeHelpers::getTypeName<T>());

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

		float input = r.nextFloat() * 125.0f - 80.0f;

		CREATE_TEST("float test(float input){ return (float)(int)(8 > 5 ? (9.0*(double)input) : 1.23+ (double)(2.0f*input)); };");
		EXPECT("Complex expression 1", input, (float)(int)(8 > 5 ? (9.0*(double)input) : 1.23 + (double)(2.0f*input)));

		input = -1.0f * r.nextFloat() * 2.0f;

		CREATE_TEST("float test(float input){ return -1.5f * fabsf(input) + 2.0f * fabsf(input - 1.0f);}; ");
		EXPECT("Complex expression 2", input, -1.5f * fabsf(input) + 2.0f * fabsf(input - 1.0f));
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


		CREATE_TEST("bool isBigger(int a){return a > 0;}; float test(float input){return isBigger(4) ? 12.0f : 4.0f; };");
		EXPECT("bool function", 2.0f, 12.0f);

		CREATE_TEST("int getIfTrue(bool isTrue){return true ? 1 : 0;}; float test(float input) { return getIfTrue(true) == 1 ? 12.0f : 4.0f; }; ");
		EXPECT("bool parameter", 2.0f, 12.0f);
#endif

		String x;
		NewLine nl;

		x << "float x = 0.0f;" << nl;
		x << "void calculateX(float newX) {" << nl;
		x << "x = newX * 2.0f;" << nl;
		x << "};" << nl;
		x << "float setup(float input) {" << nl;
		x << "calculateX(4.0f); return 1.0f;" << nl;
		x << "}; float test(float input) { return x; }; ";

		CREATE_TEST_SETUP(x);
		EXPECT("JIT function call with global parameter", 0.0f, 8.0f);

	}

	void testDoubleFunctionCalls()
	{
		beginTest("Double Function Calls");

		ScopedPointer<HiseJITTestCase<double>> test;

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
		EXPECT_TYPED("sqrt", fabs(v), sqrt(fabs(v))); // No negative square root

		CREATE_TYPED_TEST(getTestFunction<double>("return fabs(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("fabs", v, fabs(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return exp(input);"))
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
		beginTest("Test DSP modules");

		testDspSimpleGain();
		testDspSimpleSine();
		testDspSimpleLP();
		testDspSimpleLP2();
		testDspAdditiveSynthesis();
		testDspSaturator();
		testDspExternalGain();
		testSimpleDelay();

	}

	void testDspSimpleGain()
	{
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();


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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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
		ScopedPointer<HiseJITTestModule> m = new HiseJITTestModule();

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

	void expectCompileOK(HiseJITCompiler* compiler)
	{
		expect(compiler->wasCompiledOK(), compiler->getErrorMessage() + "\nFunction Code:\n\n" + compiler->getCode(true));
	}


	void expectAllFunctionsDefined(HiseJITTestModule* m)
	{
		expect(m->module.get()->allOK(), "Not all functions defined");
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
		beginTest("DynamicObject methods for HiseJIT scopes");

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

		ScopedPointer<HiseJITCompiler> compiler = new HiseJITCompiler(code, false);

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
		ADD_CODE_LINE("    const int x = get1() + get2() + get3() + get4() + get5();\n");
		ADD_CODE_LINE("    const int y = get6() + get7() + get8() + get9();\n");
		ADD_CODE_LINE("    return (float)(x+y);\n");
		ADD_CODE_LINE("};");

		ScopedPointer<HiseJITCompiler> compiler = new HiseJITCompiler(code, false);

		ScopedPointer<HiseJITScope> scope = compiler->compileAndReturnScope();

		expectCompileOK(compiler);

		auto f = scope->getCompiledFunction<float, float>("test");

		float result = f(2.0f);

		expectEquals(result, 9.0f, "Testing reallocation of Function buffers");
	}
};


static HiseJITUnitTest njut;


#undef CREATE_TEST
#undef CREATE_TEST_SETUP
#undef EXPECT

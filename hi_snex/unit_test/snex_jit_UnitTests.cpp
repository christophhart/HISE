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
USE_ASMJIT_NAMESPACE;

#pragma warning( push )
#pragma warning( disable : 4244)


#define INCLUDE_SNEX_BIG_TESTSUITE 1

static int numTests = 0;


#if INCLUDE_SNEX_BIG_TESTSUITE
class OptimizationTestCase
{
public:

	void setOptimizations(const StringArray& passList)
	{
		for (auto& p : passList)
			optimizingScope.addOptimization(p);
	}

	void setExpressionBody(const juce::String& body_)
	{
		jassert(body_.contains("%BODY%"));

		body = body_;
	}

	bool sameAssembly(const juce::String& expressionToBeOptimised, const juce::String& reference)
	{
		auto o = body.replace("%BODY%", expressionToBeOptimised);
		auto r = body.replace("%BODY%", reference);

		auto oAssembly = getAssemblyOutput(o, optimizingScope);
		auto rAssembly = getAssemblyOutput(r, referenceScope);

		bool equal = oAssembly.hashCode64() == rAssembly.hashCode64();

		if (!equal)
		{
			DBG("Wrong assembly: ");
			
			DBG(oAssembly);

			DBG("Reference assembly:");
			DBG(rAssembly);

		}

		return equal;
	}

private:

	juce::String getAssemblyOutput(const juce::String& t, GlobalScope& s)
	{
		Compiler c(s);
		auto obj = c.compileJitObject(t);

		if (!c.getCompileResult().wasOk())
		{
			auto r = c.getCompileResult().getErrorMessage();
			jassertfalse;
			return t;
		}
			

		return c.getAssemblyCode();
	}

	juce::String body;
	GlobalScope referenceScope;
	GlobalScope optimizingScope;
};
    
template <typename T, typename ReturnType=T> class HiseJITTestCase: public jit::DebugHandler
{
public:

	HiseJITTestCase(const juce::String& stringToTest, const StringArray& optimizationList) :
		code(stringToTest)
	{
		for (auto o : optimizationList)
			memory.addOptimization(o);

		compiler = new Compiler(memory);

		Types::SnexObjectDatabase::registerObjects(*compiler, 2);
	}

	void logMessage(int level, const juce::String& s) override
	{
		DBG(s);
	}

	~HiseJITTestCase()
	{
		
	}

	bool compileFailWithMessage(const juce::String& errorMessage)
	{
		func = compiler->compileJitObject(code);

		//DBG(compiler->getCompileResult().getErrorMessage());

		auto actual = compiler->getCompileResult().getErrorMessage();

		if (actual != errorMessage)
			jassertfalse;

		return actual == errorMessage;
	}

	void dump()
	{
		breakBeforeCall = true;
	}

	ReturnType getResult(T input, ReturnType expected)
	{
		if (!initialised)
			setup();

		static const Identifier t("test");

		if (auto f = func[t])
		{
			before = func.dumpTable();

			if (breakBeforeCall)
			{
				DBG("code: ");
				DBG(compiler->getLastCompiledCode());
				DBG(compiler->dumpSyntaxTree());
				DBG(compiler->dumpNamespaceTree());
				DBG("assembly: ");
				DBG(compiler->getAssemblyCode());
				DBG("Data dump before call:");
				DBG(before);
				


				jassertfalse; // there you go...
			}

            auto v = f.template call<ReturnType>(input);
            
			auto diff = std::abs(v - expected);

			if (diff > 0.000001)
			{
				dump();

				DBG("Expected: " + juce::String(expected));
				DBG("Actual: " + juce::String(v));
			}

			return v;
		}
        else
        {
            DBG(compiler->getCompileResult().getErrorMessage());
        }

		return ReturnType();
	};


	juce::String before;
	bool breakBeforeCall = false;

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

	juce::String code;

	bool initialised = false;

	jit::GlobalScope memory;
	ScopedPointer<Compiler> compiler;
	JitObject func;

};


struct VectorTestObject
{
	VectorTestObject()
	{
		int i = 0;
		for (auto& s : data)
			s = (float)(i++ % 256) - 128.0f;
	}

	bool operator==(const VectorTestObject& other) const
	{
		for (int i = 0; i < data.size(); i++)
			if (data[i] != other.data[i])
				return false;

		return true;
	}

	operator String() const { return "funky"; };

	Types::span<float, 512> data;
	block a;
	block b;

	hmath Math;
};





class VectorOpTestCase: public VectorTestObject
{
public:
	VectorOpTestCase(UnitTest& t_, const StringArray optimizationList, const String& line):
		t(t_)
	{
		code = makeCode(line);

		for (auto o : optimizationList)
			m.addOptimization(o);


		Compiler c(m);
		Types::SnexObjectDatabase::registerObjects(c, 2);


		obj = c.compileJitObject(code);

		t.expect(c.getCompileResult().wasOk());

		f = obj["process"];
	}

	String makeCode(const String& line)
	{
		String s;

		s << "void process(block a, block b, float s)\n{\n\t";
		s << line;
		s << ";\n}";
		return s;
	}

	void test(float s, int size, int offset1, int offset2)
	{
		jassert(size + offset2 < 256);
		a.referTo(data, size, offset1);
		b.referTo(data, size, offset2 + 256);
		f.callVoid(&a, &b, s);
	}

	FunctionData f;
	String code;
	UnitTest& t;
	GlobalScope m;
	JitObject obj;
	
};


class ProcessTestCase
{
public:

	static constexpr int NumChannels = 2;

	using ProcessType = Types::ProcessData<NumChannels>;

	ProcessType::ChannelDataType makeChannelData()
	{
		return Types::ProcessDataHelpers<NumChannels>::makeChannelData(data, -1);
	}

	ProcessTestCase(UnitTest* test, GlobalScope& memory, const juce::String& code)
	{
		data = 0.0f;

		HiseEventBuffer b;

		HiseEvent on(HiseEvent::Type::NoteOn, 36, 127, 1);
		on.setTimeStamp(24);
		HiseEvent off(HiseEvent::Type::NoteOff, 36, 127, 1);
		off.setTimeStamp(56);

		b.addEvent(on);
		b.addEvent(off);
		
		auto cd = Types::ProcessDataHelpers<NumChannels>::makeChannelData(data, -1);
		int numSamples = Types::ProcessDataHelpers<NumChannels>::getNumSamplesForConsequentData(data, -1);
		ProcessType d(cd.begin(), numSamples);
		
		d.setEventBuffer(b);

		using T = void;
		Compiler c(memory);
		Types:: SnexObjectDatabase::registerObjects(c, NumChannels);

		auto obj = c.compileJitObject(code);

		test->expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile fail");

		auto f = obj["test"];
		v = f.call<int>(&d);
	}

	Types::span<float, 128> data;
	int v;
};



class JITTestModule
{
public:

	void setGlobals(const juce::String& t)
	{
		globals = t;
	}

	void setInitBody(const juce::String& body)
	{
		initBody = body;
	};

	void setPrepareToPlayBody(const juce::String& body)
	{
		prepareToPlayBody = body;
	}

	void setProcessBody(const juce::String& body)
	{
		processBody = body;
	}

	void setCode(const juce::String& code_)
	{
		code = code_;
	}

	void merge()
	{
		code = juce::String();
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

	int expectedLoopCount = -1;
	juce::String globals;
	juce::String initBody;
	juce::String prepareToPlayBody;
	juce::String processBody = "return 1.0f;";
	juce::String code;

	double executionTime;
};



#define CREATE_TEST(x) test = new HiseJITTestCase<float>(x, optimizations);
#define CREATE_TYPED_TEST(x) test = new HiseJITTestCase<T>(x, optimizations);
#define CREATE_TEST_SETUP(x) test = new HiseJITTestCase<float>(x, optimizations); test->setup();
#define EXPECT_COMPILE_FAIL(x) expect(test->compileFailWithMessage(x));

#define EXPECT(testName, input, result) expect(test->wasOK(), juce::String(testName) + juce::String(" parsing")); expectAlmostEquals<float>(test->getResult(input, result), result, testName);

#define EXPECT_TYPED(testName, input, result) expect(test->wasOK(), juce::String(testName) + juce::String(" parsing")); expectAlmostEquals<T>(test->getResult(input, result), result, testName);

#define GET_TYPE(T) Types::Helpers::getTypeNameFromTypeId<T>()

#define CREATE_BOOL_TEST(x) test = new HiseJITTestCase<int>(juce::String("int test(int input){ ") + juce::String(x), optimizations);


#define EXPECT_BOOL(name, result) expect(test->wasOK(), juce::String(name) + juce::String(" parsing")); expect(test->getResult(0, result) == result, name);
#define VAR_BUFFER_TEST_SIZE 8192

#define ADD_CODE_LINE(x) code << x << "\n"
#define FINALIZE_CODE() code = code.replace("$T", getTypeName<T>()); code = code.replace("$size", juce::String(size)); code = code.replace("$index", juce::String(index));

#define T_A + getLiteral<T>(a) +
#define T_B + getLiteral<T>(b) +
#define T_1 + getLiteral<T>(1.0) +
#define T_0 + getLiteral<T>(0.0) +

#define START_BENCHMARK const double start = Time::getMillisecondCounterHiRes();
#define STOP_BENCHMARK_AND_LOG const double end = Time::getMillisecondCounterHiRes(); logPerformanceMessage(m->executionTime, end - start);

#endif




class HiseJITUnitTest : public UnitTest
{
public:

	HiseJITUnitTest() : UnitTest("HiseJIT UnitTest", "snex") {}

	


	template <typename IntegerIndexType> void testIntegerIndex(int dynamicSize=0)
	{
		using ScaledFloatType	 = index::normalised<float, IntegerIndexType>;
		using UnscaledFloatType  = index::unscaled<float, IntegerIndexType>;
		using ScaledDoubleType   = index::normalised<double, IntegerIndexType>;
		using UnscaledDoubleType = index::unscaled<double, IntegerIndexType>;
		using UnscaledLerpFloatType = index::lerp<UnscaledFloatType>;
		using ScaledLerpFloatType = index::lerp<ScaledFloatType>;
		using UnscaledLerpDoubleType = index::lerp<UnscaledDoubleType>;
		using ScaledLerpDoubleType = index::lerp<ScaledDoubleType>;
		using UnscaledHermiteFloatType = index::hermite<UnscaledFloatType>;
		using ScaledHermiteFloatType = index::hermite<ScaledFloatType>;
		using UnscaledHermiteDoubleType = index::hermite<UnscaledDoubleType>;
		using ScaledHermiteDoubleType = index::hermite<ScaledDoubleType>;

		IndexTester<IntegerIndexType>(this, optimizations, dynamicSize);

		IndexTester<UnscaledFloatType>(this, optimizations, dynamicSize);
		IndexTester<UnscaledDoubleType>(this, optimizations, dynamicSize);

		IndexTester<ScaledFloatType>(this, optimizations, dynamicSize);
		IndexTester<ScaledDoubleType>(this, optimizations, dynamicSize);
		
		IndexTester<UnscaledLerpFloatType>(this, optimizations, dynamicSize);
		IndexTester<ScaledLerpFloatType>(this, optimizations, dynamicSize);
		IndexTester<UnscaledLerpDoubleType>(this, optimizations, dynamicSize);
		IndexTester<ScaledLerpDoubleType>(this, optimizations, dynamicSize);

		IndexTester<UnscaledHermiteFloatType>(this, optimizations, dynamicSize);
		IndexTester<ScaledHermiteFloatType>(this, optimizations, dynamicSize);
		IndexTester<UnscaledHermiteDoubleType>(this, optimizations, dynamicSize);
		IndexTester<ScaledHermiteDoubleType>(this, optimizations, dynamicSize);
	}

	void testIndexTypes()
	{
        beginTest("Test index types");
        
		//testIntegerIndex<index::looped<9, false>>();
		//testIntegerIndex<index::looped<64, false>>();
		testIntegerIndex<index::wrapped<32, false>>();
		testIntegerIndex<index::wrapped<91, false>>();
        testIntegerIndex<index::clamped<32, false>>();
		testIntegerIndex<index::clamped<91, false>>();
        testIntegerIndex<index::unsafe<91, false>>();
		testIntegerIndex<index::unsafe<64, true>>();
	}

	void runTest() override
	{
		beginTest("funky");

		optimizations = OptimizationIds::Helpers::getDefaultIds();

		runTestsWithOptimisation(OptimizationIds::Helpers::getDefaultIds());
	}

#if INCLUDE_SNEX_BIG_TESTSUITE
	

	void testExternalFunctionCalls()
	{
		beginTest("Testing external function calls");

		GlobalScope s;
		Compiler compiler(s);
		auto obj = compiler.compileJitObject("void test(double& d){ d = 0.5; };");

		double v = 0.2;

		obj["test"].callVoid(&v);

		expectEquals(v, 0.5, "doesn't work");

	}

	

	void testValueTreeCodeBuilder()
	{
		File f = JitFileTestCase::getTestFileDirectory().getChildFile("node.xml");

		if (auto xml = XmlDocument::parse(f))
		{
			auto v = ValueTree::fromXml(*xml);
			cppgen::ValueTreeBuilder b(v, cppgen::ValueTreeBuilder::Format::TestCaseFile);
		}
	}

	using OpList = StringArray;

	void addRecursive(Array<OpList>& combinations, const OpList all)
	{
		if (all.isEmpty())
			return;

		combinations.addIfNotAlreadyThere(all);



		for (int i = 0; i < all.size(); i++)
		{
			OpList less;
			less.addArray(all);


			less.remove(i);
			less.sort(false);
			addRecursive(combinations, less);
		}
	}

	void runTestFileWithAllOptimizations(const String& file)
	{
		printDebugInfoForSingleTest = false;

		auto combinations = getAllOptimizationCombinations();

		String fileName = file;

		auto f = getFiles(fileName, false).getFirst();

		for (auto c : combinations)
		{
			String s;

			s << "Opt: ";

			for (auto o : c)
				s << o << ";";
			logMessage(s);

			GlobalScope memory;
			for (auto o : c)
				memory.addOptimization(o);

			{
				JitFileTestCase t(this, memory, f);
				auto r = t.test(false);
			}
		}
	}

	Array<OpList> getAllOptimizationCombinations()
	{
		OpList allIds = OptimizationIds::Helpers::getAllIds();

		allIds.sort(false);

		Array<OpList> combinations;

		addRecursive(combinations, allIds);


		struct SizeSorter
		{
			static int compareElements(const OpList& a, const OpList& b)
			{
				if (a.size() > b.size())
					return 1;
				else if (a.size() < b.size())
					return -1;
				else
					return 0;
			}
		};

		SizeSorter sorter;
		combinations.sort(sorter);

		return combinations;
	}

	void testAllOptimizations()
	{
		auto combinations = getAllOptimizationCombinations();

		juce::String s;

		for (auto c : combinations)
		{
			runTestsWithOptimisation(c);

			for (auto id : c)
				s << id << " ";

			s << "\n";
		}
	}


	


	void testInlining()
	{
		optimizations = { OptimizationIds::Inlining };

		testMathInlining<double>();
		testMathInlining<float>();
		testFunctionInlining();
		testInlinedMathPerformance();
	}

	void runTestsWithOptimisation(const StringArray& ids)
	{
		PerformanceCounter pc("run of all tests with optimisations");

		logMessage("OPTIMIZATIONS");

		for (auto o : ids)
			logMessage("--- " + o);

		optimizations = ids;

		pc.start();

		testStaticConst();
		testFpu();

		testParser();
		testSimpleIntOperations();

		testOperations<float>();
		testOperations<double>();
		testOperations<int>();

		testCompareOperators<double>();
		testCompareOperators<int>();
		testCompareOperators<float>();

		testPointerVariables<int>();
		testPointerVariables<double>();
		testPointerVariables<float>();

		

		testTernaryOperator();
		testIfStatement();

		
		testMathConstants<float>();
		testMathConstants<double>();
        
		testComplexExpressions();
		testGlobals();
		testFunctionCalls();
		testDoubleFunctionCalls();
		testBigFunctionBuffer();
		testLogicalOperations();
		
		testExternalStructDefinition();
		testExternalTypeDatabase<float>();
		testExternalTypeDatabase<double>();

		testScopes();
		
		testBlocks();
		testSpan<int>();
		testSpan<float>();
		testSpan<double>();
		testStructs();
		testUsingAliases();
		testProcessData();

		testMacOSRelocation();

		testExternalFunctionCalls();
		
		testEvents();

		runTestFiles();
		testIndexTypes();

		pc.stop();
	}
#endif

	Array<File> getFiles(juce::String& soloTest, bool isFolder)
	{
		if (soloTest.isNotEmpty() && !isFolder && !soloTest.endsWith(".h"))
		{
			soloTest.append(".h", 2);
		}

		File f = JitFileTestCase::getTestFileDirectory();

		if (isFolder)
		{
			jassert(soloTest.isNotEmpty());
			f = f.getChildFile(soloTest);
		}

		auto allFiles = f.findChildFiles(File::findFiles, true, "*.h");

		for (int i = 0; i < allFiles.size(); i++)
		{
			if (allFiles[i].getFullPathName().contains("CppTest"))
				allFiles.remove(i--);
		}

		return allFiles;
	}

	void runTestFiles(juce::String soloTest = {}, bool isFolder = false)
	{
		if (soloTest.isEmpty())
			beginTest("Testing files from test directory");

		GlobalScope memory;

		for (auto o : optimizations)
			memory.addOptimization(o);

		for (auto f : getFiles(soloTest, isFolder))
		{
			if (!isFolder && soloTest.isNotEmpty() && f.getFileName() != soloTest)
				continue;

#if JUCE_DEBUG
			if (printDebugInfoForSingleTest)
			{
				DBG(f.getFileName());
			}
#endif
				

			int numInstances = ComplexType::numInstances;

			{
				JitFileTestCase t(this, memory, f);
				auto r = t.test(printDebugInfoForSingleTest && soloTest.isNotEmpty());



				expect(r.wasOk(), r.getErrorMessage());
			}

			int numInstancesAfter = ComplexType::numInstances;

			if (numInstances != numInstancesAfter)
			{
				juce::String s;
				s << f.getFileName() << " leaked " << juce::String(numInstancesAfter - numInstances) << " complex types";
				expect(false, s);
			}
		}
	}

private:

	

	bool printDebugInfoForSingleTest = true;

#if INCLUDE_SNEX_BIG_TESTSUITE

	void trimNoopFromSyntaxTree(juce::String& s)
	{
		auto sa = StringArray::fromLines(s);

		for (int i = 0; i < sa.size(); i++)
		{
			if (sa[i].contains("Noop"))
				sa.remove(i--);
		}

		s = sa.joinIntoString("\n");
		s.trim();
	}

	void expectedEqualSyntaxTree(juce::String firstTree, juce::String secondTree, const juce::String& errorMessage)
	{
		trimNoopFromSyntaxTree(firstTree);
		trimNoopFromSyntaxTree(secondTree);

		auto match = firstTree.compare(secondTree);

		if (match != 0)
		{
			DBG(firstTree);
			DBG(secondTree);
		}

		expectEquals(match, 0, errorMessage);
	}

	void testFunctionInlining()
	{
		beginTest("Test function inlining");
		juce::String size, index, code;

		GlobalScope m;
		m.addOptimization(OptimizationIds::Inlining);
		Compiler c(m);
		
		{
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("    if(input == 2) {return input;}");
			ADD_CODE_LINE("    return input * 2;");
			ADD_CODE_LINE("}");

			c.compileJitObject(code);
			expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");
		}

		auto firstTree = c.dumpSyntaxTree();
		
		
		
		{
			code = {};

			ADD_CODE_LINE("int other(int o) { return o * o; };");
			ADD_CODE_LINE("int test(int input) { return other(input) + 2; };");

			auto obj = c.compileJitObject(code);
			expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");

			auto firstTree = c.dumpSyntaxTree();

			auto v = obj["test"].call<int>(4);

			expect(!c.getAssemblyCode().contains("call"), "Inlining didn't work");
			expectEquals(v, 4 * 4 + 2, "Wrong value");
		}
	}

	void testHiseEventExternalType()
	{
		juce::String size, index, code;

		beginTest("Test external HiseEvent type");

		GlobalScope m;

		for (auto o : optimizations)
			m.addOptimization(o);

		
		HiseEvent e(HiseEvent::Type::NoteOn, 75, 125, 3);

		{
			Compiler c(m);

			Types::SnexObjectDatabase::registerObjects(c, 1);

			auto en = e.getNoteNumber();
			auto ev = (int)e.getVelocity();
			auto ec = e.getChannel();

			ADD_CODE_LINE("int test(int s, HiseEvent e) {");
			ADD_CODE_LINE("    if(s == 0) return e.getNoteNumber();");
			ADD_CODE_LINE("    if(s == 1) return e.getVelocity();");
			ADD_CODE_LINE("    if(s == 2) return e.getChannel();");
			ADD_CODE_LINE("    return 0;");
			ADD_CODE_LINE("}");

			auto obj = c.compileJitObject(code);

			expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");

			auto f = obj["test"];

			auto an = f.call<int>(0, &e);
			auto av = f.call<int>(1, &e);
			auto ac = f.call<int>(2, &e);

			expectEquals(av, ev, "velocity mismatch");
			expectEquals(ac, ec, "channel mismatch");
			expectEquals(an, en, "note number mismatch");
		}

		{
			Compiler c(m);

			Types::SnexObjectDatabase::registerObjects(c, 1);

			HiseEvent e(HiseEvent::Type::NoteOn, 75, 125, 3);

			code = {};

			ADD_CODE_LINE("void test(HiseEvent& e, int n, int v, int c) {");
			ADD_CODE_LINE("    e.setVelocity(v);");
			ADD_CODE_LINE("    e.setNoteNumber(n);");
			ADD_CODE_LINE("    e.setChannel(c);");
			
			ADD_CODE_LINE("}");

			auto obj = c.compileJitObject(code);

			expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");

			auto f = obj["test"];

			f.callVoid(&e, 13, 80, 9);

			expectEquals(e.getNoteNumber(), 13, "set: note number mismatch");
			expectEquals((int)e.getVelocity(), 80, "set: velocity mismatch");
			expectEquals(e.getChannel(), 9, "set: channel mismatch");
		}
	}

	void testInlinedMathPerformance()
	{
		beginTest("Testing inline math performance");

		juce::String size, index, code;

		ADD_CODE_LINE("span<float, 441000> data;");
		ADD_CODE_LINE("float test(float input)");
		ADD_CODE_LINE("{");
		ADD_CODE_LINE("    for(auto& s: data)");
		ADD_CODE_LINE("        input += Math.max(Math.abs(input), Math.min(1.0f, input * 0.5f));");
		ADD_CODE_LINE("    return input;");
		ADD_CODE_LINE("}");

		GlobalScope m;
		for (auto o : optimizations)
			m.addOptimization(o);

		Compiler c(m);

		auto obj = c.compileJitObject(code);

		expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");

		auto f = obj["test"];

		juce::String mem;
		mem << "Testing inline math performance";

		for (auto o : optimizations)
			mem << " with " << o;

		PerformanceCounter pc(mem);
		pc.start();
		f.call<float>(12.0f);
		pc.stop();
	}

	template <typename T> void testExternalTypeDatabase()
	{
		juce::String size, index, code;

		beginTest("Test external type database");

		GlobalScope m;

		for (auto o : optimizations)
			m.addOptimization(o);
		
		Compiler c(m);
		
		Types::SnexObjectDatabase::registerObjects(c, 2);

		{
			Types::pimpl::_ramp<T> d;

			d.prepare(4000.0, 100.0);
			d.set(T(1));
			d.advance();
			T temp = d.get();
			d.reset();
			d.advance();
			d.set(T(0.5));
			auto e = temp + d.get() + d.advance();
            ignoreUnused(e);

			code = {};

			ADD_CODE_LINE("s$T d;");
			ADD_CODE_LINE("$T test() {");
			ADD_CODE_LINE("    d.prepare(4000.0, 100.0);");
			ADD_CODE_LINE("    d.set(($T)1.0);");
			ADD_CODE_LINE("    d.advance();");
			ADD_CODE_LINE("    $T temp = d.get();");
			ADD_CODE_LINE("    d.advance();");
			ADD_CODE_LINE("    d.set(($T)0.5);");
			ADD_CODE_LINE("    return temp + d.get() + d.advance();");
			ADD_CODE_LINE("}");

			FINALIZE_CODE();

			auto obj = c.compileJitObject(code);

			//expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");

			auto a = obj["test"].call<T>();

            ignoreUnused(a);
			//expectEquals(a, e, "value mismatch");
		}

		{
			juce::String mes;

			mes << "testing s" << Types::Helpers::getTypeName(Types::Helpers::getTypeFromTypeId<T>()) << " performance";

			for (auto o : optimizations)
				mes << " with " << o;

			PerformanceCounter pc(mes);

			code = {};
			ADD_CODE_LINE("span<$T, 44100> data;");
			ADD_CODE_LINE("s$T d;");
			ADD_CODE_LINE("$T test($T input) {");
			ADD_CODE_LINE("    d.prepare(44100.0, 5000.0);");
			ADD_CODE_LINE("    d.reset();");
			ADD_CODE_LINE("    d.set(input);");
			ADD_CODE_LINE("    for(auto& s: data)");
			ADD_CODE_LINE("    {");
			ADD_CODE_LINE("        s = d.advance();");
			ADD_CODE_LINE("    }");
			ADD_CODE_LINE("    return data[12];");
			ADD_CODE_LINE("}");
			
			FINALIZE_CODE();

			auto obj = c.compileJitObject(code);

			//DBG(c.getAssemblyCode());

			expectEquals(c.getCompileResult().getErrorMessage(), juce::String(), "compile error");

			auto f = obj["test"];

			pc.start();
			f.call<T>(2.0f);
			pc.stop();
		}
	}

	template <typename T> void testSpan()
	{
		auto type = Types::Helpers::getTypeFromTypeId<T>();

		beginTest("Testing span operations for " + Types::Helpers::getTypeName(type));

		Random r;
		int size = r.nextInt({ 1, 100 });
		int index = size > 1 ? r.nextInt({ 0, size - 1 }) : 0;
		double a = (double)r.nextInt(25) *(r.nextBool() ? 1.0 : -1.0);
		double b = (double)r.nextInt(62) *(r.nextBool() ? 1.0 : -1.0);

		if (b == 0.0) b = 55.0;

		ScopedPointer<HiseJITTestCase<T>> test;

		juce::String code;

#define NEW_CODE_TEXT() code = {};
#define DECLARE_SPAN(name) ADD_CODE_LINE("span<$T, $size> " + juce::String(name) + ";")

		auto im = [](T v)
		{
			auto type = Types::Helpers::getTypeFromTypeId<T>();
			VariableStorage v_(type, var(v));
			return Types::Helpers::getCppValueString(v_);
		};

		juce::String tdi;

		tdi << "{ " << im(1) << ", " << im(2) << ", " << im(3) << "};";

		NEW_CODE_TEXT();

		juce::String st;
		st << "struct X { double unused = 2.0; $T value = " << im(2) << "; };";

		ADD_CODE_LINE(st);
		ADD_CODE_LINE("span<X, 3> data;");
		ADD_CODE_LINE("$T test($T input){");
		ADD_CODE_LINE("    for(auto& s: data)");
		ADD_CODE_LINE("        input += s.value;");

		ADD_CODE_LINE("    return input;}");
		FINALIZE_CODE();

		CREATE_TYPED_TEST(code);
		EXPECT_TYPED(GET_TYPE(T) + " iterator with struct element type ", 0, 6);

		{
			NEW_CODE_TEXT();
			DECLARE_SPAN("data");
			ADD_CODE_LINE("index::wrapped<$size> index = {0};");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    int i = (int)input + 2;");
			ADD_CODE_LINE("    i = i > $size ? ($size -1 ) : i;");
			ADD_CODE_LINE("    index = i;");
			ADD_CODE_LINE("    data[index] = ($T)4.0;");
			ADD_CODE_LINE("    return data[index];}");
			FINALIZE_CODE();


			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " span set with dynamic index", T(index), T(4.0));
		}

		{
			NEW_CODE_TEXT();
			DECLARE_SPAN("data");
			ADD_CODE_LINE("index::wrapped<$size> index = {0};");
			ADD_CODE_LINE("int clamp(int i) { return i > $size ? ($size -1 ) : i; };");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    index = clamp((int)input + 2);");
			ADD_CODE_LINE("    Math.random();");
			ADD_CODE_LINE("    data[index] = ($T)4.0;");
			ADD_CODE_LINE("    return data[index];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);

			EXPECT_TYPED(GET_TYPE(T) + " span set with dynamic index", T(index), T(4.0));
		}
        
        
		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<$T, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    return data[4];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_COMPILE_FAIL("Line 3(17): constant index out of bounds");

		}

		

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<$T, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    for(auto& s: data)");
			ADD_CODE_LINE("        input += s;");
			ADD_CODE_LINE("    return input;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " iterator: sum elements", 0, 6);
		}

		{
			NEW_CODE_TEXT();
			DECLARE_SPAN("data");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    data[$index] = input;");
			ADD_CODE_LINE("    return input;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " span set and return", T(a), T(a));
		}
		
		{
			NEW_CODE_TEXT();
			DECLARE_SPAN("data");
			ADD_CODE_LINE("void other($T input){");
			ADD_CODE_LINE("    data[$index] = input;}");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    other(input);");
			ADD_CODE_LINE("    return data[$index];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " span set in different function", T(a), T(a));
		}

		{
			NEW_CODE_TEXT();
			DECLARE_SPAN("data");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    data[0] = 4.0;");
			ADD_CODE_LINE("    return data[0];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " span set with implicit cast", T(index), T(4.0));
		}
		
		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<$T, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    for(auto& s: data)");
			ADD_CODE_LINE("        s = 4;");
			ADD_CODE_LINE("    for(auto& s2: data)");
			ADD_CODE_LINE("        input += s2;");

			ADD_CODE_LINE("    return input;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " iterator: sum elements after set", 0, 12);
		}

		{
			NEW_CODE_TEXT();

			juce::String st;
			st << "struct X { double unused = 2.0; $T value = " << im(2) << "; };";

			ADD_CODE_LINE(st);
			ADD_CODE_LINE("span<X, 3> data;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    for(auto& s: data)");
			ADD_CODE_LINE("        input += s.value;");

			ADD_CODE_LINE("    return input;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " iterator with struct element type ", 0, 6);
		}

		{
			NEW_CODE_TEXT();
			DECLARE_SPAN("data");
			ADD_CODE_LINE("index::wrapped<$size> index;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    index = (int)input;");
			ADD_CODE_LINE("    data[index] = 4.0;");
			ADD_CODE_LINE("    return data[index];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " span set with index cast", T(index), T(4.0));
		}

		
		
		
		// ============================================================================================= 2D span tests

		tdi = {};
		tdi << "{ { " << im(1) << ", " << im(2) << " }, { " << im(3) << ", " << im(4) << "}, {" << im(5) << ", " << im(6) << "} };";

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("index::wrapped<2> j;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    j = (int)1.8f;");
			ADD_CODE_LINE("    return data[2][j];}");

			FINALIZE_CODE();
			
			CREATE_TYPED_TEST(code);
			//test->dump();
			EXPECT_TYPED(GET_TYPE(T) + " return 2D span element with j index cast", 0, 6);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    $T sum = data[0][0] + data[0][1] + data[1][0] + data[1][1] + data[2][0] + data[2][1];");
			ADD_CODE_LINE("    return sum;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " sum two dimensional array", 0, 1+2+3+4+5+6);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    return data[1][1];}");

			FINALIZE_CODE();
			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " return 2D span element with immediates", 0, 4);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("index::wrapped<3> i;");
			ADD_CODE_LINE("index::wrapped<2> j;");
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    i = (int)1.2;");
			ADD_CODE_LINE("    j = (int)1.2;");
			ADD_CODE_LINE("    return data[i][j]; }");

			FINALIZE_CODE();
			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " return 2D span element with i && j index cast", 0, 4);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("index::wrapped<3> i;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    i = (int)1.2;");
			ADD_CODE_LINE("    return data[i][0];}");

			FINALIZE_CODE();
			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " return 2D span element with i index cast", 0, 3);
		}


		

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    data[0][0] = data[0][0] + data[0][1];");
			ADD_CODE_LINE("    return data[0][0];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " sum span element to itself", 0, 3);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("void other(){");
			ADD_CODE_LINE("    $T tempValue = data[1][1] + data[2][0];");
			ADD_CODE_LINE("    data[0][0] = tempValue;");
			ADD_CODE_LINE("}");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    other();");
			ADD_CODE_LINE("    return data[0][0] + input;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " change 2D span in other function", 2, 11);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("index::wrapped<3, true> i;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    i = (int)input;");
			ADD_CODE_LINE("    data[0][0] = data[i][0];");
			ADD_CODE_LINE("    return data[0][0];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " 2D-span with dynamic i-index", 2, 5);
		}

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 2>, 3> data = " + tdi);
			ADD_CODE_LINE("index::wrapped<2> j;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    j = (int)input;");
			ADD_CODE_LINE("    data[0][0] = data[1][j];");
			ADD_CODE_LINE("    return data[0][0];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " 2D-span with dynamic j-index", 1, 4);
		}

		tdi = {};
		tdi << "{ { " << im(1) << ", " << im(2) << ", " << im(3) << "} , {" << im(4) << ", " << im(5) << ", " << im(6) << "} };";

		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 3>, 2> data = " + tdi);
			ADD_CODE_LINE("index::wrapped<2> i;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    i = (int)input;");
			ADD_CODE_LINE("    data[0][0] = data[i][0];");
			ADD_CODE_LINE("    return data[0][0];}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " 2D-span with 3 elements with dynamic i-index", 1, 4);
		}
		{
			NEW_CODE_TEXT();
			ADD_CODE_LINE("span<span<$T, 3>, 2> data = " + tdi);
			ADD_CODE_LINE("index::wrapped<2> i;");
			ADD_CODE_LINE("$T test($T input){");
			ADD_CODE_LINE("    i = (int)input;");
			ADD_CODE_LINE("    data[0][0] = data[i][0];");
			ADD_CODE_LINE("    return input;}");
			FINALIZE_CODE();

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED(GET_TYPE(T) + " don't change input register", 1, 1);
		}
	}

	

	void expectCompileOK(Compiler* compiler)
	{
		auto r = compiler->getCompileResult();

		expect(r.wasOk(), r.getErrorMessage() + "\nFunction Code:\n\n" + compiler->getLastCompiledCode());
	}

	void testProcessData()
	{
		beginTest("Testing ProcessData struct");

		GlobalScope memory;

		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<NumChannels>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("    auto f = d.toFrameData();");
			ADD_CODE_LINE("    while(f.next()) {");
			ADD_CODE_LINE("        for(auto& s: f)");
			ADD_CODE_LINE("            s = 0.8f;");
			ADD_CODE_LINE("    }");
			ADD_CODE_LINE("    return 1;}");

			ProcessTestCase test(this, memory, code);

			expectEquals(test.v, 1, "next returns one");
			expectEquals(test.data[0], 0.8f, "first frame 1");
			expectEquals(test.data[64], 0.8f, "first frame 2");
		}

		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<2>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("for(auto& s: d[0]){");
			ADD_CODE_LINE("    s = 1.0f;}");
			ADD_CODE_LINE("for(auto& s: d[1]){");
			ADD_CODE_LINE("    s = 2.0f;}");
			ADD_CODE_LINE(" return 0;}");

			ProcessTestCase test(this, memory, code);

			float sum = 0.0f;

			for (auto& s : test.data)
			{
				sum += s;
			}

			expectEquals(sum, 64.0f + 128.0f, "not working");

		}
		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<2>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("for(auto& ch: d){");
			ADD_CODE_LINE("    for(auto& s: d.toChannelData(ch)){");
			ADD_CODE_LINE("        s = 1.0f;");
			ADD_CODE_LINE("    }");
			ADD_CODE_LINE("}");
			ADD_CODE_LINE(" return 0;}");

			ProcessTestCase test(this, memory, code);

			float sum = 0.0f;

			for (auto& s : test.data)
			{
				sum += s;
			}

			expectEquals(sum, 128.0f, "not working");

		}
		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<NumChannels>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("int x = 0;");
			ADD_CODE_LINE("for(auto& e: d.toEventData())");
			ADD_CODE_LINE("    x += e.getNoteNumber();");
			ADD_CODE_LINE("return x;}");

			ProcessTestCase test(this, memory, code);

			expectEquals<int>(test.v, 72, "event iterator");
		}

		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<NumChannels>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("    auto f = d.toFrameData();");
			ADD_CODE_LINE("    while(f.next())");
			ADD_CODE_LINE("         f[0] = 3.0f;");
			ADD_CODE_LINE("    return f.next();");

			ProcessTestCase test(this, memory, code);

			float sum = 0.0f;

			for (auto& s : test.data)
				sum += s;

			expectEquals(sum, 3.0f * 64.0f, "first channel set");
			expectEquals(test.v, 0, "next is done");
		}

		

		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<NumChannels>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("    auto f = d.toFrameData();");
			ADD_CODE_LINE("    f.next();");
			ADD_CODE_LINE("    f[0] = 1.9f;");
			ADD_CODE_LINE("    f[1] = 4.9f;");
			ADD_CODE_LINE("    return f.next();}");

			ProcessTestCase test(this, memory, code);

			expectEquals(test.v, 1, "next returns one");
			expectEquals(test.data[0], 1.9f, "operator[] 1");
			expectEquals(test.data[64], 4.9f, "operator[] 1");
		}

		{
			juce::String code;
			ADD_CODE_LINE("int test(ProcessData<NumChannels>& d)");
			ADD_CODE_LINE("{");
			ADD_CODE_LINE("    auto f = d.toFrameData();");
			ADD_CODE_LINE("    int v = f.next();");
			ADD_CODE_LINE("    for(auto& s: f)");
			ADD_CODE_LINE("        s = 0.8f;");
			ADD_CODE_LINE("    v = f.next();");
			ADD_CODE_LINE("    for(auto& s: f)");
			ADD_CODE_LINE("        s = 1.4f;");
			ADD_CODE_LINE("return f.next();}");

			ProcessTestCase test(this, memory, code);

			
			expectEquals(test.data[0], 0.8f, "first frame 1");
			expectEquals(test.data[64], 0.8f, "first frame 2");
			expectEquals(test.data[1], 1.4f, "first frame 1");
			expectEquals(test.data[65], 1.4f, "first frame 2");
		}
	}

	void testVectorOps()
	{
		beginTest("Testing vector ops");

		testVectorOps({});
		testVectorOps({ OptimizationIds::AutoVectorisation });
		testVectorOps({ OptimizationIds::AutoVectorisation, OptimizationIds::AsmOptimisation });
		testVectorOps({ OptimizationIds::AutoVectorisation, OptimizationIds::BinaryOpOptimisation, OptimizationIds::AsmOptimisation });
	}

	void testVectorOps(const StringArray& opt)
	{
		optimizations = opt;

		testVectorOps(false, false);
		testVectorOps(false, true);
		testVectorOps(true, false);
		testVectorOps(true, true);
	}

	void testVectorOps(bool allowOffset, bool forceSimdSize)
	{

		Random r;

		int a, b, s; // f**ing autocomplete...

        ignoreUnused(a, b, s);
        
		using T = block;

		auto numSamples = forceSimdSize ? r.nextInt({1, 13}) * 4 : r.nextInt({ 3, 90 });
		auto scalar = (float)r.nextInt({ 1, 15 });
		auto o1 = allowOffset ? r.nextInt({ 0, 6 }) * 2 : 0;
		auto o2 = allowOffset ? r.nextInt({ 0, 6 }) * 2 : 0;

		Types::span<float, 512> expected;
		Types::span<float, 512> actual;

#define VECTOR_OBJECT(code) struct CppObject: public VectorTestObject { void test(float s, int size, int offset1, int offset2) { a.referTo(data, size, offset1); b.referTo(data, size, offset2 + 256); code; }} cppObject;
#define JIT_VECTOR_OBJECT(line) VectorOpTestCase t(*this, optimizations, #line);

#define TEST_VECTOR(line)  {JIT_VECTOR_OBJECT(line); VECTOR_OBJECT(line); t.test(scalar, numSamples, o1,o2); cppObject.test(scalar, numSamples, o1, o2); expect(t == cppObject, #line); /*expected = (cppObject.data); actual = (t.data);*/}

		TEST_VECTOR(a = Math.min(Math.max(a, 35.0f), 40.0f));
		TEST_VECTOR(a *= (b - 80.0f) * s + (a - s + b));
		TEST_VECTOR(a = Math.abs(a));
		TEST_VECTOR(a = Math.min(a, b));
		TEST_VECTOR(a = Math.min(a, 35.0f));
		TEST_VECTOR(a = Math.min(Math.max(a, 35.0f), 40.0f));
		TEST_VECTOR(a = Math.max(a, 12.0f) + b);
		TEST_VECTOR(a = Math.abs(b) + (a * 12.0f));
		TEST_VECTOR(a *= b);
		TEST_VECTOR(a = s);
		TEST_VECTOR(a = 15.0f);
		TEST_VECTOR(a = (b * 15.0f) + a);
		TEST_VECTOR(a = b - 15.0f);
		TEST_VECTOR(a = b);
		TEST_VECTOR(a = s);
		TEST_VECTOR(a *= s);
		TEST_VECTOR(a *= b);
		TEST_VECTOR(a *= b; a *= s);
		TEST_VECTOR(a += s);
		TEST_VECTOR(a += b);
		TEST_VECTOR(a = b + 12.0f);
		TEST_VECTOR(a = b + (a * b));
		TEST_VECTOR(a *= (b - 80.0f) * s + (a - s + b));
	}

	void testStaticConst()
	{
		beginTest("Testing static const");

		using T = int;

		ScopedPointer<HiseJITTestCase<T>> test;
		
		{
			juce::String code;

			ADD_CODE_LINE("int test(int input) { static const int x = 4; return x; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("static const variable in function", 5, 4);
		}
		
		{
			juce::String code;

			ADD_CODE_LINE("static const float x = Math.abs(-8.0f);");
			ADD_CODE_LINE("int test(int input) { return (int)x; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("static const variable with constexpr Math call", 5, 8);
		}
		{
			juce::String code;

			ADD_CODE_LINE("static const int x = 4 + 9;");
			ADD_CODE_LINE("int test(int input) { return x; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("static const variable with binary op", 5, 13);
		}
		{
			juce::String code;

			ADD_CODE_LINE("static const int x = 4;");
			ADD_CODE_LINE("int test(int input) { return x; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("static const variable", 5, 4);
		}
		{
			juce::String code;

			ADD_CODE_LINE("static const int x = 49;");
			ADD_CODE_LINE("static const int y = x > 50 ? 87 : 83;");
			ADD_CODE_LINE("int test(int input) { return y; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("static const variable with ternary op", 5, 83);
		}
		{
#if 0
			juce::String code;

			ADD_CODE_LINE("static const int x = 4;");
			
			ADD_CODE_LINE("span<int, x> data = { 1, 2, 3, 4 };");
			ADD_CODE_LINE("index::wrapped<x> index;");
			ADD_CODE_LINE("int test(int input) { index = input; return data[index]; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("test static const variable in span size argument", 1, 2);
#endif
		}
		{
#if 0
			juce::String code;

			ADD_CODE_LINE("static const int x = 4;");
			ADD_CODE_LINE("span<int, 2> data = { 1, x + 90 };");
			ADD_CODE_LINE("int test(int input) { return data[1]; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("test static const variable in span initialiser list", 1, 94);
#endif
		}
		{
			juce::String code;

			ADD_CODE_LINE("using T = int;");
			ADD_CODE_LINE("static const T x = 4;");
			ADD_CODE_LINE("int test(int input) { return x; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("test static const variable with using alias", 1, 4);
		}
		{
#if 0
			juce::String code;

			ADD_CODE_LINE("static const int x = 2;");
			ADD_CODE_LINE("static const int y = x-1;");
			ADD_CODE_LINE("span<int, x> data = { y, x };");
			ADD_CODE_LINE("int test(int input) { index::wrapped<x> index(x - 2);  return data[index]; }");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("test static const variable in span initialiser list", 10, 1);
#endif
		}
	}

	template <typename T> void testMathInlining()
	{
		auto type = Types::Helpers::getTypeFromTypeId<T>();
		beginTest(juce::String("Testing inlining of math operations for ") + juce::String(Types::Helpers::getTypeName(type)));

		auto v1 = getLiteral<T>(18);
		auto v2 = getLiteral<T>(39);

		ScopedPointer<HiseJITTestCase<T>> test;

		CREATE_TYPED_TEST(getTestFunction<T>("return Math.abs(input);"));
		EXPECT_TYPED("Math.abs", T(-25.0), T(25.0));

		

		CREATE_TYPED_TEST(getTestFunction<T>(juce::String("return Math.max(input, ") + v1 + ");"));
		EXPECT_TYPED("Math.max 1", T(121.0), T(121.0));
		EXPECT_TYPED("Math.max 2", T(13.0), T(18.0));

		CREATE_TYPED_TEST(getTestFunction<T>(juce::String("return Math.min(input, ") + v1 + ");"));
		EXPECT_TYPED("Math.min 1", T(121.0), T(18.0));
		EXPECT_TYPED("Math.min 2", T(13.0), T(13.0));

		CREATE_TYPED_TEST(getTestFunction<T>(juce::String("return Math.range(input, ") + v1 + ", " + v2 + ");"));
		EXPECT_TYPED("Math.range 1", T(12), T(18.0));
		EXPECT_TYPED("Math.range 2", T(29.0), T(29.0));
		EXPECT_TYPED("Math.range 3", T(112.0), T(39.0));

		

		CREATE_TYPED_TEST(getTestFunction<T>("return Math.sign(input);"));

		EXPECT_TYPED("Math.sign", T(25.0), T(1.0));
		EXPECT_TYPED("Math.sign", T(-25.0), T(-1.0));

		auto i1 = getLiteral<T>(4);
		auto i2 = getLiteral<T>(8);
		
		CREATE_TYPED_TEST(getTestFunction<T>(juce::String("return Math.map(input, ") + i1 + ", " + i2 + ");"));
		EXPECT_TYPED("Math.map 1", T(0), T(4));
		EXPECT_TYPED("Math.map 2", T(0.5), T(6));
		EXPECT_TYPED("Math.map 3", T(1.0), T(8));

		auto one = getLiteral<T>(1);

		CREATE_TYPED_TEST(getTestFunction<T>(juce::String("return Math.fmod(input, ") + one + ");"));
		EXPECT_TYPED("Math.fmod 1", T(3.7), T(0.7));
		EXPECT_TYPED("Math.fmod 2", T(0.2), T(0.2));
		
		CREATE_TYPED_TEST(getTestFunction<T>(juce::String("return Math.sin(input);")));
		EXPECT_TYPED("Math.sin 1", T(3.7), T(std::sin(3.7)));
		EXPECT_TYPED("Math.sin 2", T(0.2), T(std::sin(0.2)));
	}

	void testExternalStructDefinition()
	{
#if !SNEX_MIR_BACKEND
		using namespace Types;

		beginTest("Testing external struct definition");

		snex::GlobalScope memory;
		snex::Compiler c(memory);

		{
			struct OtherStruct
			{
				int v1 = 95;
				int v2 = 1;
			};

			struct TestStruct
			{
				int sum()
				{
					return m1 + (int)f2;
				}

				int m1 = 8;
				float f2 = 90.0f;
				span<float, 19> data;
				OtherStruct os;

				struct Wrapper
				{
					JIT_MEMBER_WRAPPER_0(int, TestStruct, sum);
				};
			};

			OtherStruct otherObj;

			auto os = CREATE_SNEX_STRUCT(OtherStruct);
			ADD_SNEX_STRUCT_MEMBER(os, otherObj, v1);
			ADD_SNEX_STRUCT_MEMBER(os, otherObj, v2);

			TestStruct obj;

			c.registerExternalComplexType(os);

			auto ts = CREATE_SNEX_STRUCT(TestStruct);
			ADD_SNEX_STRUCT_MEMBER(ts, obj, m1);
			ADD_SNEX_STRUCT_MEMBER(ts, obj, f2);
			ADD_SNEX_STRUCT_COMPLEX(ts, new SpanType(TypeInfo(Types::ID::Float), 19), obj, data);
			ADD_SNEX_STRUCT_COMPLEX(ts, os, obj, os);
			
			ts->addExternalMemberFunction("sum", TestStruct::Wrapper::sum);

			c.registerExternalComplexType(ts);
		}
		
		{
			auto obj = c.compileJitObject("TestStruct t; int test() { return t.m1; };");
			auto result = obj["test"].call<int>();
			expectEquals(result, 8, "External object initialisation");
		}

		{
			auto obj = c.compileJitObject("TestStruct t; float test() { return t.f2; };");
			auto result = obj["test"].call<float>();
			expectEquals(result, 90.0f, "float value");
		}

		{
			auto obj = c.compileJitObject("TestStruct t; int test() { return t.os.v1; };");
			auto result = obj["test"].call<int>();
			expectEquals(result, 95, "float value");
		}
		
		{
			auto obj = c.compileJitObject("TestStruct t; float test() { t.data[2] = t.f2; return t.data[2]; };");
			auto result = obj["test"].call<float>();
			expectEquals(result, 90.0f, "float value");
		}

		{
			auto obj = c.compileJitObject("TestStruct t; int test() { return t.sum(); };");
			auto result = obj["test"].call<int>();
			expectEquals(result, 98, "float value");
		}
#endif
	}

	void testOptimizations()
	{
		beginTest("Testing match constant function folding");

		{
			OptimizationTestCase t;
			t.setOptimizations({ OptimizationIds::ConstantFolding });
			t.setExpressionBody("float test(){ return %BODY%; }");

			expect(t.sameAssembly("(float)Math.fmod(2.1, false ? 0.333 : 1.0)", "0.1f"),
				"Math.pow with const ternary op");

			expect(t.sameAssembly("(float)Math.pow(2.0, 3.0)", "8.0f"),
				"Math.pow with cast");

			expect(t.sameAssembly("Math.max(2.0f, 1.0f)", "2.0f"),
				"Math.max");

			
		}

		beginTest("Testing Constant folding");

		{
			OptimizationTestCase t;
			t.setOptimizations({ OptimizationIds::ConstantFolding });
			t.setExpressionBody("int test(){ return %BODY%; }");

			expect(t.sameAssembly("1 && 0", "0"), "Simple logical and");

			expect(t.sameAssembly("2 + 5", "7"), "Simple addition folding");
			expect(t.sameAssembly("1 + 3 * 8", "25"), "Nested expression folding");

			auto refString = "(7 * 18 - (13 / 4) + (1 + 1)) / 8";
			constexpr int value = (7 * 18 - (13 / 4) + (1 + 1)) / 8;

			expect(t.sameAssembly(refString, juce::String(value)), "Complex expression folding");
			expect(t.sameAssembly("13 % 5", "3"), "Modulo folding");
			expect(t.sameAssembly("124 > 18", "1"), "Simple comparison folding");
			expect(t.sameAssembly("124.0f == 18.0f", "0"), "Simple equality folding");
			


			auto cExpr = "190.0f != 17.0f || (((8 - 2) < 4) && (9.0f == 0.4f))";
			constexpr int cExprValue = 190.0f != 17.0f || (((8 - 2) < 4) && (9.0f == 0.4f));

			expect(t.sameAssembly(cExpr, juce::String(cExprValue)), "Complex logical expression folding");

			
		}

		{
			OptimizationTestCase t;

			t.setOptimizations({ OptimizationIds::ConstantFolding });
			t.setExpressionBody("double test(){ return %BODY%; }");

			// We're not testing JUCE's double to String conversion here...
			expect(t.sameAssembly("2.0 * Math.FORTYTWO", "84.0"), "Math constant folding");

			expect(t.sameAssembly("1.0f > -125.0f ? 2.0 * Math.FORTYTWO : 0.4", "84.0"), "Math constant folding");
		}

		{
			OptimizationTestCase t;
			t.setOptimizations({ OptimizationIds::ConstantFolding });
			t.setExpressionBody("int test(){ int x = (int)Math.random(); %BODY% }");

			expect(t.sameAssembly("return 1 || x;", "return 1;"), "short circuit constant || expression");
			expect(t.sameAssembly("return x || 1;", "return 1;"), "short circuit constant || expression pt. 2");
			expect(t.sameAssembly("return x || 0;", "return x;"), "remove constant || sub-expression pt. 2");
			expect(t.sameAssembly("return 0 || x;", "return x;"), "remove constant || sub-expression pt. 2");
			expect(t.sameAssembly("return 0 && x;", "return 0;"), "short circuit constant || expression");
			expect(t.sameAssembly("return x && 0;", "return 0;"), "short circuit constant || expression pt. 2");
			expect(t.sameAssembly("return x && 1;", "return x;"), "remove constant || sub-expression pt. 2");
			expect(t.sameAssembly("return 1 && x;", "return x;"), "remove constant || sub-expression pt. 2");
		}

		{
			OptimizationTestCase t;
			t.setOptimizations({ OptimizationIds::ConstantFolding });
			t.setExpressionBody("int test(){ %BODY% }");

			expect(t.sameAssembly("if(0) return 2; return 1;", "return 1;"), "Constant if branch folding");
			expect(t.sameAssembly("if(12 > 13) return 8; else return 5;", "return 5;"), "Constant else branch folding");
		}

		

		beginTest("Testing binary op optimizations");

		{
			OptimizationTestCase t;
			t.setOptimizations({ OptimizationIds::BinaryOpOptimisation });
			t.setExpressionBody("void test(){ %BODY% }");

			expect(t.sameAssembly("int x = 5; int y = x; int z = 12 + y;",
								  "int x = 5; int y = x; int z = y + 12;"),
								  "Swap expressions to reuse register");

			expect(t.sameAssembly("int x = 5; int y = x - 5;",
				"int x = 5; int y = x + -5;"),
				"Replace minus");

			expect(t.sameAssembly("float z = 41.0f / 8.0f;",
								  "float z = 41.0f * 0.125f;"),
				"Replace constant division");

			expect(t.sameAssembly(
				"float x = 12.0f; x /= 4.0f;",
				"float x = 12.0f; x *= 0.25f;"),
				"Replace constant self-assign division");

			expect(t.sameAssembly(
				"int x = 12; x /= 4;",
				"int x = 12; x /= 4;"),
				"Don't replace constant int self-assign division");
		}

		beginTest("Testing complex binary op optimizations");

		{
			OptimizationTestCase t;
			t.setOptimizations({ OptimizationIds::BinaryOpOptimisation, OptimizationIds::ConstantFolding });
			t.setExpressionBody("float test(){ return %BODY%; }");

			expect(t.sameAssembly("Math.pow(2.0f, 1.0f + 1.0f)", "4.0f"),
				"constant function call -> binary op");

			expect(t.sameAssembly("((1.0f > 5.0f) ? 2.0f : 4.0f) + Math.pow(false ? 1.0f : 2.0f, 1.0f + 1.0f)", "8.0f"),
				"binary op -> constant function call -> binary op");
		}
	}

	void expectAllFunctionsDefined(JITTestModule* )
	{
	}

	template <typename T> void expectAlmostEquals(T actual, T expected, const juce::String& errorMessage)
	{
		if (Types::Helpers::getTypeFromTypeId<T>() == Types::ID::Integer)
		{
			expectEquals<int>(actual, expected, errorMessage);
		}
		else
		{
			auto diff = abs((double)actual - (double)expected);
			expect(diff < 0.0001, errorMessage);
            
            
		}
	}

	template <typename T> void testMathConstants()
	{
		beginTest("Testing math constants for " + Types::Helpers::getTypeNameFromTypeId<T>());

		ScopedPointer<HiseJITTestCase<T>> test;

		CREATE_TYPED_TEST(getTestFunction<T>("return Math.PI;"));
		EXPECT_TYPED(GET_TYPE(T) + " PI", T(), static_cast<T>(hmath::PI));

		CREATE_TYPED_TEST(getTestFunction<T>("return Math.E;"));
		EXPECT_TYPED(GET_TYPE(T) + " PI", T(), static_cast<T>(hmath::E));

		CREATE_TYPED_TEST(getTestFunction<T>("return Math.SQRT2;"));
		EXPECT_TYPED(GET_TYPE(T) + " PI", T(), static_cast<T>(hmath::SQRT2));
	}

	void testEventSetters()
	{
		using T = HiseEvent;

		ScopedPointer<HiseJITTestCase<T>> test;
		HiseEvent e(HiseEvent::Type::NoteOn, 49, 127, 10);

		CREATE_TYPED_TEST("event test(event in){in.setNoteNumber(80); return in; }");
	}

	void testAuto()
	{
		beginTest("Testing auto declarations");
		using T = int;
		ScopedPointer<HiseJITTestCase<T>> test;

		CREATE_TYPED_TEST("struct X { double v = 2.0; }; X x; int test(int input) { auto y = x.v; return (int)y; };");
		EXPECT_TYPED("auto declaration with struct member", 3, 2);

		CREATE_TYPED_TEST("int test(int input) { auto x = Math.abs((float)input); return (int)x; };");
		EXPECT_TYPED("auto declaration with parameter cast", -3, 3);

		CREATE_TYPED_TEST("int test(int input) { auto x = input > 12 ? 1.6f : 2.2f; return (int)x; };");
		EXPECT_TYPED("auto declaration with ternary op - true", 3, 2);
		EXPECT_TYPED("auto declaration with ternary op - false", 31, 1);

		CREATE_TYPED_TEST("auto x = 12; int test(int input) { return x; };");
		EXPECT_TYPED("class auto declaration", 1, 12);

		CREATE_TYPED_TEST("int test(int input) { auto x = 12; return x; };");
		EXPECT_TYPED("local auto declaration", 1, 12);

		CREATE_TYPED_TEST("int test(int input) { auto x = 12 + 1; return x; };");
		EXPECT_TYPED("local auto declaration with binary-op", 1, 13);

		CREATE_TYPED_TEST("int test(int input) { auto x = input; return x; };");
		EXPECT_TYPED("auto declaration with parameter", 4, 4);

		CREATE_TYPED_TEST("int test(int input) { auto x = (float)input; return (int)x; };");
		EXPECT_TYPED("auto declaration with parameter cast", 3, 3);

		CREATE_TYPED_TEST("int test(int input) { auto x = (float)input + 3.0f; return (int)x; };");
		EXPECT_TYPED("auto declaration with parameter cast and addition", 3, 6);

		CREATE_TYPED_TEST("span<float, 2> data = { 12.0f, 19.0f}; int test(int input) { auto x = data[1]; return (int)x; };");
		EXPECT_TYPED("auto declaration with span subscript", 3, 19);

		
	}

#if SNEX_ASMJIT_BACKEND
	void testFpu()
	{
		USE_ASMJIT_NAMESPACE;

		int ok = 0;

		JitRuntime rt;
		CodeHolder ch;

        ok = ch.init(rt.environment());


		X86Compiler cc(&ch);

		FuncSignatureX sig;
		sig.setRetT<double>();
		sig.addArgT<double>();

		auto funcNode = cc.addFunc(sig);

		auto r1 = cc.newXmmSd();
		
		auto st = cc.newStack(8, 16);
		
		st.setSize(8);

		funcNode->setArg(0, r1);

		cc.movsd(st, r1);
		cc.fld(st);
		cc.fsqrt();
		cc.fstp(st);
		cc.movsd(r1, st);
		cc.ret(r1);

		ok = cc.endFunc();
		ok = cc.finalize();

		

		void* f = nullptr;

		ok = rt.add(&f, &ch);

		expect(f != nullptr);

		using signature = double(*)(double);

		auto func = (signature)f;
		auto returnValue = func(144.0f);

		expect(returnValue == 12.0f);
	}

    void testMacOSRelocation()
    {
        USE_ASMJIT_NAMESPACE;
        
        int ok = 0;
        
        JitRuntime rt;
        CodeHolder ch;
        
        ok = ch.init(rt.environment());
        
        
        X86Compiler cc(&ch);
        
        FuncSignatureX sig;
        sig.setRetT<float>();
        sig.addArgT<float>();
        
        auto funcNode = cc.addFunc(sig);
        
        // a dummy external data location
        float x = 18.0f;
        auto xPtr = (void*)(&x);
        
        
		

#if 1
		auto rg = cc.newGpq();
        cc.mov(rg, reinterpret_cast<uint64_t>(xPtr));
		auto mem = x86::ptr(rg);
#else
		auto mem = x86::ptr(reinterpret_cast<uint64_t>(xPtr));
#endif


        auto r1 = cc.newXmmSs();
        
        //auto mem = cc.newFloatConst(ConstPool::kScopeLocal, 18.0f);
        
        funcNode->setArg(0, r1);
        ok = cc.movss(r1, mem);
        cc.ret(r1);
        
        ok = cc.endFunc();
        ok = cc.finalize();
        
        void* f = nullptr;
        
        ok = rt.add(&f, &ch);
        
        expect(f != nullptr);
        
        using signature = float(*)(float);
        
        auto func = (signature)f;
        auto returnValue = func(19.0f);
        
        expect(returnValue == 18.0f);
        
    }
#else
	void testFpu() {};
	void testMacOSRelocation() {};
#endif
    
	void testUsingAliases()
	{
		using T = int;

		beginTest("Testing using alias");

		ScopedPointer<HiseJITTestCase<T>> test;

		{
			juce::String code;

			ADD_CODE_LINE("int get(double input){ return 3;}");
			ADD_CODE_LINE("int get(float input){ return 9;}");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    using T = float;");
			ADD_CODE_LINE("    T v = (T)input;");
			ADD_CODE_LINE("    if(input > 2)");
			ADD_CODE_LINE("    {");
			ADD_CODE_LINE("        using T = double;");
			ADD_CODE_LINE("        T v = (T)input;");
			ADD_CODE_LINE("        return get(v);");
			ADD_CODE_LINE("    }");
			ADD_CODE_LINE("    return get(v);");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("function overload with anonymous scope alias true branch", 4, 3);
			EXPECT_TYPED("function overload with anonymous scope alias false branch", 1, 9);
		}

		CREATE_TYPED_TEST("using T = int; int test(int input){ float x = 2.0f; return (T)x; };");
		EXPECT_TYPED("cast with alias", 6, 2);

		CREATE_TYPED_TEST("using T = int; T test(T input){ return input; };");
		EXPECT_TYPED("native type using for function parameters", 6, 6);

		{
			juce::String code;

			ADD_CODE_LINE("int get(double input){ return 3;}");
			ADD_CODE_LINE("int get(float input){ return 9;}");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    using T = float;");
			ADD_CODE_LINE("    float v = (float)input;");
			ADD_CODE_LINE("    return get(v);");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("function overload with local alias", 4, 9);
		}

		

		{
			juce::String code;

			ADD_CODE_LINE("struct X { int v = 12; };");
			ADD_CODE_LINE("using T = X;");
			ADD_CODE_LINE("T obj = { 18 };");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    return obj.v;");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("struct using alias", 4, 18);
		}

		{
			juce::String code;

			ADD_CODE_LINE("using T = span<int, 8>;");
			ADD_CODE_LINE("T data = { 1, 2, 3, 4, 5, 6, 7, 8};");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    return data[3];");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("span using alias", 1, 4);
		}

		{
			juce::String code;

			ADD_CODE_LINE("using T = int;");
			ADD_CODE_LINE("using S = span<T, 8>;");
			ADD_CODE_LINE("S data = { 1, 2, 3, 4, 5, 6, 7, 8};");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    return data[3];");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("span using alias with type alias", 1, 4);
		}

		{
			juce::String code;

			ADD_CODE_LINE("using T = span<int, 2>;");
			ADD_CODE_LINE("using S = span<T, 2>;");
			ADD_CODE_LINE("S data = { {1,2} , {3,4} };");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    return data[1][0];");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("span using alias with type alias", 1, 3);
		}

		{
			juce::String code;

			ADD_CODE_LINE("struct X {");
			ADD_CODE_LINE("    using T = int;");
			ADD_CODE_LINE("    T value = 12;");
			ADD_CODE_LINE("};");

			ADD_CODE_LINE("using C = X;");
			ADD_CODE_LINE("C x;");
			ADD_CODE_LINE("int test(int input){");
			ADD_CODE_LINE("    return x.value;");
			ADD_CODE_LINE("}");

			CREATE_TYPED_TEST(code);
			EXPECT_TYPED("struct local using alias", 1, 12);
		}
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

		CREATE_TYPED_TEST("block& test(int in2, block in){ return in; };");

		test->setup();

		auto rb = test->func["test"].call<block*>(
			9, &bl);

		expectEquals<uint64>(reinterpret_cast<uint64>(bl.begin()), reinterpret_cast<uint64>(rb->begin()), "simple block return");


		CREATE_TYPED_TEST("float test(block in){ double x = 2.0; in[1] = Math.sin(x); return 1.0f; };");
		test->setup();
		test->func["test"].call<float>(&bl);
		expectAlmostEquals<float>(bl[1], (float)hmath::sin(2.0), "Implicit cast of function call to block assignment");

		CREATE_TYPED_TEST("int v = 0; int test(block in) { for(auto& s: in) v += 1; return v; }");
		test->setup();
		auto numSamples2 = test->func["test"].call<int>(&bl);
		expectEquals<int>(numSamples2, bl.size(), "Counting samples in block");

		for (int i = 0; i < b.getNumSamples(); i++)
		{
			b.setSample(0, i, (float)(i + 1));
		}

		auto t = reinterpret_cast<uint64_t>(b.getWritePointer(0));
        ignoreUnused(t);

		CREATE_TYPED_TEST("int test(block in){ return in.size(); };");
		test->setup();
		auto s_ = test->func["test"].call<int>(&bl);
        
        ignoreUnused(s_);

		//expectEquals( s_, 512, "size() operator");


		

		CREATE_TYPED_TEST("block test(int in2, block in){ return in; };");

		CREATE_TYPED_TEST("float test(block in){ in[4] = 124.0f; return 1.0f; };");
		test->setup();
		
		test->func["test"].call<float>(&bl);
		
		expectEquals<float>(bl[4], 124.0f, "Setting block value");

		CREATE_TYPED_TEST("float v = 0.0f; float test(block in) { for(auto& s: in) v = s; return v; }");
		test->setup();
		auto numSamples3 = test->func["test"].call<float>(&bl);
		expectEquals<int>(numSamples3, (float)bl.size(), "read block value into global variable");


		CREATE_TYPED_TEST("int v = 0; int test(block in) { for(auto& s: in) v = s; return v; }");
		test->setup();
		auto numSamples4 = test->func["test"].call<int>(&bl);
		expectEquals<int>(numSamples4, bl.size(), "read block value with cast");

		b.clear();

        CREATE_TYPED_TEST("float test(block in){ in[in.index<block::unsafe>(1)] = Math.abs(in, 124.0f); return 1.0f; };");
        test->setup();
        test->func["test"].call<float>(&bl);
        expectEquals<float>(bl[1], 0.0f, "Calling function with wrong signature as block assignment");

        CREATE_TYPED_TEST("float test(block in){ double x = 2.0; in[1] = Math.sin(x); return 1.0f; };");
        test->setup();
        test->func["test"].call<float>(&bl);
		expectAlmostEquals<float>(bl[1], (float)hmath::sin(2.0), "Implicit cast of function call to block assignment");
        
		
		bl[0] = 0.86f;
		bl2[128] = 0.92f;

		CREATE_TYPED_TEST("float test(block in, block in2){ int idx = 0; return in[idx] + in2[idx + 128]; };");

		test->setup();
		auto rb2 = test->func["test"].call<float>(&bl, &bl2);
		expectEquals<float>(rb2, 0.86f + 0.92f, "Adding two block values");

		CREATE_TYPED_TEST("float test(block in){ in[1] = 124.0f; return 1.0f; };");
		test->setup();
		test->func["test"].call<float>(&bl);
		expectEquals<float>(bl[1], 124.0f, "Setting block value");
        
		

        CREATE_TYPED_TEST("float l = 1.94f; float test(block in){ for(auto& s: in) s = 2.4f; for(auto& s: in) l = s; return l; }");
        test->setup();
        auto shouldBe24 = test->func["test"].call<float>(&bl);
        expectEquals<float>(shouldBe24, 2.4f, "Setting global variable in block loop");
        
		CREATE_TYPED_TEST("int v = 0; int test(block in) { for(auto& s: in) v += 1; return v; }");
		test->setup();
		auto numSamples = test->func["test"].call<int>(&bl);
		expectEquals<int>(numSamples, bl.size(), "Counting samples in block");
        
		CREATE_TYPED_TEST("void test(block in){ for(auto& sample: in){ sample = 2.0f; }}");
		test->setup();

		auto f = test->func["test"];
		
		f.callVoid(&bl);

		for (int i = 0; i < bl.size(); i++)
		{
			expectEquals<float>(bl[i], 2.0f, "Setting all values");
		}
	}

	void testEvents()
	{
		beginTest("Testing HiseEvents in JIT");

		using Event2IntTest = HiseJITTestCase<HiseEvent*, int>;

		HiseEvent testEvent = HiseEvent(HiseEvent::Type::NoteOn, 59, 127, 1);
		HiseEvent testEvent2 = HiseEvent(HiseEvent::Type::NoteOff, 59, 127, 1);
		testEvent2.setTimeStamp(32);
		
		ScopedPointer<Event2IntTest> test;

		test = new Event2IntTest("int test(HiseEvent& in){ return in.getNoteNumber(); }", optimizations);
		EXPECT("getNoteNumber", &testEvent, 59);

		test = new Event2IntTest("int test(HiseEvent& in){ return in.getNoteNumber() > 64 ? 17 : 13; }", optimizations);
		EXPECT("getNoteNumber arithmetic", &testEvent, 13);

		test = new Event2IntTest("int test(HiseEvent& in1, HiseEvent& in2){ return in1.getNoteNumber() > in2.getNoteNumber() ? 17 : 13; }", optimizations);

		{
			juce::String code;

			ADD_CODE_LINE("void change(HiseEvent& e) { e.setVelocity(40); };");
			ADD_CODE_LINE("int test(HiseEvent& in){ change(in); return in.getVelocity();}");

			test = new Event2IntTest(code, optimizations);

			HiseEvent testEvent2 = HiseEvent(HiseEvent::Type::NoteOn, 59, 127, 1);

			test->getResult(&testEvent2, 40);

			EXPECT("change velocity in function", &testEvent2, 40);

			expectEquals<int>(testEvent2.getVelocity(), 40, "reference change worked");

		}

		{
			HiseEventBuffer buffer;

			buffer.addEvent(testEvent);
			buffer.addEvent(testEvent2);

			Types::dyn<HiseEvent> d = { buffer.begin(), (size_t)buffer.getNumUsed() };

			juce::String code;

			ADD_CODE_LINE("int test(dyn<HiseEvent>& in){ int x = 0; for(auto& e: in) x += e.getNoteNumber(); return x;}");

			GlobalScope memory;
			Compiler c(memory);
			Types::SnexObjectDatabase::registerObjects(c, 2);

			auto obj = c.compileJitObject(code);
			auto result = obj["test"].call<int>(&d);

			expectEquals<int>(result, 118, "event buffer iteration");

		}
	}

	void testParser()
	{
		beginTest("Testing Parser");

		ScopedPointer<HiseJITTestCase<int>> test;

		test = new HiseJITTestCase<int>("float x = 1.0f;;", optimizations);
		expectCompileOK(test->compiler);
	}

	void testSimpleIntOperations()
	{
		beginTest("Testing simple integer operations");

		ScopedPointer<HiseJITTestCase<int>> test;

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { x++; return x; }", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("post int increment", 0, 13);

		test = new HiseJITTestCase<int>("int x = 0; int test(int input){ x = input; return x;};", optimizations);
		expectCompileOK(test->compiler);

		EXPECT("int assignment", 6, 6);
		
		test = new HiseJITTestCase<int>("int other() { return 2; }; int test(int input) { return other(); }", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("reuse double assignment", 0, 2);

		test = new HiseJITTestCase<int>("int test(int input) { int x = 5; int y = x; int z = y + 12; return z; }", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("reuse double assignment", 0, 17);

		test = new HiseJITTestCase<int>("int test(int input){ input += 3; return input;};", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("add-assign to input parameter", 2, 5);

		test = new HiseJITTestCase<int>("int test(int input){ int x = 6; return x;};", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("local int variable", 0, 6);

		test = new HiseJITTestCase<int>("int test(int input){ int x = 6; return x;};", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("local int variable", 0, 6);

		test = new HiseJITTestCase<int>("int x = 0; int test(int input){ x = input; return x;};", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("int assignment", 6, 6);

		test = new HiseJITTestCase<int>("int x = 2; int test(int input){ x = -5; return x;};", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("negative int assignment", 0, -5);

		

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { return x++; }", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("post increment as return", 0, 12);

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { ++x; return x; }", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("post int increment", 0, 13);

		test = new HiseJITTestCase<int>("int x = 12; int test(int in) { return ++x; }", optimizations);
		expectCompileOK(test->compiler);
		EXPECT("post increment as return", 0, 13);
	}

	void testScopes()
	{
		beginTest("Testing variable scopes");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float test(float in) { float x = 8.0f; float y = 0.0f; { float x = x + 9.0f; y = x; } return y; }");
		expectCompileOK(test->compiler);
		EXPECT("Save scoped variable to local variable", 12.0f, 17.0f);

		CREATE_TEST("float test(float in) {{return 2.0f;}}; ");
		expectCompileOK(test->compiler);
		EXPECT("Empty scope", 12.0f, 2.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) { float x = x; x *= 1000.0f;  return x; }");
		expectCompileOK(test->compiler);
		EXPECT("Overwrite with local variable", 12.0f, 1000.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) {{ float x = x; x *= 1000.0f; } return x; }");
		expectCompileOK(test->compiler);
		EXPECT("Overwrite with local variable", 12.0f, 1.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) {{ x *= 1000.0f; } return x; }");
		expectCompileOK(test->compiler);
		EXPECT("Change global in sub scope", 12.0f, 1000.0f);

		CREATE_TEST("float test(float input){ float x1 = 12.0f; float x2 = 12.0f; float x3 = 12.0f; float x4 = 12.0f; float x5 = 12.0f; float x6 = 12.0f; float x7 = 12.0f;float x8 = 12.0f; float x9 = 12.0f; float x10 = 12.0f; float x11 = 12.0f; float x12 = 12.0f; return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10 + x11 + x12; }");
		expectCompileOK(test->compiler);
		EXPECT("12 variables", 12.0f, 144.0f);

		
	}

	void testLogicalOperations()
	{
		beginTest("Testing logic operations");

		ScopedPointer<HiseJITTestCase<float>> test;

		
		CREATE_TEST("float x = 1.0f; int change() { x = 5.0f; return 1; } float test(float in){ int c = change(); 0 && c; return x;}");
		expectCompileOK(test->compiler);
		EXPECT("Don't short circuit variable expression with &&", 12.0f, 5.0f);

		CREATE_TEST("float test(float i){ if(i > 0.5) return 10.0f; else return 5.0f; };");
		expectCompileOK(test->compiler);
		EXPECT("Compare with cast", 0.2f, 5.0f);

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

		{
			float delta = 0.0f;
			const float x = 200.0f;
			const float y = x / 44100.0f;
			delta = 2.0f * 3.14f * y;

			CREATE_TEST("float delta = 0.0f; float test(float input) { float y = 200.0f / 44100.0f; delta = 2.0f * 3.14f * y; return delta; }");

			EXPECT("Reusing of local variable", 0.0f, delta);
		}

        CREATE_TEST("float x=2.0f; void setup() { x = 5.0f; } float test(float i){return x;};")
        expectCompileOK(test->compiler);
        EXPECT("Global set in other function", 2.0f, 5.0f);
        

		
		

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

		CREATE_TEST("float x = 2.0f; float getX(){ return x; } float test(float input) { x = input; return getX();}")
		EXPECT("Set global variable before function call", 5.0f, 5.0f);

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


		CREATE_TEST("float g = 0.0f; void setup() { float x = 1.0f; g = x + 2.0f * x; } float test(float i){return g;}")
			expectCompileOK(test->compiler);
		EXPECT("Don't reuse local variable slot", 2.0f, 3.0f);

	}

	template <typename T> juce::String getTypeName() const
	{
		return Types::Helpers::getTypeNameFromTypeId<T>();
	}

	template <typename T> juce::String getTestSignature()
	{
		return getTypeName<T>() + " test(" + getTypeName<T>() + " input){%BODY%};";
	}

	template <typename T> juce::String getTestFunction(const juce::String& body)
	{
		auto x = getTestSignature<T>();
		return x.replace("%BODY%", body);
	}

	template <typename T> juce::String getLiteral(double value)
	{
		VariableStorage v(Types::Helpers::getTypeFromTypeId<T>(), value);

		return Types::Helpers::getCppValueString(v);
	}

	template <typename T> juce::String getGlobalDefinition(double value)
	{
		auto valueString = getLiteral<T>(value);

		return getTypeName<T>() + " x = " + valueString + ";";
	}

	void testStructs()
	{
		beginTest("Testing structs");

		using T = int;

		ScopedPointer<HiseJITTestCase<int>> test;

		CREATE_TYPED_TEST("struct X { span<int, 2> data = {7, 9}; }; X x; int test(int input) { return x.data[0] + input; };");
		expectCompileOK(test->compiler);
		EXPECT("span member access", 7, 14);

		CREATE_TYPED_TEST("struct X { int value = 3; int get() { return value; } }; X x1; X x2; int test(int input) { x1.value = 8; x2.value = 9; return x1.get() + x2.get(); }");
		expectCompileOK(test->compiler);
		EXPECT("two instances set value", 7, 8 + 9);

		CREATE_TYPED_TEST("struct X { int x = 3; int getX() { return x; } }; X x; int test(int input) { return x.getX(); };");
		expectCompileOK(test->compiler);
		EXPECT("member variable with instance id", 0, 3);


		CREATE_TYPED_TEST("struct X { int u = 2; int v = 3; int getX() { return v; } }; X x; int test(int input) { return x.getX(); };");
		expectCompileOK(test->compiler);
		EXPECT("member variable", 0, 3);

#if 0
		CREATE_TYPED_TEST("struct X { double z = 12.0; int value = 3; }; X x1; X x2; int test(int input) { X& ref = x2; return ref.value; };");
		expectCompileOK(test->compiler);
		EXPECT("struct ref", 7, 3);
#endif

		CREATE_TYPED_TEST("struct X { int x = 3; }; span<X, 3> d; int test(int input) { return d[0].x + input; };");
		expectCompileOK(test->compiler);
		EXPECT("span of structs", 7, 10);
		
		CREATE_TYPED_TEST("struct X { struct Y{ span<int, 2> data = {7, 9};}; Y y; }; X x; int test(int input) { return x.y.data[0] + input; };");
		expectCompileOK(test->compiler);
		EXPECT("span member access", 7, 14);

		CREATE_TYPED_TEST("struct X { int value = 3; double v2 = 8.0; }; X x; int test(int input) { return (int)x.v2 + input; };");
		expectCompileOK(test->compiler);
		EXPECT("unaligned double member access", 7, 15);

		CREATE_TYPED_TEST("struct X { int value = 5; int getX() { return value; } }; X x; int test(int input) { return x.getX(); }");
		expectCompileOK(test->compiler);
		EXPECT("simple struct getter method", 7, 5);

		CREATE_TYPED_TEST("struct X { int value = 5; }; X x; int test(int input) { return x.value; }");
		expectCompileOK(test->compiler);
		EXPECT("simple struct access", 7, 5);

		CREATE_TYPED_TEST("struct X { int value = 5; }; X x; int test(int input) { x.value = input * 2; return x.value; }");
		expectCompileOK(test->compiler);
		EXPECT("simple struct member set", 7, 14);

		CREATE_TYPED_TEST("struct X { int value = 5; void set(int v) { value = v; } }; X x; int test(int input) { x.set(input); return x.value * 3; }");
		expectCompileOK(test->compiler);
		EXPECT("simple struct setter method", 7, 21);

		CREATE_TYPED_TEST("struct X { struct Y { int value = 19; }; Y y; }; X x; int test(int input) { return x.y.value; }");
		expectCompileOK(test->compiler);
		EXPECT("nested struct member access", 7, 19);

		CREATE_TYPED_TEST("struct X { struct Y { int value = 19; }; Y y; }; X x; int test(int input) { x.y.value = input + 5; int v = x.y.value; return v; }");
		expectCompileOK(test->compiler);
		EXPECT("nested struct member setter", 7, 12);

		

		
	}

	template <typename T> void testPointerVariables()
	{
		auto type = Types::Helpers::getTypeFromTypeId<T>();

		beginTest("Testing pointer variables for " + Types::Helpers::getTypeName(type));

		Random r;

		double a = (double)r.nextInt(25) *(r.nextBool() ? 1.0 : -1.0);
		double b = (double)r.nextInt(25) *(r.nextBool() ? 1.0 : -1.0);

		ScopedPointer<HiseJITTestCase<T>> test;

#define CREATE_POINTER_FUNCTION_TEMPLATE(codeAfterRefDefiniton) CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){" + getTypeName<T>() + "& ref = x; " + codeAfterRefDefiniton + "}");

		CREATE_POINTER_FUNCTION_TEMPLATE("ref += input; return x;");
		EXPECT_TYPED(GET_TYPE(T) + " Adding input to reference variable", b, a + b);

		CREATE_POINTER_FUNCTION_TEMPLATE("ref = input; return x;");
		EXPECT_TYPED(GET_TYPE(T) + " Setting reference variable", a, a);


		

		CREATE_POINTER_FUNCTION_TEMPLATE("ref += input; return ref;");
		EXPECT_TYPED(GET_TYPE(T) + " Adding input to reference variable", b, a + b);

		CREATE_POINTER_FUNCTION_TEMPLATE("ref += input; return x;");
		EXPECT_TYPED(GET_TYPE(T) + " Adding input to reference variable", b, a + b);

#undef CREATE_POINTER_FUNCTION_TEMPLATE
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

        CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " * " T_B ";"));
        EXPECT_TYPED(GET_TYPE(T) + " Multiplication", T(), (T)a*(T)b);
        
        CREATE_TYPED_TEST(getGlobalDefinition<T>(a) + getTypeName<T>() + " test(" + getTypeName<T>() + " input){ x *= " T_B "; return x;};");
        EXPECT_TYPED(GET_TYPE(T) + " *= operator", T(), (T)a * (T)b);
        
		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " > " T_B " ? " T_1 ":" T_0 ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Conditional", T(), (T)(a > b ? 1 : 0));

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " + " T_B ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Addition", T(), (T)(a + b));

		CREATE_TYPED_TEST(getTestFunction<T>("return " T_A " - " T_B ";"));
		EXPECT_TYPED(GET_TYPE(T) + " Subtraction", T(), (T)(a - b));

		

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

		juce::String s;
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


        

		CREATE_TEST("struct X { struct Y { int u = 8; int v = 12; int getV() { return v; }}; Y y; }; X x; float test(float input){ return x.y.getV() + input;");

		EXPECT("inner struct member call", 8.0f, 20.0f);

		CREATE_TEST("struct X { int u = 8; float v = 12.0f; float getV() { return v; }}; span<X, 3> d; float test(float input){ return d[1].getV() + input;");
		EXPECT("span of structs struct member call", 8.0f, 20.0f);

		CREATE_TEST("float t2(float a){ return a * 2.0f; } float t1(float b){ return t2(b) + 5.0f; } float test(float input) { return t1(input); }");
		EXPECT("nested function call", 2.0f, 9.0f);

		CREATE_TEST("int sumThemAll(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8){ return i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8; } float test(float in) { return (float)sumThemAll(1, 2, 3, 4, 5, 6, 7, 8); }");
		EXPECT("Function call with 8 parameters", 20.0f, 36.0f);

		CREATE_TEST("float ov(int a){ return 9.0f; } float ov(double a) { return 14.0f; } float test(float input) { return ov(5); }");
		EXPECT("function overloading", 2.0f, 9.0f);

		

		CREATE_TEST("float other(float input) { return input * 2.0f; } float test(float input) { return other(input); }");
		EXPECT("root class function call", 8.0f, 16.0f);

		CREATE_TEST("float test(float input) { return Math.abs(input); }");
		EXPECT("simple math API call", -5.0f, 5.0f);

		CREATE_TEST("struct X { int u = 8; float v = 12.0f; float getV() { return v; }}; X x; float test(float input){ return x.getV() + input;");
		EXPECT("struct member call", 8.0f, 20.0f);

		

		CREATE_TEST("struct X { struct Y { int u = 8; float v = 12.0f; float getV() { return v; } }; Y y; float getY() { return y.getV();} }; X x; float test(float input){ return x.getY() + input;");
		EXPECT("nested struct member call", 8.0f, 20.0f);

		CREATE_TEST("struct X { int u = 8; float v = 12.0f; float getV() { return v; }}; span<X, 3> d; float test(float input){ return d[1].getV() + input;");
		EXPECT("span of structs struct member call", 8.0f, 20.0f);
		
		CREATE_TEST("struct X { struct Y { int u = 8; float v = 12.0f; float getV() { return v; }}; Y y; }; span<X, 3> d; float test(float input){ return d[1].y.getV() + input;");
		EXPECT("span of structs inner struct member call", 8.0f, 20.0f);

		CREATE_TEST("struct X { struct Y { int u = 8; float v = 12.0f; float getV() { return v; }}; span<Y, 3> y; }; X x; float test(float input){ return x.y[1].getV() + input;");
		EXPECT("struct inner span member call", 8.0f, 20.0f);

		CREATE_TEST("struct X { struct Y { int u = 8; float v = 12.0f; float getV() { return v; }}; span<Y, 3> y; }; span<X, 3> d; float test(float input){ return d[1].y[1].getV() + input;");
		EXPECT("struct inner span member call", 8.0f, 20.0f);

        CREATE_TEST("float ov(int a){ return 9.0f; } float ov(double a) { return 14.0f; } float test(float input) { return ov(5); }");
        EXPECT("function overloading", 2.0f, 9.0f);
        
		Random r;

		const float v = r.nextFloat() * 122.0f * (r.nextBool() ? 1.0f : -1.0f);

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

		juce::String x;
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

		

	}

	void testDoubleFunctionCalls()
	{
		beginTest("Double Function Calls");

		ScopedPointer<HiseJITTestCase<double>> test;

#define T double

		Random r;

		const double v = (double)(r.nextFloat() * 122.0f * (r.nextBool() ? 1.0f : -1.0f));


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
		EXPECT_TYPED("atanh", 0.6, atanh(0.6));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.pow(input, 2.0);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("pow", v, pow(v, 2.0));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.sqrt(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("sqrt", fabs(v), sqrt(fabs(v))); // No negative square root

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.abs(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("fabs", v, fabs(v));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.map(input, 10.0, 20.0);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("map", 0.5, jmap(0.5, 10.0, 20.0));

		CREATE_TYPED_TEST(getTestFunction<double>("return Math.exp(input);"))
			expectCompileOK(test->compiler);
		EXPECT_TYPED("exp", v, exp(v));

#undef T

	}

	void testIfStatement()
	{
		beginTest("Test if-statement");

		ScopedPointer<HiseJITTestCase<float>> test;

		CREATE_TEST("float test(float input){ if (input == 12.0f) return 1.0f; else return 2.0f;");
		expectCompileOK(test->compiler);
		EXPECT("If statement as last statement", 12.0f, 1.0f);
		EXPECT("If statement as last statement, false branch", 9.0f, 2.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) { if (input == 10.0f) x += 1.0f; else x += 2.0f; return x; }");
		EXPECT("Set global variable, true branch", 10.0f, 2.0f);
		EXPECT("Set global variable, false branch", 12.0f, 4.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) { if (input == 10.0f) x += 12.0f; return x; }");
		EXPECT("Set global variable in true branch, false branch", 9.0f, 1.0f);
		EXPECT("Set global variable in true branch", 10.0f, 13.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) { if (input == 10.0f) return 2.0f; else x += 12.0f; return x; }");
		EXPECT("Set global variable in false branch, true branch", 10.0f, 2.0f);
		EXPECT("Set global variable in false branch", 12.0f, 13.0f);

		CREATE_TEST("float test(float input){ if(input > 1.0f) return 10.0f; return 2.0f; }");
		EXPECT("True branch", 4.0f, 10.0f);
		EXPECT("Fall through", 0.5f, 2.0f);

		CREATE_TEST("float x = 1.0f; float test(float input) { x = 1.0f; if (input < -0.5f) x = 12.0f; return x; }");
		EXPECT("Set global variable in true branch after memory load, false branch", 9.0f, 1.0f);
		EXPECT("Set global variable in true branch after memory load", -10.0f, 12.0f);
		
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

	template <typename OpType, int SizeA, int SizeB> void testSpanOperatorWith2Spans()
	{
		Random r;

		span<float, SizeA> d1;
		span<float, SizeB> d2;

		for (auto& s : d1)
			s = r.nextFloat();
		for (auto& s : d2)
			s = r.nextFloat();

		span<float, SizeA> e1;
		span<float, SizeB> e2;

		FloatVectorOperations::copy(e2.begin(), d2.begin(), SizeB);
		FloatVectorOperations::copy(e1.begin(), d1.begin(), SizeA);

		d1.template performOp<OpType>(d2);

		for (int i = 0; i < SizeA; i++)
		{
			auto srcIndex = jmin(SizeB - 1, i);

			OpType::op(e1[i], e2[srcIndex]);
			expectEquals<float>(d1[i], e1[i], "not equal");
		}
	}

	template <typename OpType, int SizeA> void testSpanOperatorWithScalar()
	{
		Random r;

		span<float, SizeA> d1;
		
		float s = r.nextFloat();

		for (auto& s : d1)
			s = r.nextFloat();
		
		span<float, SizeA> e1;
		
		FloatVectorOperations::copy(e1.begin(), d1.begin(), SizeA);

		d1.template performOp<OpType>(s);

		for (int i = 0; i < SizeA; i++)
		{
			OpType::op(e1[i], s);
			expectEquals<float>(d1[i], e1[i], "not equal");
		}

	}

	template <typename OpType> void testSpanOperator()
	{
		testSpanOperatorWithScalar<OpType, 2>();
		testSpanOperatorWithScalar<OpType, 4>();
		testSpanOperatorWithScalar<OpType, 1>();
		testSpanOperatorWithScalar<OpType, 18>();
		testSpanOperatorWithScalar<OpType, 17>();
		testSpanOperatorWithScalar<OpType, 32>();
		testSpanOperatorWithScalar<OpType, 256>();

		testSpanOperatorWith2Spans<OpType, 8, 4>();
		testSpanOperatorWith2Spans<OpType, 4, 4>();
		testSpanOperatorWith2Spans<OpType, 4, 8>();
		testSpanOperatorWith2Spans<OpType, 16, 3>();
		testSpanOperatorWith2Spans<OpType, 3, 16>();
		testSpanOperatorWith2Spans<OpType, 128, 256>();
		testSpanOperatorWith2Spans<OpType, 128, 47>();
		testSpanOperatorWith2Spans<OpType, 128, 1>();
		testSpanOperatorWith2Spans<OpType, 1, 128>();
		testSpanOperatorWith2Spans<OpType, 21, 16>();
		testSpanOperatorWith2Spans<OpType, 2, 3>();
		testSpanOperatorWith2Spans<OpType, 3, 2>();
		testSpanOperatorWith2Spans<OpType, 3, 3>();
		testSpanOperatorWith2Spans<OpType, 1, 3>();
		testSpanOperatorWith2Spans<OpType, 3, 1>();
		testSpanOperatorWith2Spans<OpType, 1, 1>();
	}

	void testSpanOperators()
	{
		beginTest("Testing span operators");

		span<float, 2> d1 = { 5.0f, 6.0f };
		span<float, 3> d2 = { 5.0f, 6.0f };
		auto s = 0.8f;
		expectEquals<float>(d1[0], 5.0f, "init assign1");
		expectEquals<float>(d1[1], 6.0f, "init assign2");

		// These will fire compile errors
		d1 *= d2;
		d1 += d2;
		d1 /= d2;
		d1 = d2;
		d1 -= d2;

		d1 *= s;
		d1 += s;
		d1 /= s;
		d1 -= s;
		d1 = s;

		testSpanOperator<SpanOperators<float>::add>();
		testSpanOperator<SpanOperators<float>::multiply>();
		testSpanOperator<SpanOperators<float>::divide>();
		testSpanOperator<SpanOperators<float>::sub>();
		testSpanOperator<SpanOperators<float>::assign>();

		span<span<float, 4>, 2> md;
		span<float, 4> sd = { 1.0 };

		span<float, 4> mul = { 1.0, 2.0, 3.0, 4.0 };

		md = sd;
		md += sd;

		md *= mul;

	}

	void testBigFunctionBuffer()
	{
		beginTest("Testing big function buffer");

		juce::String code;

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

#endif

	StringArray optimizations;
};


static HiseJITUnitTest njut;


#undef CREATE_TEST
#undef CREATE_TEST_SETUP
#undef EXPECT

#pragma warning( pop)

}}

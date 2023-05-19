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


#define TEST_ALL_INDEXES 1

template <typename IndexType> struct IndexTester
{
	using Type = typename IndexType::Type;
	static constexpr int Limit = IndexType::LogicType::getUpperLimit();

	static constexpr bool isLoopTest()
	{
		return std::is_same<index::looped_logic<Limit>, typename IndexType::LogicType>();
	}

	IndexTester(UnitTest* test_, StringArray opt, int dynamicSize = 0) :
		test(*test_),
		indexName(IndexType::toString()),
		optimisations(opt),
		ArraySize(Limit != 0 ? Limit : dynamicSize)
	{
		runTest();
	}



private:

	const int ArraySize;

	const String indexName;

	void runTest()
	{
		testLoopRange(0.0, 0.0);
		testLoopRange(0.0, 1.0);
		testLoopRange(0.5, 1.0);
		testLoopRange(0.3, 0.6);

#if TEST_ALL_INDEXES
		testIncrementors(FunctionClass::SpecialSymbols::IncOverload);
		testIncrementors(FunctionClass::SpecialSymbols::DecOverload);
		testIncrementors(FunctionClass::SpecialSymbols::PostIncOverload);
		testIncrementors(FunctionClass::SpecialSymbols::PostDecOverload);
		testAssignAndCast();
		testFloatAlphaAndIndex();
		testSpanAccess();
		testDynAccess();
#endif
		testInterpolators();
	}

	Range<int> getLoopRange(double nStart, double nEnd)
	{
		auto s = roundToInt(jlimit(0.0, 1.0, nStart) * (double)Limit);
		auto e = roundToInt(jlimit(0.0, 1.0, nEnd) * (double)Limit);

		return Range<int>(s, e);
	}

	String getLoopRangeCode(double start, double end)
	{
		auto l = getLoopRange(start, end);

		String c;
		c << ".setLoopRange(" << l.getStart() << ", " << l.getEnd() << ");";
		return c;
	}

	void testLoopRange(double normalisedStart, double normalisedEnd)
	{
		if constexpr (isLoopTest() && IndexType::LogicType::hasBoundCheck())
		{
			cppgen::Base c(cppgen::Base::OutputType::AddTabs);

			span<Type, Limit> data;
			String spanCode;

			initialiseSpan(spanCode, data);

			c << indexName << " i;";
			c << spanCode;
			c << "T test(T input)";
			{
				cppgen::StatementBlock sb(c);
				c << "i" << getLoopRangeCode(normalisedStart, normalisedEnd);
				c << "i = input;";
				c << "return data[i];";
			}

			test.logMessage("Testing loop range " + indexName + getLoopRangeCode(normalisedStart, normalisedEnd));

			c.replaceWildcard("T", Types::Helpers::getTypeNameFromTypeId<Type>());

			auto obj = compile(c.toString());

			// Test Routine ==============================================

			auto testWithValue = [&](Type testValue)
			{
				IndexType i;
				auto lr = getLoopRange(normalisedStart, normalisedEnd);
				i.setLoopRange(lr.getStart(), lr.getEnd());
				i = testValue;

				auto expected = data[i];
				auto actual = obj["test"].template call<Type>(testValue);
				String message = indexName;

				message << " with value " << String(testValue);
				test.expectWithinAbsoluteError(actual, expected, Type(0.0001), message);
			};

			// Test List =======================================================


			testWithValue(Type(0.5));
#if SNEX_WRAP_ALL_NEGATIVE_INDEXES
			testWithValue(Type(-1.5));
#endif
			testWithValue(Type(20.0));
			testWithValue(Type(-1));
			testWithValue(Type(Limit * 0.99));
			testWithValue(Type(Limit * 1.2));
			testWithValue(Type(Limit * 141.2));
			testWithValue(Type(Limit * 8141.92));
			testWithValue(Type(0.3));
			testWithValue(Type(8.0));
			testWithValue(Type(Limit / 3));
		}
	}

	void testInterpolators()
	{
		if constexpr (!IndexType::canReturnReference() && IndexType::LogicType::hasBoundCheck())
		{
			// Test Code ===================================================

			cppgen::Base c(cppgen::Base::OutputType::AddTabs);

			span<Type, Limit> data;

			String spanCode;

			initialiseSpan(spanCode, data);

			c << indexName + " i;";
			c << spanCode;
			c << "T test(T input)";

			{
				cppgen::StatementBlock sb(c);
				c << "i = input;";
				c << "i.setLoopRange(0, 0);";
				c << "return data[i];";
			}

			test.logMessage("Testing interpolator " + indexName);

			c.replaceWildcard("T", Types::Helpers::getTypeNameFromTypeId<Type>());

			auto obj = compile(c.toString());

			// Test Routine ==============================================

			auto testWithValue = [&](Type testValue)
			{
				IndexType i;
				i = testValue;
				auto expected = data[i];
				auto actual = obj["test"].template call<Type>(testValue);
				String message = indexName;

				message << " with value " << String(testValue);
				test.expectWithinAbsoluteError(actual, expected, Type(0.0001), message);
			};

			// Test List =======================================================

			testWithValue(Type(0.5));
#if SNEX_WRAP_ALL_NEGATIVE_INDEXES
			testWithValue(Type(-1.5));
#endif
			testWithValue(Type(20.0));
			testWithValue(Type(Limit * 0.99));
			testWithValue(Type(Limit * 1.2));
			testWithValue(Type(0.3));
			testWithValue(Type(8.0));
			testWithValue(Type(Limit / 3));
		}
	}

	void testSpanAccess()
	{
		if constexpr (Limit != 0 && !isInterpolator())
		{
			// Test Code ===================================================

			cppgen::Base c(cppgen::Base::OutputType::AddTabs);

			span<int, Limit> data;
			String spanCode;

			initialiseSpan(spanCode, data);

			c << spanCode;
			c << indexName + " i;";
			c << "int test(T input)";

			{
				cppgen::StatementBlock sb(c);
				c.addWithSemicolon("i = input;");
				c.addWithSemicolon("return data[i];");
			}

			c << "int test2(T input)";
			{
				cppgen::StatementBlock sb(c);
				c << "i = input;";
				c << "data[i] = (T)50;";
				c << "return data[i];";
			}

			//test.logMessage("Testing " + indexName + " span[]");

			c.replaceWildcard("T", Types::Helpers::getTypeNameFromTypeId<Type>());
			auto obj = compile(c.toString());

			// Test Routine ==============================================

			auto testWithValue = [&](Type testValue)
			{
				if (IndexType::LogicType::hasBoundCheck())
				{
					IndexType i;
					i = testValue;

					auto expectedValue = data[i];
					auto actualValue = obj["test"].template call<int>(testValue);

					String m = indexName;
					m << "::operator[]";
					m << " with value " << String(testValue);

					test.expectEquals(actualValue, expectedValue, m);

					data[i] = Type(50);
					auto e2 = data[i];

					auto a2 = obj["test2"].template call<int>(testValue);

					m << "(write access)";

					test.expectEquals(e2, a2, m);

				}
			};

			// Test List =======================================================

			if (std::is_floating_point<Type>())
			{
				testWithValue(0.5);
				testWithValue(Limit + 0.5);
				testWithValue(Limit / 3.f);
				testWithValue(-0.5 * Limit);
			}
			else
			{
				testWithValue(80);
				testWithValue(Limit);
				testWithValue(Limit - 1);
				testWithValue(-1);
				testWithValue(0);
				testWithValue(1);
				testWithValue(Limit + 1);
				testWithValue(-Limit + 1);
			}
		}
	}

	void testDynAccess()
	{
		if constexpr (!isInterpolator())
		{
            if(ArraySize == 0)
                return;
            
			// Test Code ===================================================

			heap<int> data;

			data.setSize(ArraySize);

			cppgen::Base c(cppgen::Base::OutputType::AddTabs);

			String spanCode;

			initialiseSpan(spanCode, data);

			dyn<int> d;
			d.referTo(data);

			c << spanCode;
			c << "dyn<int> d;";
			c << indexName + " i;";
			c << "int test(XXX input)";

			{
				cppgen::StatementBlock sb(c);

				c << "d.referTo(data, data.size());";
				c << "i = input;";
				c << "return d[i];";
			}

			//test.logMessage("Testing " + indexName + " dyn[]");

			c.replaceWildcard("XXX", Types::Helpers::getTypeNameFromTypeId<Type>());
			auto obj = compile(c.toString());

			// Test Routine ==============================================

			auto testWithValue = [&](Type testValue)
			{
				if (IndexType::LogicType::hasBoundCheck())
				{
					
					IndexType i;
					i = testValue;

					auto expectedValue = d[i];
					auto actualValue = obj["test"].template call<int>(testValue);

					String m = indexName;
					m << "::operator[]";
					m << "(dyn) with value " << String(testValue);

					test.expectEquals(actualValue, expectedValue, m);
				}
				else
				{
					test.logMessage("skip [] access for unsafe index");
				}
			};

			// Test List =======================================================

			if (std::is_floating_point<Type>())
			{
				testWithValue(Type(0.5));
				testWithValue(Type(Limit + 0.5));
				testWithValue(Type(Limit / 3.f));

#if SNEX_WRAP_ALL_NEGATIVE_INDEXES
				testWithValue(Type(-12.215 * Limit));
#endif

			}
			else
			{
				testWithValue(Type(80));
				testWithValue(Type(Limit));
				testWithValue(Type(Limit - 1));
				testWithValue(Type(-1));
				testWithValue(Type(0));
				testWithValue(Type(1));
				testWithValue(Type(Limit + 1));
				testWithValue(Type(-Limit + 1));
			}
		}
	}

	template <typename Container> void initialiseSpan(String& asCode, Container& data)
	{

		auto elementType = Types::Helpers::getTypeFromTypeId<typename Container::DataType>();

		asCode << "span<" << Types::Helpers::getTypeName(elementType) << ", " << ArraySize << "> data = { ";

		for (int i = 0; i < ArraySize; i++)
		{
			asCode << Types::Helpers::getCppValueString(var(i), elementType) << ", ";
			data[i] = (typename Container::DataType)i;
		}

		asCode = asCode.upToLastOccurrenceOf(", ", false, false);
		asCode << " };";
	}

	static constexpr bool isInterpolator()
	{
		return !IndexType::canReturnReference();
	}

	static constexpr bool hasDynamicBounds()
	{
		return IndexType::LogicType::hasDynamicBounds();
	}

	void testFloatAlphaAndIndex()
	{
        
		if constexpr (std::is_floating_point<Type>() && !isInterpolator())
		{
			if constexpr (hasDynamicBounds())
			{
                // Test Code ===================================================

				cppgen::Base c(cppgen::Base::OutputType::AddTabs);

				c << indexName + " i;";
				c << "T testAlpha(T input, int limit)";

				{
					cppgen::StatementBlock sb(c);
					c.addWithSemicolon("i = input;");
					c.addWithSemicolon("return i.getAlpha(limit);");
				}

				c << "int testIndex(T input, int delta, int limit)";
				{
					cppgen::StatementBlock sb(c);
					c.addWithSemicolon("i = input;");
					c.addWithSemicolon("return i.getIndex(limit, delta);");
				}

				//test.logMessage("Testing " + indexName + "::getAlpha");

				c.replaceWildcard("T", Types::Helpers::getTypeNameFromTypeId<Type>());
				auto obj = compile(c.toString());

				// Test Routine ==============================================

				auto testWithValue = [&](Type testValue, int deltaValue, int limit)
				{
					
					IndexType i;
					i = testValue;
					auto expectedAlpha = i.getAlpha(limit);
					auto actualAlpha = obj["testAlpha"].template call<Type>(testValue, limit);
					String am = indexName;
					am << "::getAlpha()";
					am << " with value " << String(testValue);
					test.expectWithinAbsoluteError(actualAlpha, expectedAlpha, Type(0.00001), am);

					auto expectedIndex = i.getIndex(limit, deltaValue);
					auto actualIndex = obj["testIndex"].template call<int>(testValue, deltaValue, limit);

					String im = indexName;
					im << "::getIndex()";
					im << " with value " << String(testValue) << " and delta " << String(deltaValue);
					test.expectEquals(actualIndex, expectedIndex, im);
				};

				// Test List =======================================================

				testWithValue(0.51, 0, 48);
				testWithValue(12.3, 0, 64);
				testWithValue(-0.52, -1, 91);
				testWithValue(Limit - 0.44, 2, 10);
				testWithValue(Limit + 25.2, 1, 16);
				testWithValue(Limit / 0.325 - 1, 9, 1);
				testWithValue(Limit * 9.029, 4, 2);
				testWithValue(Limit * -0.42, Limit + 2, 32);
				testWithValue(324.42, -Limit + 2, 57);
			}
			else
			{
				// Test Code ===================================================

				cppgen::Base c(cppgen::Base::OutputType::AddTabs);

				c << indexName + " i;";
				c << "T testAlpha(T input)";

				{
					cppgen::StatementBlock sb(c);
					c.addWithSemicolon("i = input;");
					c.addWithSemicolon("return i.getAlpha(0);");
				}

				c << "int testIndex(T input, int delta)";
				{
					cppgen::StatementBlock sb(c);
					c.addWithSemicolon("i = input;");
					c.addWithSemicolon("return i.getIndex(0, delta);");
				}

				//test.logMessage("Testing " + indexName + "::getAlpha");

				c.replaceWildcard("T", Types::Helpers::getTypeNameFromTypeId<Type>());
				auto obj = compile(c.toString());

				// Test Routine ==============================================

				auto testWithValue = [&](Type testValue, int deltaValue)
				{
					
					IndexType i;
					i = testValue;
					auto expectedAlpha = i.getAlpha(0);
					auto actualAlpha = obj["testAlpha"].template call<Type>(testValue);
					String am = indexName;
					am << "::getAlpha()";
					am << " with value " << String(testValue);
					test.expectWithinAbsoluteError(actualAlpha, expectedAlpha, Type(0.00001), am);

					auto expectedIndex = i.getIndex(0, deltaValue);
					auto actualIndex = obj["testIndex"].template call<int>(testValue, deltaValue);

					String im = indexName;
					im << "::getIndex()";
					im << " with value " << String(testValue) << " and delta " << String(deltaValue);
					test.expectEquals(actualIndex, expectedIndex, im);
				};

				// Test List =======================================================

				testWithValue(Type(0.51), Type(0));
				testWithValue(Type(12.3), Type(0));
				testWithValue(Type(-0.52), Type(-1));
				testWithValue(Type(Limit - 0.44), Type(2));
				testWithValue(Type(Limit + 25.2), Type(1));
				testWithValue(Type(Limit / 0.325 - 1), Type(9));
				testWithValue(Type(Limit * 9.029), Type(4));
				testWithValue(Type(Limit * 0.42), Type(Limit + 2));
				testWithValue(Type(324.42), Type(-Limit + 2));
			}
		}
	}

	void testIncrementors(FunctionClass::SpecialSymbols incType)
	{
		if constexpr (std::is_integral<Type>() && !IndexType::LogicType::hasDynamicBounds())
		{
			// Test Code ===================================================

			cppgen::Base c(cppgen::Base::OutputType::AddTabs);

			c << indexName + " i;";
			c << "int test(int input)";

			String op;

			{
				cppgen::StatementBlock sb(c);
				c.addWithSemicolon("i = input");

				switch (incType)
				{
				case FunctionClass::IncOverload:	 op = "++i;"; break;
				case FunctionClass::PostIncOverload: op = "i++;"; break;
				case FunctionClass::DecOverload:	 op = "--i;"; break;
				case FunctionClass::PostDecOverload: op = "i--;"; break;
                default:                             op = "";     break;
				}

				c.addWithSemicolon("return (int)" + op);
			}

			//test.logMessage("Testing " + indexName + "::" + FunctionClass::getSpecialSymbol({}, incType).toString());

			auto obj = compile(c.toString());

			// Test Routine ==============================================

			auto testWithValue = [&](int testValue)
			{
				
				IndexType i;
				i = testValue;
				int expected;

				switch (incType)
				{
				case FunctionClass::IncOverload:	 expected = (int)++i; break;
				case FunctionClass::PostIncOverload: expected = (int)i++; break;
				case FunctionClass::DecOverload:	 expected = (int)--i; break;
				case FunctionClass::PostDecOverload: expected = (int)i--; break;
                default:                             expected = 0; break;
				}

				auto actual = obj["test"].template call<int>(testValue);
				String message = indexName;
				message << ": " << op;
				message << " with value " << String(testValue);
				test.expectEquals(actual, expected, message);
			};

			// Test List =======================================================

			testWithValue(0);
			testWithValue(-1);
			testWithValue(Limit - 1);
			testWithValue(Limit + 1);
			testWithValue(Limit);
			testWithValue(Limit * 2);
			testWithValue(-Limit);
			testWithValue(Limit / 3);
		}
	}

	void testAssignAndCast()
	{
		if constexpr (Limit != 0)
		{
			test.logMessage("Testing assignment and type cast ");

			// Test Code ===================================================

			cppgen::Base c(cppgen::Base::OutputType::AddTabs);

			c << indexName + " i;";
			c << "T test(T input)";

			{
				cppgen::StatementBlock sb(c);
				c.addWithSemicolon("i = input");
				c.addWithSemicolon("return (T)i");
			}

			c.replaceWildcard("T", Types::Helpers::getTypeNameFromTypeId<Type>());
			auto obj = compile(c.toString());

			// Test Routine ==============================================

			auto testWithValue = [&](Type testValue)
			{
				IndexType i;
				i = testValue;
				auto expected = (Type)i;
				auto actual = obj["test"].template call<Type>(testValue);
				String message = indexName;
				message << " with value " << String(testValue);
				test.expectWithinAbsoluteError(actual, expected, Type(0.00001), message);
			};

			// Test List =======================================================

			if constexpr (std::is_floating_point<Type>())
			{
				testWithValue(Type(Limit - 0.4));
				testWithValue(Type(Limit + 0.1));
				testWithValue(Type(Limit + 2.4));
				testWithValue(Type(-0.2));
				testWithValue(Type(-80.2));
			}
			else
			{
				testWithValue(Type(0));
				testWithValue(Type(Limit - 1));
				testWithValue(Type(Limit));
				testWithValue(Type(Limit + 1));
				testWithValue(Type(-1));
				testWithValue(Type(-Limit - 2));
				testWithValue(Type(Limit * 32 + 9));
			}
		}
	}

	JitObject compile(const String& code)
	{


		for (auto& o : optimisations)
			s.addOptimization(o);

		Compiler compiler(s);
		SnexObjectDatabase::registerObjects(compiler, 2);
		auto obj = compiler.compileJitObject(code);

		test.expect(compiler.getCompileResult().wasOk(), compiler.getCompileResult().getErrorMessage());
		return obj;
	}

	GlobalScope s;
	UnitTest& test;
	StringArray optimisations;
};


}
}

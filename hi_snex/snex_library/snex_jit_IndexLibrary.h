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

namespace IndexIds
{
#define DECLARE_ID(x) static const Identifier x(#x);
	DECLARE_ID(CheckBoundsOnAssign);
	DECLARE_ID(HasDynamicBounds);
	DECLARE_ID(wrapped);
	DECLARE_ID(clamped);
	DECLARE_ID(normalised);
	DECLARE_ID(unscaled);
	DECLARE_ID(unsafe);
	DECLARE_ID(previous);
	DECLARE_ID(lerp);
	DECLARE_ID(hermite);
	DECLARE_ID(looped);
#undef DECLARE_ID
}

struct IndexBuilder : public TemplateClassBuilder
{
	enum class Type
	{
		Integer,
		Float,
		Interpolated,
		numIndexTypes
	};

	struct MetaDataExtractor
	{
		

		enum class WrapLogicType
		{
			Unsafe,
			Clamped,
			Wrapped,
			Previous,
			Looped,
			numWrapLogicTypes
		};

		MetaDataExtractor(StructType* st);

		/** Returns true if the index has an integer type or false if it's a float index. */
		bool isIntegerType() const
		{
			return mainStruct == indexStruct;
		}

		bool isInterpolationType() const
		{
			auto id = mainStruct->id.getIdentifier();
			
			if (id == IndexIds::lerp || id == IndexIds::hermite)
			{
				jassert(floatIndexStruct != nullptr);
				return true;
			}
			
			return false;
		}

		bool hasBoundCheck() const
		{
			return indexStruct->id.getIdentifier() != IndexIds::unsafe;
		}

		bool isNormalisedFloat() const;

		bool shouldRedirect(InlineData* b) const;

		/** Returns the Type ID used for the index (either int or a float type). */
		Types::ID getIndexType() const
		{
			if (isIntegerType())
				return Types::ID::Integer;
			else if (isInterpolationType())
				return getFloatTypeParameter(0).type.getType();
			else
				return getMainParameter(0).type.getType();
		}

		int getStaticBounds() const
		{
			return getIndexTypeParameter(0).constant;
		}

		TypeInfo getInterpolatedIndexAsType() const;

		bool hasDynamicBounds() const
		{
			return getStaticBounds() == 0 && getWrapType() != WrapLogicType::Unsafe;
		}

		bool checkBoundsOnAssign() const;

		String getWithCast(const String& v, Types::ID t = Types::ID::Void) const;

		String getTypedLiteral(int v, Types::ID t = Types::ID::Void) const
		{
			return getWithCast(String(v), t);
		}

		String getScaledExpression(const String& v, bool from0To1, Types::ID t=Types::ID::Void) const;

		String getLimitExpression(const String& dynamicLimit, Types::ID t = Types::ID::Void) const
		{
			return  hasDynamicBounds() ? getWithCast(dynamicLimit, t) : String(getTypedLiteral(getStaticBounds(), t));
		}

		bool isLoopType() const;

		String getWithLimit(const String& v, const String& l, Types::ID dataType=Types::ID::Void) const;

		WrapLogicType getWrapType() const;

		TypeInfo getContainerElementType(InlineData* b) const;

	private:

		TemplateParameter getMainParameter(int index) const
		{
			return mainStruct->getTemplateInstanceParameters()[index];
		}

		TemplateParameter getFloatTypeParameter(int index) const
		{
			jassert(isInterpolationType() && floatIndexStruct != nullptr);
			return floatIndexStruct->getTemplateInstanceParameters()[index];
		}

		TemplateParameter getIndexTypeParameter(int index) const
		{
			return indexStruct->getTemplateInstanceParameters()[index];
		}

		StructType* mainStruct;			
		StructType* indexStruct;
		StructType* floatIndexStruct = nullptr; // Only non-null when interpolating
	};

	IndexBuilder(Compiler& c, const Identifier& indexId, Type indexType);

	static FunctionData assignFunction(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData af;
		af.id = st->id.getChildId("assignInternal");
		af.returnType = Types::ID::Void;
		af.addArgs("v", mt.getIndexType());

		if (mt.isInterpolationType())
		{
			af.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
			{
				cppgen::Base cb(cppgen::Base::OutputType::NoProcessing);

				cb << "this->idx = v;";

				return SyntaxTreeInlineParser(b, { "v" }, cb).flush();
			});
		}
		else
		{
			af.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
			{
				cppgen::Base cb(cppgen::Base::OutputType::NoProcessing);
				String e;

				if (mt.checkBoundsOnAssign())
				{
					auto l = mt.getLimitExpression({});

					if (mt.isNormalisedFloat())
					{
						String l1, l2, l3;

						l1 << "auto scaled = " << mt.getScaledExpression("v", true) << ";";
						l2 << "auto wrapped = " << mt.getWithLimit("scaled", l) << ";";
						l3 << "this->value = " << mt.getScaledExpression("wrapped", false) << ";";

						cb << l1;
						cb << l2;
						cb << l3;
					}
					else
					{
						e << "this->value = " << mt.getWithLimit("v", l) << ";";
						cb << e;
					}
				}
				else
				{
					e << "this->value = v;";
					cb << e;
				}

				return SyntaxTreeInlineParser(b, { "v" }, cb).flush();
			});
		}



		return af;
	}

	static FunctionData getFrom(StructType* st);

	static Result getElementType(InlineData* b);

	template <FunctionClass::SpecialSymbols IncType> static FunctionData incOp(StructType* st)
	{
		MetaDataExtractor mt(st);

		// can't inc non float types;
		jassert(mt.isIntegerType());

		FunctionData f;
		f.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, IncType));
		//f.returnType = { Types::ID::Integer };

		bool isInc = IncType == FunctionClass::SpecialSymbols::PostIncOverload ||
			IncType == FunctionClass::SpecialSymbols::IncOverload;

		bool isPre = IncType == FunctionClass::SpecialSymbols::IncOverload ||
			IncType == FunctionClass::SpecialSymbols::DecOverload;

		String expr = isInc ? "this->value + 1" : "this->value - 1";

		if (isPre)
		{
			// PreInc will return itself as object after incrementing.
			// We can't define it directly because of cyclic references...
			f.returnType = TypeInfo::makeNonRefCountedReferenceType(st);

			/* Original Function:

				if constexpr (checkBoundsOnAssign())
					value = LogicType::getWithDynamicLimit(++value, LogicType::getUpperLimit());
				else
					++value;

				return *this;
				*/
			f.inliner = Inliner::createHighLevelInliner({}, [mt, expr](InlineData* b)
			{
				cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

				String l1, l2;

				l1 << "auto newValue = " << expr;

				if (mt.checkBoundsOnAssign())
					l2 << "this->value = " << mt.getWithLimit("newValue", mt.getLimitExpression({}));
				else
					l2 << "this->value = newValue";

				cb << l1 << l2;

				cb << "return *this";

				return SyntaxTreeInlineParser(b, {}, cb).flush();
			});
		}
		else
		{
			// Postinc will always return a (checked) integer
			f.returnType = { Types::ID::Integer };

			/** Original function: 
			
			int operator++(int)
			{
				if constexpr (checkBoundsOnAssign())
				{
					auto v = value;
					value = LogicType::getWithDynamicLimit(++value, LogicType::getUpperLimit());
					return v;
				}
				else
				{
					auto v = LogicType::getWithDynamicLimit(value, LogicType::getUpperLimit());
					++value;
					return v;
				}
			}/
			*/

			f.inliner = Inliner::createHighLevelInliner({}, [mt, expr](InlineData* b)
			{
				// We need to check the bounds here in every case
				if (mt.hasDynamicBounds())
					return Result::fail("can't post increment index with dynamic bounds");

				cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

				auto limit = mt.getLimitExpression({});

				String l1, l2, l3;

				if (mt.checkBoundsOnAssign())
				{
					l1 << "auto v = this->value";
					l2 << "this->value = " << mt.getWithLimit(expr, limit);
				}
				else
				{
					l1 << "auto v = " << mt.getWithLimit("this->value", limit);
					l2 << "this->value = " << expr;
				}

				l3 << "return v";

				cb << l1 << l2 << l3;


				return SyntaxTreeInlineParser(b, {}, cb).flush();
			});

		}

		

		return f;
	}


	static FunctionData nativeTypeCast(StructType* st);

	static FunctionData setLoopRange(StructType* st);

	static FunctionData getIndexFunction(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData gi;
		gi.id = st->id.getChildId("getIndex");
		gi.returnType = { Types::ID::Integer };
		gi.addArgs("limit", Types::ID::Integer);
		gi.addArgs("delta", Types::ID::Integer);

		gi.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
			{
				cppgen::Base cb(cppgen::Base::OutputType::AddTabs);

				//auto scaled = (int)from0To1(v, limit) + delta;
				//return LogicType::getWithDynamicLimit(scaled, limit);

				String l1, l2;

				auto limit = mt.getLimitExpression("limit", Types::ID::Integer);

				if (mt.isNormalisedFloat())
					l1 << "auto scaled = (int)(this->value * " << mt.getWithCast(limit) << ") + delta;";
				else
					l1 << "auto scaled = (int)this->value + delta;";


				l2 << "return " << mt.getWithLimit("scaled", limit, Types::ID::Integer) << ";";

				cb << l1;
				cb << l2;

				return SyntaxTreeInlineParser(b, { "limit", "delta" }, cb).flush();
			});

		return gi;
	}

	static FunctionData getInterpolated(StructType* st);

	static FunctionData getAlphaFunction(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData gi;
		gi.id = st->id.getChildId("getAlpha");
		gi.returnType = { mt.getIndexType() };
		gi.addArgs("limit", Types::ID::Integer);

		gi.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::AddTabs);

			//auto f = from0To1(v, limit);
			//return f - (FloatType)((int)f);

			String l1, l2;

			if (mt.isNormalisedFloat())
				l1 << "auto f = this->value * (" << mt.getLimitExpression("limit") << ");";
			else
				l1 << "auto f = this->value;";


			l2 << "return f - " << mt.getWithCast(mt.getWithCast("f", Types::ID::Integer)) << ";";

			cb << l1;
			cb << l2;

			return SyntaxTreeInlineParser(b, { "limit" }, cb).flush();
		});

		return gi;
	}

	static FunctionData constructorFunction(StructType* st);

	static FunctionData assignOp(StructType* st);

	static void initialise(const TemplateObject::ConstructData&, StructType* st);
};

struct IndexLibrary : public LibraryBuilderBase
{
	IndexLibrary(Compiler& c, int numChannels) :
		LibraryBuilderBase(c, numChannels)
	{};

	Identifier getFactoryId() const override { RETURN_STATIC_IDENTIFIER("index"); }

	Result registerTypes() override
	{
		IndexBuilder lb(c, "looped", IndexBuilder::Type::Integer);			lb.flush();
		IndexBuilder wb(c, "wrapped", IndexBuilder::Type::Integer);			wb.flush();
		IndexBuilder cb(c, "clamped", IndexBuilder::Type::Integer);			cb.flush();
		IndexBuilder ub(c, "unsafe", IndexBuilder::Type::Integer);			ub.flush();
		IndexBuilder fnb(c, "normalised", IndexBuilder::Type::Float);		fnb.flush();
		IndexBuilder fub(c, "unscaled", IndexBuilder::Type::Float);			fub.flush();

		IndexBuilder lub(c, "lerp", IndexBuilder::Type::Interpolated);		lub.flush();
		IndexBuilder hub(c, "hermite", IndexBuilder::Type::Interpolated);	hub.flush();

		return Result::ok();
	}
};

}
}

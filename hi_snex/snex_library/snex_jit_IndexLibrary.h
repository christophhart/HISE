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
#undef DECLARE_ID;
}

struct IndexBuilder : public TemplateClassBuilder
{
	struct MetaDataExtractor
	{
		enum class WrapLogicType
		{
			Unsafe,
			Clamped,
			Wrapped,
			numWrapLogicTypes
		};

		MetaDataExtractor(StructType* st):
			mainStruct(st)
		{
			if (getMainParameter(0).constantDefined)
				indexStruct = mainStruct;
			else
				indexStruct = getMainParameter(1).type.getTypedComplexType<StructType>();
		}

		/** Returns true if the index has an integer type or false if it's a float index. */
		bool isIntegerType() const
		{
			return mainStruct == indexStruct;
		}

		bool hasBoundCheck() const
		{
			return indexStruct->id.getIdentifier() != IndexIds::unsafe;
		}

		bool isNormalisedFloat() const
		{
			return !isIntegerType() && mainStruct->id.getIdentifier() == IndexIds::normalised;
		}

		/** Returns the Type ID used for the index (either int or a float type). */
		Types::ID getIndexType() const
		{
			if (isIntegerType())
				return Types::ID::Integer;
			else
				return getMainParameter(0).type.getType();
		}

		int getStaticBounds() const
		{
			return getIndexTypeParameter(0).constant;
		}

		bool hasDynamicBounds() const
		{
			return getStaticBounds() == 0 && getWrapType() != WrapLogicType::Unsafe;
		}

		bool checkBoundsOnAssign() const
		{
			if (hasDynamicBounds())
				return false;

			return getIndexTypeParameter(1).constant > 0;
		}

		String getWithCast(const String& v, Types::ID t = Types::ID::Void) const
		{
			if (t == Types::ID::Dynamic)
				return v;

			String s;
			s << "(" << Types::Helpers::getTypeName(t != Types::ID::Void ? t : getIndexType()) << ")" << v;
			return s;
		}

		String getTypedLiteral(int v, Types::ID t = Types::ID::Void) const
		{
			return getWithCast(String(v), t);
		}

		String getScaledExpression(const String& v, bool from0To1, Types::ID t=Types::ID::Void) const
		{
			if (!isNormalisedFloat())
				return v;

			String s;

			s << v;
			s << (from0To1 ? JitTokens::times : JitTokens::divide);
			s << getLimitExpression({}, t);

			return s;	
		}

		String getLimitExpression(const String& dynamicLimit, Types::ID t = Types::ID::Void) const
		{
			return  hasDynamicBounds() ? getWithCast(dynamicLimit, t) : String(getTypedLiteral(getStaticBounds(), t));
		}

		String getWithLimit(const String& v, const String& l, Types::ID dataType=Types::ID::Void) const
		{
			if (dataType == Types::ID::Void)
				dataType = getIndexType();

			String expression;

			switch (getWrapType())
			{
			case WrapLogicType::Unsafe:
				expression << v;
				break;
			case WrapLogicType::Clamped:
			{
				expression << "Math.range(";
				expression << v << ", " << getTypedLiteral(0, dataType);
				expression << ", " << l << " - " << getTypedLiteral(1) << ")";
			}
			break;
			case WrapLogicType::Wrapped:
			{
				if (dataType == Types::ID::Integer)
				{
					String formula = "(!v >= 0) ? !v % !limit : (!limit - Math.abs(!v) % !limit) % !limit";
					expression << formula.replace("!v", v).replace("!limit", l);
				}
				else
					expression << "Math.fmod(" << v << ", " << l << ")";

				break;
			}
			}

			return expression;
		}

#if 0
		String getBoundsExpression(const String& v, const String& dynamicLimit) const
		{
			String expression;
			String limitExpression = getLimitExpression(dynamicLimit);

			if (isNormalisedFloat())
				limitExpression = getTypedLiteral(1);

			switch (getWrapType())
			{
			case WrapLogicType::Unsafe: 
				expression << v; 
				break;
			case WrapLogicType::Clamped: 
			{
				expression << "Math.range(";
				expression << v;
				expression << ", " << getTypedLiteral(0) << ", ";

				// subtract 1 for integers
				if (isIntegerType())
					expression << limitExpression << " - " << getTypedLiteral(1) << ")";
				else
					expression << limitExpression << ")";
			}

				
				break;
			case WrapLogicType::Wrapped:
			{
				if (isIntegerType())
				{
					String formula = "(!v >= 0) ? !v % !limit : (!limit - Math.abs(!v) % !limit) % !limit";

					expression << formula.replace("!v", v).replace("!limit", limitExpression);
				}
					
				else
					expression << "Math.fmod(v, " << limitExpression << ")";

				break;
			}
			}

			return expression;
		}
#endif

		WrapLogicType getWrapType() const
		{
			auto i = indexStruct->id.getIdentifier();

			if (i == IndexIds::unsafe)
				return WrapLogicType::Unsafe;

			if (i == IndexIds::wrapped)
				return WrapLogicType::Wrapped;

			if (i == IndexIds::clamped)
				return WrapLogicType::Clamped;
		}

	private:

		TemplateParameter getMainParameter(int index) const
		{
			return mainStruct->getTemplateInstanceParameters()[index];
		}

		TemplateParameter getIndexTypeParameter(int index) const
		{
			return indexStruct->getTemplateInstanceParameters()[index];
		}

		StructType* mainStruct;
		StructType* indexStruct;
	};

	IndexBuilder(Compiler& c, const Identifier& indexId, bool isIntegerIndex) :
		TemplateClassBuilder(c, NamespacedIdentifier("index").getChildId(indexId))
	{
		if (isIntegerIndex)
		{
			addIntTemplateParameter("UpperLimit");
			addIntTemplateParameterWithDefault("CheckOnAssign", 0);
		}
		else
		{
			addTypeTemplateParameter("FloatType");
			addTypeTemplateParameter("IndexType");
		}

		setInitialiseStructFunction(IndexBuilder::initialise);

		addFunction(IndexBuilder::assignFunction);
		addFunction(IndexBuilder::constructorFunction);
		addFunction(IndexBuilder::nativeTypeCast);
		addFunction(IndexBuilder::assignOp);
		addFunction(IndexBuilder::getFrom);

		if (isIntegerIndex)
		{
			addFunction(incOp<FunctionClass::SpecialSymbols::IncOverload>);
			addFunction(incOp<FunctionClass::SpecialSymbols::DecOverload>);
			addFunction(incOp<FunctionClass::SpecialSymbols::PostIncOverload>);
			addFunction(incOp<FunctionClass::SpecialSymbols::PostDecOverload>);
		}
		else
		{
			addFunction(IndexBuilder::getIndexFunction);
			addFunction(IndexBuilder::getAlphaFunction);
		}
	}

	static FunctionData assignFunction(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData af;
		af.id = st->id.getChildId("assignInternal");
		af.returnType = TypeInfo(Types::ID::Void);
		af.addArgs("v", mt.getIndexType());

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
					auto v = 

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

			return SyntaxTreeInlineParser(b, {"v"}, cb).flush();
		});

		return af;
	}

	static FunctionData getFrom(StructType* st);

	template <FunctionClass::SpecialSymbols IncType> static FunctionData incOp(StructType* st)
	{
		MetaDataExtractor mt(st);

		// can't inc non float types;
		jassert(mt.isIntegerType());

		FunctionData f;
		f.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, IncType));
		//f.returnType = { Types::ID::Integer };

		f.returnType = { Types::ID::Dynamic };

		f.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			if (mt.hasDynamicBounds())
				return Result::fail("can't increment index with dynamic bounds");

			cppgen::Base cb(cppgen::Base::OutputType::NoProcessing);

			bool isInc = IncType == FunctionClass::SpecialSymbols::PostIncOverload ||
					     IncType == FunctionClass::SpecialSymbols::IncOverload;

			bool isPre = IncType == FunctionClass::SpecialSymbols::IncOverload ||
						 IncType == FunctionClass::SpecialSymbols::DecOverload;

			auto expr = isInc ? JitTokens::plusplus : JitTokens::minusminus;

			if (!isPre)
				cb << "auto retValue = this->value;";

			cb << "auto& v = this->value;";
			cb << expr << "v;";

			auto limit = mt.getLimitExpression({});

			if (mt.checkBoundsOnAssign())
			{
				String s;
				s << "v = " << mt.getWithLimit("v", limit) << ";";
				cb << s;

				if (isPre) // already bound checked...
					cb << "return v;";
				else
				{
					// we need to bound check the return type again
					String r;
					r << "return " << mt.getWithLimit("retValue", limit) << ";";
					cb << r;
				}
			}
			else
			{
				// We don't check the bounds on assignment (so just return the value)
				String e;
				e << "return " << (isPre ? "v" : "retValue") << ";";
				cb << e;
			}

			DBG(cb.toString());

			return SyntaxTreeInlineParser(b, {}, cb).flush();
		});

		f.inliner->returnTypeFunction = [st](InlineData* b)
		{
			auto rt = dynamic_cast<ReturnTypeInlineData*>(b);

			if (IncType == FunctionClass::IncOverload ||
				IncType == FunctionClass::DecOverload)
			{
				rt->f.returnType = TypeInfo(st, false, true);
			}
			else
			{
				rt->f.returnType = { Types::ID::Integer };
			}

			return Result::ok();
		};

		return f;
	}


	static FunctionData nativeTypeCast(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData d;
		d.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::NativeTypeCast));
		d.returnType = TypeInfo(mt.getIndexType());

		d.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

			if (mt.checkBoundsOnAssign())
				cb << "return this->value";
			else
			{
				if (mt.hasDynamicBounds())
					return Result::fail("Can't cast an index with dynamic bounds");

				if (mt.isNormalisedFloat())
				{
					String l1, l2, l3;

					cb << "auto scaled = " + mt.getScaledExpression("this->value", true);
					cb << "auto limit = " + mt.getLimitExpression({});
					cb << "return " + mt.getWithLimit("scaled", "limit");
				}
				else
				{
					String e;
					cb << "auto limit = " + mt.getLimitExpression({});
					cb << "return " + mt.getWithLimit("this->value", "limit");
				}
			}

			return SyntaxTreeInlineParser(b, {}, cb).flush();
		});

		return d;
	}

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

			return SyntaxTreeInlineParser(b, {"limit", "delta"}, cb).flush();
		});

		return gi;
	}

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
				l1 << "auto f = this->value * " << mt.getLimitExpression("limit") << ";";
			else
				l1 << "auto f = this->value;";

			
			l2 << "return f - " << mt.getWithCast(mt.getWithCast("f", Types::ID::Integer)) << ";";

			cb << l1;
			cb << l2;

			return SyntaxTreeInlineParser(b, { "limit" }, cb).flush();
		});

		return gi;
	}

	static FunctionData constructorFunction(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData c;
		c.id = st->id.getChildId(st->id.getIdentifier());
		c.returnType = Types::ID::Void;
		c.addArgs("initValue", TypeInfo(mt.getIndexType()));
		c.setDefaultParameter("initValue", VariableStorage(mt.getIndexType(), var(0)));

		c.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::AddTabs);

			cb << "this->assignInternal(initValue);";

			return SyntaxTreeInlineParser(b, { "initValue" }, cb).flush();
		});

		return c;
	}

	static FunctionData assignOp(StructType* st)
	{
		MetaDataExtractor mt(st);

		FunctionData af;
		af.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::AssignOverload));
		af.returnType = TypeInfo(Types::ID::Void);
		af.addArgs("v", mt.getIndexType());

		af.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::AddTabs);
			cb << "this->assignInternal(v); ";
			return SyntaxTreeInlineParser(b, { "v" }, cb).flush();
		});

		return af;
	}

	

	static void initialise(const TemplateObject::ConstructData&, StructType* st)
	{
		MetaDataExtractor metadata(st);

		st->addMember("value", TypeInfo(metadata.getIndexType()));
		st->setDefaultValue("value", InitialiserList::makeSingleList(VariableStorage(metadata.getIndexType(), var(0))));
		st->setVisibility("value", NamespaceHandler::Visibility::Private);
	}
};

struct IndexLibrary : public LibraryBuilderBase
{
	IndexLibrary(Compiler& c, int numChannels) :
		LibraryBuilderBase(c, numChannels)
	{};

	Identifier getFactoryId() const override { RETURN_STATIC_IDENTIFIER("index"); }

	Result registerTypes() override
	{
		IndexBuilder wb(c, "wrapped", true);
		wb.flush();

		IndexBuilder cb(c, "clamped", true);
		cb.flush();

		IndexBuilder ub(c, "unsafe", true);
		ub.flush();

		IndexBuilder fnb(c, "normalised", false);
		fnb.flush();

		IndexBuilder fub(c, "unscaled", false);
		fub.flush();

		return Result::ok();
	}
};

}
}

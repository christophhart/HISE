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


IndexBuilder::MetaDataExtractor::MetaDataExtractor(StructType* st) :
	mainStruct(st)
{
	if (getMainParameter(0).constantDefined)
		indexStruct = mainStruct;
	else
	{
		if (st->getTemplateInstanceParameters().size() == 2)
		{
			indexStruct = getMainParameter(1).type.getTypedComplexType<StructType>();
		}
		else
		{
			floatIndexStruct = getMainParameter(0).type.getTypedComplexType<StructType>();
			indexStruct = floatIndexStruct->getTemplateInstanceParameters()[1].type.getTypedComplexType<StructType>();
		}
	}	
}

bool IndexBuilder::MetaDataExtractor::isNormalisedFloat() const
{
	if (isIntegerType())
		return false;

	if (isInterpolationType())
		return floatIndexStruct->id.getIdentifier() == IndexIds::normalised;
	else
		return mainStruct->id.getIdentifier() == IndexIds::normalised;
}

bool IndexBuilder::MetaDataExtractor::shouldRedirect(InlineData* b) const
{
	if (!hasBoundCheck())
		return false;

	return b->toSyntaxTreeData()->args[0]->getTypeInfo().getTypedIfComplexType<DynType>() != nullptr;
}


snex::jit::TypeInfo IndexBuilder::MetaDataExtractor::getInterpolatedIndexAsType() const
{
	jassert(isInterpolationType());

	return TypeInfo(floatIndexStruct, false, false);
}

bool IndexBuilder::MetaDataExtractor::checkBoundsOnAssign() const
{
	if (hasDynamicBounds())
		return false;

	return getIndexTypeParameter(1).constant > 0;
}

String IndexBuilder::MetaDataExtractor::getWithCast(const String& v, Types::ID t /*= Types::ID::Void*/) const
{
	if (t == Types::ID::Dynamic)
		return v;

	String s;
	s << "(" << Types::Helpers::getTypeName(t != Types::ID::Void ? t : getIndexType()) << ")" << v;
	return s;
}

String IndexBuilder::MetaDataExtractor::getScaledExpression(const String& v, bool from0To1, Types::ID t/*=Types::ID::Void*/) const
{
	if (!isNormalisedFloat())
		return v;

	String s;

	s << v;
	s << (from0To1 ? JitTokens::times : JitTokens::divide);
	s << getLimitExpression({}, t);

	return s;
}

bool IndexBuilder::MetaDataExtractor::isLoopType() const
{
	if (mainStruct->id.getIdentifier() == IndexIds::looped)
		return true;

	if (floatIndexStruct != nullptr)
		return false;

	if (indexStruct != nullptr && indexStruct->id.getIdentifier() == IndexIds::looped)
		return true;
}

String IndexBuilder::MetaDataExtractor::getWithLimit(const String& v, const String& l, Types::ID dataType/*=Types::ID::Void*/) const
{
	if (dataType == Types::ID::Void)
		dataType = getIndexType();

	String expression;

	switch (getWrapType())
	{
	case WrapLogicType::Previous:
	case WrapLogicType::Unsafe:
		expression << v;
		break;
	case WrapLogicType::Clamped:
	{
		expression << "Math.range(";
		expression << v << ", " << getTypedLiteral(0, dataType);
		expression << ", " << l << " - " << getTypedLiteral(1, dataType) << ")";
		break;
	}
	case WrapLogicType::Wrapped:
	{
		expression << "Math.wrap(" << v << ", " << l << ")";

		break;
	}
	case WrapLogicType::Looped:
	{
		String formula = "!v < !s ? Math.max(!z, !v) : Math.wrap(!v - !s, this->length != 0 ? !length : !limit) + !s";
		expression << formula.replace("!v", v)
						     .replace("!s", getWithCast("this->start", dataType))
							 .replace("!limit", l)
							 .replace("!length", getWithCast("this->length", dataType))
							 .replace("!z", getTypedLiteral(0, dataType));
		break;
	}
	default: jassertfalse;
	}

	return expression;
}

snex::jit::IndexBuilder::MetaDataExtractor::WrapLogicType IndexBuilder::MetaDataExtractor::getWrapType() const
{
	auto i = indexStruct->id.getIdentifier();

	if (i == IndexIds::unsafe)
		return WrapLogicType::Unsafe;

	if (i == IndexIds::wrapped)
		return WrapLogicType::Wrapped;

	if (i == IndexIds::clamped)
		return WrapLogicType::Clamped;

	if (i == IndexIds::previous)
		return WrapLogicType::Previous;

	if (i == IndexIds::looped)
		return WrapLogicType::Looped;

	jassertfalse;
	return WrapLogicType::numWrapLogicTypes;
}

snex::jit::TypeInfo IndexBuilder::MetaDataExtractor::getContainerElementType(InlineData* b) const
{
	auto containerType = b->toSyntaxTreeData()->args[0]->getTypeInfo();

	if (auto at = containerType.getTypedIfComplexType<ArrayTypeBase>())
	{
		return at->getElementType();
	}

	jassertfalse;
	return {};
}

IndexBuilder::IndexBuilder(Compiler& c, const Identifier& indexId, Type indexType) :
	TemplateClassBuilder(c, NamespacedIdentifier("index").getChildId(indexId))
{
	switch (indexType)
	{
	case Type::Float:
		addTypeTemplateParameter("FloatType");
		addTypeTemplateParameter("IndexType");
		break;
	case Type::Integer:
		addIntTemplateParameter("UpperLimit");
		addIntTemplateParameterWithDefault("CheckOnAssign", 0);
		break;
	case Type::Interpolated:
		addTypeTemplateParameter("FloatIndexType");
		break;
	}

	setInitialiseStructFunction(IndexBuilder::initialise);

	addFunction(IndexBuilder::assignFunction);
	addFunction(IndexBuilder::constructorFunction);
	addFunction(IndexBuilder::nativeTypeCast);
	addFunction(IndexBuilder::assignOp);
	addFunction(IndexBuilder::getFrom);
	addFunction(IndexBuilder::setLoopRange);

	switch (indexType)
	{
	case Type::Float:
		addFunction(IndexBuilder::getIndexFunction);
		addFunction(IndexBuilder::getAlphaFunction);
		break;
	case Type::Integer:
		addFunction(incOp<FunctionClass::SpecialSymbols::IncOverload>);
		addFunction(incOp<FunctionClass::SpecialSymbols::DecOverload>);
		addFunction(incOp<FunctionClass::SpecialSymbols::PostIncOverload>);
		addFunction(incOp<FunctionClass::SpecialSymbols::PostDecOverload>);
		break;
	case Type::Interpolated:
		addFunction(IndexBuilder::getInterpolated);
		break;
	}
}

#if 0
snex::jit::FunctionData IndexBuilder::rawAccess(StructType* st)
{
	FunctionData rf;
	rf.id = st->id.getChildId("getRaw");
	rf.returnType = TypeInfo(Types::ID::Dynamic, false, true);

	rf.addArgs("c", TypeInfo(Types::ID::Dynamic, true, true));
	rf.addArgs("index", Types::ID::Integer);

	MetaDataExtractor mt(st);

	rf.inliner = Inliner::createAsmInliner({}, [mt](InlineData* b)
	{
		auto d = b->toAsmInlineData();

		if (auto at = d->args[0]->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
		{
			auto byteSize = at->getElementType().getRequiredByteSize();

			auto offset = 0;

			if (mt.getWrapType() == MetaDataExtractor::WrapLogicType::Previous)
				offset = -1 * byteSize;

			d->gen.emitSpanReference(d->target, d->args[0], d->args[1], byteSize, offset);

			return Result::ok();
		}

		return Result::fail("can't deduce array type");
	});

	rf.inliner->returnTypeFunction = getElementType;

	return rf;
}
#endif


snex::jit::FunctionData IndexBuilder::getFrom(StructType* st)
{
	MetaDataExtractor mt(st);

	FunctionData gf;

	gf.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::GetFrom));
	gf.addArgs("c", TypeInfo(Types::ID::Dynamic, true, true));
	gf.returnType = TypeInfo(Types::ID::Dynamic, false, !mt.isInterpolationType());

	if (mt.isInterpolationType())
	{
		if (st->id.getIdentifier() == IndexIds::lerp)
		{
			gf.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
			{
				BaseCompiler::ScopedUnsafeBoundChecker sb(b->toSyntaxTreeData()->object->currentCompiler);
				cppgen::Base cb(cppgen::Base::OutputType::AddTabs);

				cb << "auto i0 = this->idx.getIndex(c.size(), 0);";
				cb << "auto i1 = this->idx.getIndex(c.size(), 1);";
				cb << "auto alpha = this->idx.getAlpha(c.size());";

				auto containerType = mt.getContainerElementType(b);

				if (auto at = containerType.getTypedIfComplexType<ArrayTypeBase>())
				{
					String l0, l1, l2, l3;

					cb << (containerType.toString() + " d;");
					cb << "int j = 0;";
					cb << "auto& c0 = c[i0];";
					cb << "auto& c1 = c[i1];";

					cb << "for(auto& s: d)";
					{
						cppgen::StatementBlock sb(cb);

						l0 << "auto x0 = " << mt.getWithCast("c0[j]") << ";";
						l1 << "auto x1 = " << mt.getWithCast("c1[j]") << ";";

						cb << l0 << l1;
						cb << "++j;";
						cb << "s = this->getInterpolated(x0, x1, alpha);";
					}

					cb << "return d;";

					return SyntaxTreeInlineParser(b, { "c" }, cb).flush();
				}
				else
				{
					String l1, l2;

					l1 << "auto x0 = " << mt.getWithCast("c[i0]") << ";";
					l2 << "auto x1 = " << mt.getWithCast("c[i1]") << ";";

					cb << l1 << l2;

					cb << "return this->getInterpolated(x0, x1, alpha);";

					return SyntaxTreeInlineParser(b, { "c" }, cb).flush();
				}
			});
		}
		else
		{
			gf.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
			{
				BaseCompiler::ScopedUnsafeBoundChecker sb(b->toSyntaxTreeData()->object->currentCompiler);

				cppgen::Base cb(cppgen::Base::OutputType::AddTabs);

				cb << "auto i0 = this->idx.getIndex(c.size(), -1);";
				cb << "auto i1 = this->idx.getIndex(c.size(), 0);";
				cb << "auto i2 = this->idx.getIndex(c.size(), 1);";
				cb << "auto i3 = this->idx.getIndex(c.size(), 2);";
				cb << "auto alpha = this->idx.getAlpha(c.size());";

				String l1, l2, l3, l4;

				auto containerType = mt.getContainerElementType(b);

				if (auto at = containerType.getTypedIfComplexType<ArrayTypeBase>())
				{
					String l0, l1, l2, l3;

					cb << (containerType.toString() + " d;");
					cb << "int j = 0;";
					cb << "auto& c0 = c[i0];";
					cb << "auto& c1 = c[i1];";
					cb << "auto& c2 = c[i2];";
					cb << "auto& c3 = c[i3];";

					cb << "for(auto& s: d)";
					{
						cppgen::StatementBlock sb(cb);

						l1 << "auto x0 = " << mt.getWithCast("c0[j]") << ";";
						l2 << "auto x1 = " << mt.getWithCast("c1[j]") << ";";
						l3 << "auto x2 = " << mt.getWithCast("c2[j]") << ";";
						l4 << "auto x3 = " << mt.getWithCast("c3[j]") << ";";

						cb << l1 << l2 << l3 << l4;
						cb << "++j;";
						cb << "s = this->getInterpolated(x0, x1, x2, x3, alpha);";
					}

					cb << "return d;";
				}
				else
				{
					l1 << "auto x0 = " << mt.getWithCast("c[i0]") << ";";
					l2 << "auto x1 = " << mt.getWithCast("c[i1]") << ";";
					l3 << "auto x2 = " << mt.getWithCast("c[i2]") << ";";
					l4 << "auto x3 = " << mt.getWithCast("c[i3]") << ";";

					cb << l1 << l2 << l3 << l4;

					cb << "return this->getInterpolated(x0, x1, x2, x3, alpha);";
				}

				return SyntaxTreeInlineParser(b, { "c" }, cb).flush();
			});
		}

		
	}
	else
	{
		gf.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			BaseCompiler::ScopedUnsafeBoundChecker sb(b->toSyntaxTreeData()->object->currentCompiler);

			cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

			String l1, l2, l3, l4;

			l1 << "int limit = Math.max(1, " << mt.getLimitExpression("c.size()", Types::ID::Dynamic) << ")";

			if (mt.isIntegerType())
			{
				l2 << "int idx = " << mt.getWithLimit("this->value", "limit");
			}
			else
			{
				if (mt.isNormalisedFloat())
					l2 << "auto scaled = (int)(this->value * " << mt.getWithCast("limit") << ")";
				else
					l2 << "auto scaled = (int)this->value";

				l3 << "int idx = " << mt.getWithLimit("scaled", "limit", Types::ID::Integer);
			}

			l4 << "return c[idx];";// this->getRaw(c, idx); ";

			cb << l1 << l2 << l3 << l4;

			return SyntaxTreeInlineParser(b, { "c" }, cb).flush();
		});
	}

	gf.inliner->returnTypeFunction = getElementType;

	return gf;
}

juce::Result IndexBuilder::getElementType(InlineData* b)
{
	auto rt = dynamic_cast<ReturnTypeInlineData*>(b);

	if (rt->object != nullptr)
	{
		auto prevType = rt->f.returnType;

		if (auto a = rt->object->getSubExpr(1))
		{
			if (auto at = a->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
			{
				rt->f.returnType = at->getElementType().withModifiers(prevType.isConst(), prevType.isRef(), prevType.isStatic());
				return Result::ok();
			}
		}

		return Result::ok();

	}

	return Result::fail("Can't deduce array element type");
}

snex::jit::FunctionData IndexBuilder::nativeTypeCast(StructType* st)
{
	MetaDataExtractor mt(st);

	FunctionData d;
	d.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::NativeTypeCast));
	d.returnType = TypeInfo(mt.getIndexType());



	d.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
	{
		cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

		if (mt.isInterpolationType())
		{
			String l1;
			l1 << "return " << mt.getWithCast("this->idx");
			cb << l1;
		}
		else
		{
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
		}

		return SyntaxTreeInlineParser(b, {}, cb).flush();
	});

	return d;
}

snex::jit::FunctionData IndexBuilder::setLoopRange(StructType* st)
{
	FunctionData lr;
	lr.id = st->id.getChildId("setLoopRange");
	lr.returnType = Types::ID::Void;
	lr.addArgs("loopStart", Types::ID::Integer);
	lr.addArgs("loopEnd", Types::ID::Integer);

	lr.inliner = Inliner::createHighLevelInliner({}, [st](InlineData* b)
	{
		MetaDataExtractor mt(st);

		cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

		if (mt.isInterpolationType())
		{
			cb << "this->idx.setLoopRange(loopStart, loopEnd);";
		}
		else
		{
			if (mt.isLoopType())
			{
				cb << "this->start = loopStart;";
				cb << "this->length = loopEnd - loopStart;";
			}
		}

		return SyntaxTreeInlineParser(b, { "loopStart", "loopEnd" }, cb).flush();
	});

	return lr;
}

snex::jit::FunctionData IndexBuilder::getInterpolated(StructType* st)
{
	MetaDataExtractor mt(st);

	jassert(mt.isInterpolationType());

	FunctionData ip;
	ip.id = st->id.getChildId("getInterpolated");

	auto floatType = TypeInfo(mt.getIndexType());

	if (st->id.getIdentifier() == IndexIds::lerp)
	{
		ip.addArgs("x0", floatType);
		ip.addArgs("x1", floatType);
		ip.addArgs("alpha", floatType);
		ip.returnType = floatType;

		ip.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

			cb << "return x0 + (x1 - x0) * alpha";

			return SyntaxTreeInlineParser(b, {"x0", "x1", "alpha"}, cb).flush();
		});
	}
	else
	{
		ip.addArgs("x0", floatType);
		ip.addArgs("x1", floatType);
		ip.addArgs("x2", floatType);
		ip.addArgs("x3", floatType);
		ip.addArgs("alpha", floatType);
		ip.returnType = floatType;

		ip.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

			String l1, l2, l3, l4;

			l1 << "auto a = ((" << mt.getWithCast("3") << " * (x1 - x2)) - x0 + x3) * " << mt.getWithCast("0.5");
			l2 << "auto b = x2 + x2 + x0 - (" << mt.getWithCast("5") << " *x1 + x3) * " << mt.getWithCast("0.5");
			l3 << "auto c = (x2 - x0) * " << mt.getWithCast("0.5");
			l4 << "return ((a*alpha + b)*alpha + c)*alpha + x1";

			cb << l1 << l2 << l3 << l4;

			return SyntaxTreeInlineParser(b, { "x0", "x1", "x2", "x3", "alpha" }, cb).flush();
		});
	}

	return ip;
}

snex::jit::FunctionData IndexBuilder::constructorFunction(StructType* st)
{
	MetaDataExtractor mt(st);

	FunctionData c;
	c.id = st->id.getChildId(st->id.getIdentifier());
	c.returnType = Types::ID::Void;
	c.addArgs("initValue", TypeInfo(mt.getIndexType()));
	c.setDefaultParameter("initValue", VariableStorage(mt.getIndexType(), var(0)));

	c.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		auto numArgs = d->args.size();

		if (numArgs == 0)
		{
			d->target = new Operations::Noop(d->location);
			d->replaceIfSuccess();
			return Result::ok();
		}

		if (numArgs != 0)
		{
			cppgen::Base cb(cppgen::Base::OutputType::AddTabs);

			cb << "this->assignInternal(initValue);";

			return SyntaxTreeInlineParser(b, { "initValue" }, cb).flush();
		}


		jassertfalse;

		return Result::fail("TUT");
		
	});

	return c;
}

snex::jit::FunctionData IndexBuilder::assignOp(StructType* st)
{
	MetaDataExtractor mt(st);

	FunctionData af;
	af.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::AssignOverload));
	af.returnType = TypeInfo::makeNonRefCountedReferenceType(st);
	af.addArgs("v", mt.getIndexType());

	af.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
		{
			cppgen::Base cb(cppgen::Base::OutputType::AddTabs);
			cb << "this->assignInternal(v); ";
			cb << "return *this;";
			return SyntaxTreeInlineParser(b, { "v" }, cb).flush();
		});

	return af;
}

void IndexBuilder::initialise(const TemplateObject::ConstructData& td, StructType* st)
{
	MetaDataExtractor metadata(st);

	auto sizeValue = metadata.getStaticBounds();

	if (sizeValue < 0)
	{
		String e;

		e << "Illegal size value: " + String(sizeValue) << ".Use zero for dynamic bounds.";

		if (td.r != nullptr)
			*td.r = Result::fail(e);
		else
			jassertfalse;

		return;
	}

	if (metadata.isInterpolationType())
	{
		st->addMember("idx", TypeInfo(metadata.getInterpolatedIndexAsType()));
		st->setDefaultValue("idx", InitialiserList::makeSingleList(VariableStorage(metadata.getIndexType(), var(0))));
		st->setVisibility("idx", NamespaceHandler::Visibility::Private);
	}
	else
	{
		st->addMember("value", TypeInfo(metadata.getIndexType()));
		st->setDefaultValue("value", InitialiserList::makeSingleList(VariableStorage(metadata.getIndexType(), var(0))));
		st->setVisibility("value", NamespaceHandler::Visibility::Private);
	}

	if (metadata.isLoopType())
	{
		st->addMember("start", TypeInfo(Types::ID::Integer));
		st->addMember("length", TypeInfo(Types::ID::Integer));
		
		st->setDefaultValue("start", InitialiserList::makeSingleList(VariableStorage(0)));
		st->setDefaultValue("length", InitialiserList::makeSingleList(VariableStorage(0)));

		st->setVisibility("start", NamespaceHandler::Visibility::Public);
		st->setVisibility("length", NamespaceHandler::Visibility::Public);
	}

	
}


}
}

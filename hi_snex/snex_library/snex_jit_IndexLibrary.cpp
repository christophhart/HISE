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
using namespace asmjit;


snex::jit::FunctionData IndexBuilder::getFrom(StructType* st)
{
	MetaDataExtractor mt(st);

	FunctionData gf;

	gf.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::GetFrom));
	gf.addArgs("c", TypeInfo(Types::ID::Dynamic, true, true));
	gf.returnType = TypeInfo(Types::ID::Dynamic, false, true);

	gf.inliner = Inliner::createHighLevelInliner({}, [mt](InlineData* b)
	{
		BaseCompiler::ScopedUnsafeBoundChecker sb(b->toSyntaxTreeData()->object->currentCompiler);

		cppgen::Base cb(cppgen::Base::OutputType::StatementListWithoutSemicolon);

		auto castName = FunctionClass::getSpecialSymbol({}, FunctionClass::SpecialSymbols::NativeTypeCast);
			
		auto le = mt.getLimitExpression("c.size()", Types::ID::Dynamic);

		if (mt.isIntegerType())
		{
			String s;
			s << "int idx = " << mt.getWithLimit("this->value", le);
			cb << s;
		}
		else
		{
			String l1, l2;

			auto limit = mt.getLimitExpression("c.size()", Types::ID::Integer);

			if (mt.isNormalisedFloat())
				l1 << "auto scaled = (int)(this->value * " << mt.getWithCast(limit) << ")";
			else
				l1 << "auto scaled = (int)this->value";

			l2 << "auto idx = " << mt.getWithLimit("scaled", limit, Types::ID::Integer);

			cb << l1;
			cb << l2;
		}
		
		cb << "return c[idx]";

		return SyntaxTreeInlineParser(b, { "c" }, cb).flush();
	});

	gf.inliner->returnTypeFunction = [](InlineData* b)
	{
		auto rt = dynamic_cast<ReturnTypeInlineData*>(b);

		if (rt->object != nullptr)
		{
			if (auto a = rt->object->getSubExpr(1))
			{
				if (auto at = a->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
				{
					rt->f.returnType = at->getElementType().withModifiers(false, true, false);
					return Result::ok();
				}
			}
		}

		return Result::fail("Can't deduce array element type");
	};

	return gf;
}

}
}

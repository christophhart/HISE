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



struct ReturnTypeInlineData : public InlineData
{
	ReturnTypeInlineData(FunctionData& f_) :
		f(f_)
	{
		templateParameters = f.templateParameters;
	};

	bool isHighlevel() const override { return true; }

	Operations::Expression::Ptr object;
	FunctionData& f;
};





struct SyntaxTreeInlineData : public InlineData
{
	SyntaxTreeInlineData(Operations::Statement::Ptr e_, const NamespacedIdentifier& path_, const FunctionData& originalFunction_) :
		expression(e_),
		location(e_->location),
		path(path_),
		originalFunction(originalFunction_)
	{

	}

	NamespacedIdentifier getFunctionId() const
	{
		auto fc = dynamic_cast<const Operations::FunctionCall*>(expression.get());
		jassert(fc != nullptr);

		return fc->function.id;
	}

	bool isHighlevel() const override
	{
		return true;
	}

	/** Adds the function parameters as inlined parameters and converts the given syntax tree to a statement
	    block that will act as inlined function. 
	*/
	Result makeInlinedStatementBlock(SyntaxTree* syntaxTree, const Array<Symbol>& functionParameters)
	{
		using namespace Operations;
		
		auto fc = dynamic_cast<Operations::FunctionCall*>(expression.get());
		jassert(fc != nullptr);

		if (syntaxTree->getReturnType() == TypeInfo(Types::ID::Dynamic))
		{
			return Result::fail("must set return type before passing here");
		}

		target = syntaxTree->clone(location);
		auto cs = as<StatementBlock>(target);

		cs->setReturnType(syntaxTree->getReturnType());

		jassert(cs != nullptr);

		if (object != nullptr)
		{
			auto thisSymbol = Symbol("this");
			auto e = object->clone(location);
			cs->addInlinedParameter(-1, thisSymbol, dynamic_cast<Operations::Expression*>(e.get()));

			if (auto st = e->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				if (!as<ThisPointer>(e))
				{
					target->forEachRecursive([st, e](Operations::Statement::Ptr p)
					{
						if (auto v = dynamic_cast<Operations::VariableReference*>(p.get()))
						{
							auto canBeMember = st->id == v->id.id.getParent();
							auto hasMember = canBeMember && st->hasMember(v->id.id.getIdentifier());

							if (hasMember)
							{
								auto newParent = e->clone(v->location);
								auto newChild = v->clone(v->location);

								auto newDot = new Operations::DotOperator(v->location,
									dynamic_cast<Operations::Expression*>(newParent.get()),
									dynamic_cast<Operations::Expression*>(newChild.get()));

								v->replaceInParent(newDot);
							}
						}

						return false;
					});
				}
			}
		}

		for (int i = 0; i < fc->getNumArguments(); i++)
		{
			auto pVarSymbol = functionParameters[i];
			Operations::Expression::Ptr e = dynamic_cast<Operations::Expression*>(fc->getArgument(i)->clone(fc->location).get());
			cs->addInlinedParameter(i, pVarSymbol, e);
		}

		return Result::ok();
	}

	static void processUpToCurrentPass(Operations::Statement::Ptr currentStatement, Operations::Statement::Ptr e)
	{
		auto c = currentStatement->currentCompiler;
		auto s = currentStatement->currentScope;

		if (auto t = dynamic_cast<Operations::StatementBlock*>(e.get()))
			s = t->createOrGetBlockScope(s);

		for (int i = 0; i <= currentStatement->currentPass; i++)
		{
			auto thisPass = (BaseCompiler::Pass)i;
			BaseCompiler::ScopedPassSwitcher svs(c, thisPass);
			c->executePass(thisPass, s, e.get());
		};

		jassert(e->currentPass == currentStatement->currentPass);
	}

	bool replaceIfSuccess()
	{
		if (target != nullptr)
		{
			expression->replaceInParent(target);
			processUpToCurrentPass(expression, target);
			return true;
		}

		return false;
	}

	ParserHelpers::CodeLocation location;
	Operations::Statement::Ptr expression;
	Operations::Statement::Ptr target;
	Operations::Statement::Ptr object;
	ReferenceCountedArray<Operations::Expression> args;
	NamespacedIdentifier path;
	FunctionData originalFunction;
};

struct AsmInlineData : public InlineData
{
	AsmInlineData(AsmCodeGenerator& gen_) :
		gen(gen_)
	{};

	bool isHighlevel() const override
	{
		return false;
	}

	AsmCodeGenerator& gen;
	AssemblyRegister::Ptr target;
	AssemblyRegister::Ptr object;
	AssemblyRegister::List args;
};





SyntaxTreeInlineData* InlineData::toSyntaxTreeData() const
{
	jassert(isHighlevel());
	return dynamic_cast<SyntaxTreeInlineData*>(const_cast<InlineData*>(this));
}

AsmInlineData* InlineData::toAsmInlineData() const
{
	jassert(!isHighlevel());

	return dynamic_cast<AsmInlineData*>(const_cast<InlineData*>(this));
}

juce::Result Inliner::process(InlineData* d) const
{
	if (dynamic_cast<ReturnTypeInlineData*>(d))
	{
		return returnTypeFunction(d);
	}

	if (d->isHighlevel() && highLevelFunc)
		return highLevelFunc(d);

	if (!d->isHighlevel() && asmFunc)
		return asmFunc(d);

	return Result::fail("Can't inline function");
}



}
}

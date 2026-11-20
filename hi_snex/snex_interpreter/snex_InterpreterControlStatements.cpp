/** ===========================================================================
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
namespace interpreter {
namespace nodes {



struct SyntaxTree : public Statement
{
	SNEX_INTERPRETER_NODE(SyntaxTree, WantsNode::Yes);

	SyntaxTree(Statement::Factory& f, const ValueTree& v) :
	  Statement(f, v),
	  blockData(f, *this)
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{ 
		return blockData.perform(s, returnValue);
	};

	BlockExecutor blockData;

	int instructionPointer = 0;
	std::vector<UriLocation> locations;
};

struct StatementBlock : public DefinitionBase
{
	SNEX_INTERPRETER_NODE(StatementBlock, WantsNode::Yes);

	StatementBlock(Statement::Factory& f, const ValueTree& v) :
		DefinitionBase(f, v),
		blockData(f, *this),
		blockId(Helpers::getNamespacedIdentifier(v, AstIds::ScopeId))
	{};

	Result allocateInScope(Scope* scopeToUse) override
	{
		blockScope = new Scope(Scope::ScopeType::Block, scopeToUse, scopeToUse->engine, blockId);

		for (auto c : childStatements)
		{
			if (auto as = c->getTyped<DefinitionBase>())
			{
				auto ok = as->allocateInScope(blockScope);
				if (!ok.wasOk())
					return ok;
			}
		}

		blockScope->finalise();

		return Result::ok();
	}

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		return blockData.perform(s, returnValue);
	};

	BlockExecutor blockData;
	NamespacedIdentifier blockId;
	Scope::Ptr blockScope;
};

struct Function : public Statement
{
	SNEX_INTERPRETER_NODE(Function, WantsNode::Yes);

	Function(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v)
	{
		body = childStatements[0];
		fData = Helpers::getFunctionData(f, v);
		jassert(body != nullptr);
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		return body->perform(s, returnValue);
	};

	void annotateUriLocationTree(UriLocationTree& t) override
	{
		for (auto a : fData.args)
		{
			auto argDef = new UriLocationTree(&t);
			argDef->fillLocation(t);
			argDef->category = "parameter";
			argDef->id = a.id.toString();
			Helpers::annotateType(*argDef, a.typeInfo);
		}
	}

	jit::FunctionData fData;
	Ptr body;
};

struct TernaryOp : public Expression
{
	SNEX_INTERPRETER_NODE(TernaryOp, WantsNode::No);

	TernaryOp(Statement::Factory& f, const ValueTree& v) :
		Expression(f, v),
		condition(Helpers::getChildExpression(this, 0)),
		trueValue(Helpers::getChildExpression(this, 1)),
		falseValue(Helpers::getChildExpression(this, 2))
	{};

	TypeInfo getTypeInfo() const override { return trueValue->getTypeInfo(); }

	VariableStorage getResult(Scope* s) override
	{
		auto conditionValue = condition->getResult(s).toInt();

		if (conditionValue != 0)
			return trueValue->getResult(s);
		else
			return falseValue->getResult(s);
	}

	ExprPtr condition;
	ExprPtr trueValue;
	ExprPtr falseValue;
};

struct ReturnStatement : public Statement
{
	SNEX_INTERPRETER_NODE(ReturnStatement, WantsNode::No);

	ReturnStatement(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v),
		value(Helpers::getChildExpression(this, 0))
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		
		
		if (returnValue != nullptr && value != nullptr)
			*returnValue = value->getResult(s);
		
		return ResultCode::returnWasHit;
	};

	Expression::ExprPtr value;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex

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

struct IfStatement : public Statement
{
	SNEX_INTERPRETER_NODE(IfStatement, WantsNode::No);

	IfStatement(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v),
		condition(Helpers::getChildExpression(this, 0)),
		trueBranch(childStatements[1]),
		falseBranch(childStatements.size() == 3 ? childStatements[2] : nullptr)
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		auto conditionValue = condition->getResult(s).toInt();

		if (conditionValue != 0)
			return trueBranch->perform(s, returnValue);
		else if (falseBranch != nullptr)
			return falseBranch->perform(s, returnValue);
		else
			return ResultCode::ok;
	}

	Expression::ExprPtr condition;
	Statement::Ptr trueBranch;
	Statement::Ptr falseBranch;
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

 struct Loop : public DefinitionBase
{
	enum class LoopType
	{
		Undefined,
		Span,
		Dyn,
		Custom
	};

	SNEX_INTERPRETER_NODE(Loop, WantsNode::No);

	Loop(Statement::Factory& f, const ValueTree& v) :
		DefinitionBase(f, v),
		iterator(Helpers::getSymbol(f, v, AstIds::Iterator)),
		loadIterator(v[AstIds::LoadIterator]),
		elementType(Helpers::getType(f, v, AstIds::ElementType)),
		elementSize((int)v.getProperty(AstIds::ElementSize, -1)),
		numElements((int)v.getProperty(AstIds::NumElements, -1)),
		target(Helpers::getChildExpression(this, 0)),
		body(childStatements[1]->getTyped<StatementBlock>())
	{
		static const StringArray lt({ "Undefined", "Span", "Dyn", "CustomObject" });
		loopType = (LoopType)lt.indexOf(v[AstIds::LoopType].toString());
	}

	Result allocateInScope(Scope* scopeToUse) override
	{
		body->allocateInScope(scopeToUse);
		return body->blockScope->addReference(iterator);
	}

	void resolveDataPointers(Scope* scopeToUse) override
	{
		iteratorIndex = body->blockScope->getLValueReferenceIndex(iterator.id.id);
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		auto obj = target->getResult(s);

		void* beginPtr = nullptr;

		switch (loopType)
		{
		case LoopType::Span:
			beginPtr = obj.getDataPointer();
			break;
		case LoopType::Dyn:
			beginPtr = obj.toBlock().begin();
			numElements = obj.toBlock().size();
			break;
		default:
			jassertfalse;
		}

		if (numElements > 0)
		{
			auto& itLValue = body->blockScope->getLValueReference(iteratorIndex);

			for (int i = 0; i < numElements; i++)
			{
				itLValue = { beginPtr, i * elementSize};

				auto ok = body->perform(body->blockScope.get(), returnValue);

				if (ok == ResultCode::breakWasHit)
					break;

				if (ok == ResultCode::continueWasHit)
					continue;

				if (ok == ResultCode::returnWasHit)
					return ok;
			}
		}

		return ResultCode::ok;
	};

	int iteratorIndex = -1;
	Expression::ExprPtr target;
	StatementBlock* body;

	jit::Symbol iterator;
	const bool loadIterator;
	LoopType loopType;
	TypeInfo elementType;
	int elementSize;
	int numElements;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex

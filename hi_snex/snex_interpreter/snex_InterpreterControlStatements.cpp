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

struct StatementBlock : public Statement,
						public DefinitionBase
{
	SNEX_INTERPRETER_NODE(StatementBlock, WantsNode::Yes);

	StatementBlock(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v),
		blockData(f, *this),
		blockId(Helpers::getNamespacedIdentifier(v, AstIds::ScopeId))
	{};

	Scope::Ptr createScope(Scope* scopeToUse) override
	{
		return new Scope(Scope::ScopeType::Block, scopeToUse, scopeToUse->engine, blockId);
	}

	Result allocateInScope(Scope* scopeToUse) override
	{
		setAllocated();
		jassert(scopeToUse == statementScope.get());

		for (const auto& s : additionalSymbolsToAllocate)
		{
			jassert(s.id.getParent() == blockId);
			
			auto ok = statementScope->addSymbol(s);

			if (!ok.wasOk())
				return ok;
		}

		return Result::ok();
	}

	ResultCode perform(Scope* s, VariableStorage* returnValue) final override
	{
		return blockData.perform(statementScope, returnValue);
	};

	BlockExecutor blockData;
	NamespacedIdentifier blockId;

	// hack: the outer loop needs to pass its iterator in here so that it's allocated before finalising
	Array<jit::Symbol> additionalSymbolsToAllocate;
};

struct AnonymousBlock : public Statement
{
	SNEX_INTERPRETER_NODE(AnonymousBlock, WantsNode::No);

	AnonymousBlock(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v),
		blockData(f, *this)
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) final override
	{
		return blockData.perform(s, returnValue);
	};

	BlockExecutor blockData;

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

struct ControlFlowStatement : public Statement
{
	SNEX_INTERPRETER_NODE(ControlFlowStatement, WantsNode::No);

	ControlFlowStatement(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v),
		returnCode(v[AstIds::command].toString() == "continue" ? ResultCode::continueWasHit : ResultCode::breakWasHit)
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		for (auto c : childStatements)
			c->perform(s, nullptr);

		return returnCode;
	};

	const ResultCode returnCode;
};



struct ReturnStatement : public Statement
{
	SNEX_INTERPRETER_NODE(ReturnStatement, WantsNode::No);

	ReturnStatement(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v),
		value(Helpers::getType(f, v).getType() == Types::ID::Void ? nullptr : Helpers::getChildExpression(this, 0)),
		isRef(Helpers::isNativeTypeReference(f, v)),
		cleanupStart(value != nullptr ? 1 : 0)
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		if (returnValue != nullptr && value != nullptr)
		{
			if (isRef)
			{
				auto lValue = value->getLValue(s);
				*returnValue = lValue.readAsPointer();
			}
			else
			{
				*returnValue = value->getResult(s);
			}
		}

		performCleanup(s);
		return ResultCode::returnWasHit;
	};

	void performCleanup(Scope* s)
	{
		for (int i = cleanupStart; i < childStatements.size(); i++)
			childStatements.getUnchecked(i)->perform(s, nullptr);
	}

	Expression::ExprPtr value;
	const bool isRef;
	const int cleanupStart;
};

 struct Loop : public Statement
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
		Statement(f, v),
		iterator(Helpers::getSymbol(f, v, AstIds::Iterator)),
		loadIterator(v[AstIds::LoadIterator]),
		elementType(Helpers::getType(f, v, AstIds::ElementType)),
		elementSize((int)v.getProperty(AstIds::ElementSize, -1)),
		numElements((int)v.getProperty(AstIds::NumElements, -1)),
		target(Helpers::getChildExpression(this, 0)),
		body(childStatements[1]->getTyped<StatementBlock>()),
		copyIteratorValue(loadIterator && !elementType.isRef()),
		iteratorLoad(copyIteratorValue ? (elementType.getType() == Types::ID::Pointer ?
			OpFunctions::Data::readPointerValue : OpFunctions::Data::getFunctionPtr(elementType).first) : nullptr),
		customInlinerArgs(*this)
	{
		if (v[AstIds::Iterator].toString().startsWith("unresolved"))
			iterator.typeInfo = elementType;

		body->additionalSymbolsToAllocate.add(iterator);

		static const StringArray lt({ "Undefined", "Span", "Dyn", "CustomObject" });
		loopType = (LoopType)lt.indexOf(v[AstIds::LoopType].toString());

		if (loopType == LoopType::Custom)
		{
			customObject = dynamic_cast<StructType*>(Helpers::getObjectType(f, v));
			customInlinerArgs.argTypePtrs = { &customObject, 1 };
			
			if(customObject != nullptr)
			{
				beginFunction = OpFunctions::Inliners::getInliner(customObject, FunctionClass::SpecialSymbols::BeginIterator);
				sizeFunction = OpFunctions::Inliners::getInliner(customObject, FunctionClass::SpecialSymbols::SizeFunction);
			}
		}
	}

	void resolveDataPointers(Scope* scopeToUse) override
	{
		if (loopType == LoopType::Custom)
		{
			if (customObject == nullptr)
				throwRuntimeError("no object type");

			if (beginFunction == nullptr)
				throwRuntimeError("no begin() function");

			if (sizeFunction == nullptr)
				throwRuntimeError("no size() function");
		}

		iteratorRef = body->statementScope->resolveDataPointer(iterator.id);
		jassert(iteratorRef);
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
		{
			auto view = OpFunctions::Data::readDynView(obj);
			beginPtr = view.data;
			numElements = view.size;
			break;
		}
		case LoopType::Custom:
		{
			customInlinerArgs.args = { &obj, 1 };
			beginPtr = beginFunction(customInlinerArgs).getDataPointer();
			numElements = sizeFunction(customInlinerArgs).toInt();
			break;
		}
		default:
			throwRuntimeError("not implemented yet");
		}

		if (numElements > 0)
		{
			auto& itLValue = iteratorRef.getLValue();

			for (int i = 0; i < numElements; i++)
			{
				auto elementAddress = Scope::LValue { beginPtr, (size_t)i * elementSize };

				if (copyIteratorValue)
					iteratorRef.write(iteratorLoad(elementAddress));
				else
					iteratorRef.assign(elementAddress);

				auto ok = body->perform(body->statementScope.get(), returnValue);

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

	jit::Symbol iterator;
	const bool loadIterator;
	LoopType loopType;
	TypeInfo elementType;
	ComplexType* customObject = nullptr;
	int elementSize;
	int numElements;

	Scope::ResolvedReference iteratorRef;
	Expression::ExprPtr target;
	StatementBlock* body;
	const bool copyIteratorValue;
	const Scope::ReadFunction iteratorLoad;

	OpFunctions::Inliners::Args customInlinerArgs;
	OpFunctions::Inliners::Prototype beginFunction = nullptr, sizeFunction = nullptr;
};

struct WhileLoop : public Statement
{
	SNEX_INTERPRETER_NODE(WhileLoop, WantsNode::No);

	WhileLoop(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v)
	{
		auto isWhileLoop = v[AstIds::LoopType].toString() == "While";

		if (isWhileLoop)
		{
			condition = Helpers::getChildExpression(this, 0);
			body = childStatements[1];
		}
		else
		{
			initialiser = childStatements[0];
			condition = Helpers::getChildExpression(this, 1);
			body = childStatements[2];
			postOp = childStatements[3];

			// This is required for the scope allocation to work
			// the while loop has the statement body as 3rd child but the initialiser
			// needs the scope to be processed and allocated
			childStatements.swap(1, 2);
			childStatements.swap(0, 1);

			if (auto as = initialiser->getTyped<Assignment>())
			{
				if (auto v = initialiser->childStatements[1]->getTyped<VariableReference>())
				{
					if (auto bodyBlock = body->getTyped<StatementBlock>())
						bodyBlock->additionalSymbolsToAllocate.add(v->symbol);
					else
						throwRuntimeError("for loop body has no scope");
				}
			}
		}
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{ 
		static const int RecursionProtection = 10000000;
		int loopCounter = 0;

		if (initialiser != nullptr)
			initialiser->perform(s, returnValue);

		auto v = condition->getResult(s).toInt();

		while (v != 0)
		{
			if (loopCounter++ > RecursionProtection)
				throwRuntimeError("infinite loop detected!");

			auto rc = body->perform(s, returnValue);

			if (rc == ResultCode::breakWasHit)
				break;

			if (rc == ResultCode::returnWasHit)
				return rc;

			if (postOp != nullptr)
				postOp->perform(s, returnValue);

			v = condition->getResult(s).toInt();
		}

		return ResultCode::ok;
	};

	Statement::Ptr initialiser;
	Expression::ExprPtr condition;
	Statement::Ptr body;
	Statement::Ptr postOp;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex

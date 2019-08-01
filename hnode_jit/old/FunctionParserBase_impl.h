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

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;


// Handy macro. Use this to #define a if statement that returns a typed node and undef it afterwards...

#if INCLUDE_BUFFERS
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int) MATCH_TYPE_AND_RETURN(Buffer*) MATCH_TYPE_AND_RETURN(BooleanType)
#elif INCLUDE_CONDITIONALS
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int) MATCH_TYPE_AND_RETURN(BooleanType)
#else
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int)
#endif

FunctionParserBase::FunctionParserBase(JITScope::Pimpl* scope_, const FunctionParserInfo& info_) :
	TokenIterator(info_.code, info_.code, info_.length),
	BaseScope({}, scope_, 1024),
	scope(scope_),
	info(info_)
{

	
}

void FunctionParserBase::parseFunctionBody()
{
	lines.clear();

	while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
	{
		if (currentType == JitTokens::identifier)
		{
			Identifier id = Identifier(currentValue);
			Identifier className;

			if (matchIf(JitTokens::dot))
			{
				className = id;
				id == parseIdentifier();
			}

			if (auto l = getLine(id))
			{
				if (JITTypeHelpers::matchesType<float>(l->getType())) parseLineAssignment<float>(l);
				if (JITTypeHelpers::matchesType<double>(l->getType())) parseLineAssignment<double>(l);
				if (JITTypeHelpers::matchesType<int>(l->getType())) parseLineAssignment<int>(l);
#if HNODE_BOOL_IS_NOT_INT
				if (JITTypeHelpers::matchesType<BooleanType>(l->getType())) parseLineAssignment<BooleanType>(l);
#endif
			}
#if 0
			else if (scope->getScopeForSymbol(className, id))
			{
				parseAssignmentFunky(scope->get(className, id));
			}
#endif
			else
			{
				parseUntypedLine();
			}
#if 0
			if (auto g = scope->getGlobal(id))
			{
				if (JITTypeHelpers::matchesType<float>(g->type)) parseGlobalAssignment<float>(g);
				if (JITTypeHelpers::matchesType<double>(g->type)) parseGlobalAssignment<double>(g);
				if (JITTypeHelpers::matchesType<int>(g->type)) parseGlobalAssignment<int>(g);
#if HNODE_BOOL_IS_NOT_INT
				if (JITTypeHelpers::matchesType<BooleanType>(g->type)) parseGlobalAssignment<BooleanType>(g);
#endif
#if INCLUDE_BUFFERS
				if (JITTypeHelpers::matchesType<Buffer*>(g->type)) parseBufferLine(id);
#endif

			}
#endif
		}
		else if (matchIf(JitTokens::const_))
		{
			if (matchIf(JitTokens::float_))		parseLine<float>(true);
			else if (matchIf(JitTokens::int_))	parseLine<int>(true);
			else if (matchIf(JitTokens::double_))	parseLine<double>(true);
			else if (matchIf(JitTokens::bool_))	parseLine<BooleanType>(true);

		}
		else if (matchIf(JitTokens::float_))		parseLine<float>(false);
		else if (matchIf(JitTokens::int_))	parseLine<int>(false);
		else if (matchIf(JitTokens::double_))	parseLine<double>(false);
		else if (matchIf(JitTokens::bool_))	parseLine<BooleanType>(false);
		else if (matchIf(JitTokens::return_)) parseReturn();
		else
		{
			parseUntypedLine();
		}

		//else match(HiseJitTokens::eof);

	}
}

int FunctionParserBase::getParameterIndex(const Identifier& id)
{
	return info.parameterNames.indexOf(id);
}


BaseNodePtr FunctionParserBase::getParameterNode(const Identifier& id)
{
	for (int i = 0; i < parameterNodes.size(); i++)
	{
		if (parameterNodes[i]->getId() == id)
		{
			return parameterNodes[i];
		}
	}

	return nullptr;
}

#define ScopedTypedNodePointer(x) ScopedPointer<AsmJitHelpers::TypedNode<x>>

template <typename T>
void FunctionParserBase::parseLine(bool isConst)
{
	Identifier id = parseIdentifier();

	auto existingLine = getLine(id);

	if (existingLine != nullptr)
	{
		location.throwError("Identifier " + id.toString() + " already defined");
	}

	match(JitTokens::assign_);

	ScopedTypedNodePointer(T) r = parseTypedExpression<T>();
	
	r->setId(id);

	if (isConst)
		r->setConst();
	
	lines.add(r.release());

	match(JitTokens::semicolon);
}


void FunctionParserBase::parseAssignmentFunky(BaseScope::Reference target)
{
	Identifier id = parseIdentifier();

	AsmJitHelpers::BaseNode::Ptr l = getOrCreateVariableNode(target);

	bool isPostDec = matchIf(JitTokens::minusminus);
	bool isPostInc = matchIf(JitTokens::plusplus);

	if (isPostInc || isPostDec)
	{
		if (target.getType() == Types::ID::Integer)
		{
			if (isPostInc) AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(l));
			else		   AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(l));

			match(JitTokens::semicolon);
			return;
		}
		else
		{
			location.throwError("Can't increment / decrement non integer types!");
		}
	}

	TokenType assignType;// = parseAssignType();
	AsmJitHelpers::BaseNode::Ptr n = parseExpression();

	if (assignType != JitTokens::assign_)
	{
		ScopedBaseNodePointer n2 = createBinaryNode(l, n, assignType);

		l->setRegister(n2->getRegister());

	}
	else
	{
		l->setRegister(n->getRegister());
	}

	match(JitTokens::semicolon);

}


TYPED_VOID FunctionParserBase::parseLineAssignment(AsmJitHelpers::BaseNode* l)
{
	if (l->isConst())
	{
		location.throwError("Can't assign to const variable " + l->getId());
	}

	Identifier id = parseIdentifier();

	bool isPostDec = matchIf(JitTokens::minusminus);
	bool isPostInc = matchIf(JitTokens::plusplus);

	if (isPostInc || isPostDec)
	{
		if (JITTypeHelpers::is<T, int>())
		{
			if (isPostInc) AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(l));
			else		   AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(l));

			match(JitTokens::semicolon);
			return;
		}
		else
		{
			location.throwError("Can't increment / decrement non integer types!");
		}
	}

	TokenType assignType; // parseAssignType();
	ScopedTypedNodePointer(T) n = parseTypedExpression<T>();

	if (assignType != JitTokens::assign_)
	{
		ScopedBaseNodePointer n2 = createBinaryNode(l, n, assignType);
		auto n3 = getTypedNode<T>(n2);
		auto lTyped = getTypedNode<T>(l);
		
		lTyped->setRegister(n3->getRegister());
		
	}
	else
	{
		auto lTyped = getTypedNode<T>(l);

		lTyped->setRegister(n->getRegister());
	}

	match(JitTokens::semicolon);
}

void FunctionParserBase::parseUntypedLine()
{
	ScopedPointer<AsmJitHelpers::BaseNode> expr = parseExpression();
	match(JitTokens::semicolon);
}

BaseNodePtr FunctionParserBase::parseExpression()
{
	return parseTernaryOperator();
}

#if 0
void FunctionParserBase::checkAllLinesReferenced()
{
	for (int i = 0; i < lines.size(); i++)
	{
		if (!lines[i]->node->IsReferenced())
		{
			location.throwError("Unused variable: " + lines[i]->id.toString());
		}
	}
}
#endif

BaseNodePtr FunctionParserBase::parseSum()
{
	ScopedBaseNodePointer left(parseDifference());

	if (currentType == JitTokens::plus)
	{
		TokenType op = currentType;
		skip();

		ScopedBaseNodePointer right(parseSum());

		return createBinaryNode(left, right, op);
	}
	else
	{
		return left.release();
	}
}

BaseNodePtr FunctionParserBase::parseDifference()
{
	ScopedBaseNodePointer left(parseProduct());

	if (currentType == JitTokens::minus)
	{
		TokenType op = currentType;
		skip();

		ScopedBaseNodePointer right(parseDifference());

		return createBinaryNode(left, right, op);
	}
	else
	{
		return left.release();
	}
};

BaseNodePtr FunctionParserBase::parseProduct()
{
	ScopedBaseNodePointer left = parseTerm();

	if (currentType == JitTokens::times ||
		currentType == JitTokens::divide ||
		currentType == JitTokens::modulo)
	{
		TokenType op = currentType;
		skip();

		ScopedBaseNodePointer right = parseProduct();

		return createBinaryNode(left, right, op);
	}
	else
	{
		return left.release();
	}
}


BaseNodePtr FunctionParserBase::parseTernaryOperator()
{
	ScopedBaseNodePointer condition = parseBool();

	if (matchIf(JitTokens::question))
	{
		auto conditionTyped = getTypedNode<BooleanType>(condition);

		if (conditionTyped != nullptr)
		{
			if (conditionTyped->isImmediateValue())
			{
				const bool isTrue = (int)conditionTyped->getImmediateValue() > 0;

				if (isTrue)
				{
					ScopedBaseNodePointer trueBranch = parseExpression();
					match(JitTokens::colon);

					auto skipFalse = asmCompiler->newLabel();

					asmCompiler->jmp(skipFalse);
					ScopedBaseNodePointer falseBranch = parseExpression();
					asmCompiler->bind(skipFalse);

					return trueBranch.release();
				}
				else
				{
					auto skipTrue = asmCompiler->newLabel();

					asmCompiler->jmp(skipTrue);
					ScopedBaseNodePointer trueBranch = parseExpression();
					asmCompiler->bind(skipTrue);
					match(JitTokens::colon);

					ScopedBaseNodePointer falseBranch = parseExpression();
					
					return falseBranch.release();
				}
			}


			asmjit::Error error;

			auto l = asmCompiler->newLabel();
			auto r = asmCompiler->newLabel();
			auto e = asmCompiler->newLabel();

			auto intResult = asmCompiler->newInt32();
			auto doubleResult = asmCompiler->newXmmSd();
			auto floatResult = asmCompiler->newXmmSs();
			auto boolResult = asmCompiler->newGpb();



			error = asmCompiler->cmp(conditionTyped->getAsGenericRegister(), 1);
			ASSERT_ASM_OK;

			error = asmCompiler->jnz(r);
			ASSERT_ASM_OK;

			error = asmCompiler->bind(l);
			ASSERT_ASM_OK;

			ScopedBaseNodePointer left = parseExpression();
			
			if (JITTypeHelpers::matchesType<int>(left->getType()))				AsmJitHelpers::BinaryOpInstructions::Int::store(*asmCompiler, intResult, left);
			else if (JITTypeHelpers::matchesType<float>(left->getType()))		AsmJitHelpers::BinaryOpInstructions::Float::store(*asmCompiler, floatResult, left);
			else if (JITTypeHelpers::matchesType<double>(left->getType()))		AsmJitHelpers::BinaryOpInstructions::Double::store(*asmCompiler, doubleResult, left);
			else if (JITTypeHelpers::matchesType<BooleanType>(left->getType())) AsmJitHelpers::BinaryOpInstructions::Bool::store(*asmCompiler, boolResult, left);

			ASSERT_ASM_OK;

			error = asmCompiler->jmp(e);
			ASSERT_ASM_OK;

			match(JitTokens::colon);

			error = asmCompiler->bind(r);
			ASSERT_ASM_OK;

			ScopedBaseNodePointer right = parseExpression();
			
			if (JITTypeHelpers::matchesType<int>(right->getType()))				AsmJitHelpers::BinaryOpInstructions::Int::store(*asmCompiler, intResult, right);
			else if (JITTypeHelpers::matchesType<float>(right->getType()))		AsmJitHelpers::BinaryOpInstructions::Float::store(*asmCompiler, floatResult, right);
			else if (JITTypeHelpers::matchesType<double>(right->getType()))		AsmJitHelpers::BinaryOpInstructions::Double::store(*asmCompiler, doubleResult, right);
			else if (JITTypeHelpers::matchesType<BooleanType>(right->getType())) AsmJitHelpers::BinaryOpInstructions::Bool::store(*asmCompiler, boolResult, right);
			
			error = asmCompiler->bind(e);
			ASSERT_ASM_OK;

			TypeInfo tl = getTypeForNode(left);
			TypeInfo tr = getTypeForNode(right);

			if (tr == tl)
			{
				if (JITTypeHelpers::matchesType<int>(tr)) return new AsmJitHelpers::TypedNode<int>(intResult);
				if (JITTypeHelpers::matchesType<double>(tr)) return new AsmJitHelpers::TypedNode<double>(doubleResult);
				if (JITTypeHelpers::matchesType<float>(tr)) return new AsmJitHelpers::TypedNode<float>(floatResult);
				if (JITTypeHelpers::matchesType<BooleanType>(tr)) return new AsmJitHelpers::TypedNode<BooleanType>(boolResult);
			}
			else
			{
				THROW_PARSING_ERROR(JITTypeHelpers::getTypeMismatchErrorMessage(tl, tr), nullptr);
			}
		}
		else
		{
			THROW_PARSING_ERROR("Condition must be a bool", nullptr);
		}
	}
	else
	{
		return condition.release();
	}
}



template <typename ExpectedType> BaseNodePtr FunctionParserBase::parseCast()
{
	match(JitTokens::closeParen);

	ScopedBaseNodePointer source = parseTerm();

	TypeInfo sourceType = getTypeForNode(source);

	if (JITTypeHelpers::matchesType<ExpectedType>(sourceType))
		return source.release();

	if (JITTypeHelpers::matchesType<int>(sourceType))
		return AsmJitHelpers::EmitCast<int, ExpectedType>(*asmCompiler, getTypedNode<int>(source.get()));

    if (JITTypeHelpers::matchesType<double>(sourceType))
        return AsmJitHelpers::EmitCast<double, ExpectedType>(*asmCompiler, getTypedNode<double>(source.get()));

    if (JITTypeHelpers::matchesType<float>(sourceType))
        return AsmJitHelpers::EmitCast<float, ExpectedType>(*asmCompiler, getTypedNode<float>(source.get()));

	THROW_PARSING_ERROR("Unsupported type for cast", nullptr);	
}




BaseNodePtr FunctionParserBase::parseTerm()
{
	if (matchIf(JitTokens::openParen))
	{
		if (matchIf(JitTokens::float_))  return parseCast<float>();
		if (matchIf(JitTokens::int_))    return parseCast<int>();
		if (matchIf(JitTokens::double_)) return parseCast<double>();
		else
		{
			auto result = parseExpression();
            match(JitTokens::closeParen);
            return result;
		}
	}
    else
    {
        return parseUnary();
    }
}

BaseNodePtr FunctionParserBase::parseUnary()
{
    if (currentType == JitTokens::identifier ||
        currentType == JitTokens::literal ||
		currentType == JitTokens::minus ||
		currentType == JitTokens::plusplus ||
		currentType == JitTokens::minusminus)
    {
        return parseFactor();
    }
#if INCLUDE_CONDITIONALS
	else if (matchIf(JitTokens::true_))
	{
		return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 1);
	}
	else if (matchIf(JitTokens::false_))
	{
		return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 0);
	}
	else if (currentType == JitTokens::logicalNot)
	{
		return parseBool();
	}
#endif
	else
    {
        location.throwError("Parsing error");
    }
    
	return nullptr;
}

BaseNodePtr FunctionParserBase::parseFactor()
{	
	const bool isMinus = matchIf(JitTokens::minus);
	
	auto expr = parseSymbolOrLiteral();

	if(isMinus)
	{
		TypeInfo t = getTypeForNode(expr);

#define MATCH_TYPE_AND_RETURN(type) if(JITTypeHelpers::matchesType<type>(t)) return AsmJitHelpers::Negate<type>(*asmCompiler, expr);

		MATCH_TYPE_AND_RETURN(int)
		MATCH_TYPE_AND_RETURN(double)
		MATCH_TYPE_AND_RETURN(float)

#undef MATCH_TYPE_AND_RETURN

		THROW_PARSING_ERROR("Unsupported type for negation", nullptr);

	}
	else
	{
		return expr;
	}
}

BaseNodePtr FunctionParserBase::parseSymbolOrLiteral()
{
	

	ScopedBaseNodePointer expr;

	if (matchIf(JitTokens::literal))
	{
		expr = parseLiteral();
	}
	else
	{
		expr = parseSymbol();
	}

	return expr.release();
}

BaseNodePtr FunctionParserBase::getNodeForLine(NamedNode* r)
{
	return r->clone();
}

BaseNodePtr FunctionParserBase::parseSymbol()
{
	bool preInc = matchIf(JitTokens::plusplus);
	bool preDec = matchIf(JitTokens::minusminus);

	Identifier symbolId = parseIdentifier();
	Identifier className;

	if (matchIf(JitTokens::dot))
	{
		className = symbolId;
		symbolId = parseIdentifier();
	}

	bool postInc = matchIf(JitTokens::plusplus);
	bool postDec = matchIf(JitTokens::minusminus);


	if (info.parameterNames.indexOf(symbolId) != -1)		return parseParameterReference(symbolId);
	if (auto r = getLine(symbolId))
	{
		ScopedBaseNodePointer line = getNodeForLine(r);

		if (preInc || postInc)      AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(line));
		else if (preDec || postDec) AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(line));
		
		return line.release();
	}
	
#if 0
	else if ( auto g = scope->getGlobal(symbolId))
	{
#if INCLUDE_BUFFERS
		if (JITTypeHelpers::matchesType<Buffer*>(g->getType()))
			return parseBufferOperation(symbolId);
#endif

		ScopedBaseNodePointer expr;

		if (auto gn = getGlobalNode(symbolId))
		{
			expr = getNodeForLine(gn);
		}
		else
		{
			expr = getGlobalReference(symbolId);
		}

		if (preInc || postInc) // don't care about the position
		{
			BaseNodePtr node = getGlobalNode(symbolId);

			AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(node));

			node->setIsChangedGlobal();
		}
		else if (preDec || postDec)
		{
			AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(expr.get()));
			expr->setIsChangedGlobal();
		}

		return expr.release();
}
#endif


	else if (matchIf(JitTokens::openParen) && scope->hasFunction(className, symbolId))
	{
		AsmJitHelpers::ParameterInfo pi;
		while (currentType != JitTokens::closeParen)
		{
			auto e = parseExpression();

			TypeInfo t = getTypeForNode(e);
			auto t_ = JITTypeHelpers::convertToHnodeType(t);

			pi.data.args.add(t_);
			pi.nodes.add(e);

			matchIf(JitTokens::comma);
		}

		match(JitTokens::closeParen);

		Array<FunctionData> matches;

		scope->addMatchingFunctions(matches, className, symbolId);

		for (auto m : matches)
		{
			if (m.matchesArgumentTypes(pi.data.args))
			{
				pi.data = m;
				return parseFunctionCall(pi);
			}
		}
		

#if 0
			if (auto cb = scope->getCompiledBaseFunction(className))
			{
				pi.data.returnType = cb->returnType;

				if (pi.data.matchesSignature(*cb))
				{
					pi.data.id = cb->id;
					pi.data.function = cb->function;
					return parseFunctionCall(pi);
				}
			}
			if (auto b = scope->getExposedFunction(className, symbolId, pi))
			{
				pi.data.returnType = b->returnType;
				return parseFunctionCall(pi);
			}
			else if (auto objFunc = scope->getMemberFunction(className, symbolId, pi))
			{
				pi.data.returnType = objFunc->data.returnType;
				return parseMemberFunctionCall(objFunc, pi);
			}
			else
			{
				String s;

				s << "No matching function found for arguments: ";

				for (auto args : pi.data.args)
					s << Types::Helpers::getCppTypeName(args) << " ";

				location.throwError(s);
			}
#endif

		String s;
		s << "Unknown function: " << className << "." << symbolId;
		location.throwError(s);
	}
	
	else
	{
		location.throwError("Unknown identifier " + symbolId.toString());
	}

	return nullptr;
}



hnode::jit::BaseNodePtr FunctionParserBase::parseReferenceFunky(const BaseScope::Reference& ref)
{
	return getOrCreateVariableNode(ref);
}



BaseNodePtr FunctionParserBase::parseParameterReference(const Identifier &id)
{
	BaseNodePtr existingParameterNode = getParameterNode(id);

	if (existingParameterNode != nullptr)
	{
		return existingParameterNode->clone();
	}

	if (auto newParameterNode = parseParameterReferenceTyped(id))
	{
		parameterNodes.add(newParameterNode);

		return newParameterNode->clone();
	}

	location.throwError("Parameter reference " + id + " could not be parsed");
	
}

#if INCLUDE_BUFFERS
BaseNodePtr FunctionParserBase::parseBufferOperation(const Identifier &id)
{
	if (matchIf(JitTokens::openBracket))
	{
		return parseBufferAccess(id);
	}
	else
	{
		match(JitTokens::dot);

		return parseBufferFunction<float>(id);
	}
}
#endif

BaseNodePtr FunctionParserBase::createBinaryNode(BaseNodePtr a, BaseNodePtr b, TokenType op)
{
	TypeInfo ta = getTypeForNode(a);
	TypeInfo tb = getTypeForNode(b);

	if (ta != tb)
	{
		if ((JITTypeHelpers::matchesType<float>(ta) &&
			JITTypeHelpers::matchesType<double>(tb)))
		{
			b = AsmJitHelpers::EmitCast<double, float>(*asmCompiler, getTypedNode<double>(b));
			tb = typeid(float);
		}
		else if ((JITTypeHelpers::matchesType<double>(ta) &&
			JITTypeHelpers::matchesType<float>(tb)))
		{
			a = AsmJitHelpers::EmitCast<float, double>(*asmCompiler, getTypedNode<float>(a));
			ta = typeid(float);
		}
		else
		{
			THROW_PARSING_ERROR("Type mismatch for binary operation " + String(op), nullptr);
		}
	}
	
	if (JITTypeHelpers::matchesType<int>(ta)) return createTypedBinaryNode<int>(getTypedNode<int>(a), getTypedNode<int>(b), op);
	else if (JITTypeHelpers::matchesType<double>(ta)) return createTypedBinaryNode<double>(getTypedNode<double>(a), getTypedNode<double>(b), op);
	else if (JITTypeHelpers::matchesType<float>(ta)) return createTypedBinaryNode<float>(getTypedNode<float>(a), getTypedNode<float>(b), op);
	else if (JITTypeHelpers::matchesType<BooleanType>(ta)) return createTypedBinaryNode<BooleanType>(getTypedNode<BooleanType>(a), getTypedNode<BooleanType>(b), op);
	else
	{
		THROW_PARSING_ERROR("Unsupported type for binary operation", nullptr);
	}
}



template <typename T> BaseNodePtr FunctionParserBase::createTypedBinaryNode(TypedNodePtr left, TypedNodePtr right, TokenType op)
{
	if (left->isImmediateValue() && right->isImmediateValue())
	{
		if (op == JitTokens::plus)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, 
				(T)left->getImmediateValue() + 
				(T)right->getImmediateValue());
		}
		if (op == JitTokens::minus)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, 
				(T)left->getImmediateValue() - 
				(T)right->getImmediateValue());
		}
		if (op == JitTokens::times)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, 
				(T)left->getImmediateValue() * 
				(T)right->getImmediateValue());
		}
		if (op == JitTokens::divide)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, 
				(T)left->getImmediateValue() / 
				(T)right->getImmediateValue());
		}
		if (op == JitTokens::modulo)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, 
				left->getImmediateValue().toInt() % 
				right->getImmediateValue().toInt());
		}
		else if (op == JitTokens::greaterThan)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				(T)left->getImmediateValue() > 
				(T)right->getImmediateValue() ? 1 : 0);
		}
		else if (op == JitTokens::greaterThanOrEqual)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				(T)left->getImmediateValue() >=
				(T)right->getImmediateValue() ? 1 : 0);
		}
		else if (op == JitTokens::lessThan)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				(T)left->getImmediateValue() <
				(T)right->getImmediateValue() ? 1 : 0);
		}
		else if (op == JitTokens::lessThanOrEqual)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				(T)left->getImmediateValue() <=
				(T)right->getImmediateValue() ? 1 : 0);
		}
		else if (op == JitTokens::equals)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				(T)left->getImmediateValue() ==
				(T)right->getImmediateValue() ? 1 : 0);
		}
		else if (op == JitTokens::notEquals)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				(T)left->getImmediateValue() !=
				(T)right->getImmediateValue() ? 1 : 0);
		}
		else if (op == JitTokens::logicalAnd)
		{
			if (JITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, (
					(T)left->getImmediateValue() != 0) && 
					(T)(right->getImmediateValue()) != 0 ? 1 : 0);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}
		else if (op == JitTokens::logicalOr)
		{
			if (JITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 
				((T)left->getImmediateValue() != 0) || 
				((T)right->getImmediateValue()) || 0 ? 1 : 0);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}

	}
	else
	{
		if (op == JitTokens::plus)
		{
			return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Add>(*asmCompiler, left, right);
		}
		if (op == JitTokens::minus)
		{
			return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Sub>(*asmCompiler, left, right);
		}
		if (op == JitTokens::times)
		{
			return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Mul>(*asmCompiler, left, right);
		}
		if (op == JitTokens::divide)
		{
			return AsmJitHelpers::EmitBinaryOp < AsmJitHelpers::Div>(*asmCompiler, left, right);
		}
		if (op == JitTokens::modulo)
		{
			return AsmJitHelpers::EmitBinaryOp < AsmJitHelpers::Mod>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::greaterThan)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::GT>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::greaterThanOrEqual)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::GTE>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::lessThan)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::LT>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::lessThanOrEqual)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::LTE>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::equals)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::EQ>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::notEquals)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::NEQ>(*asmCompiler, left, right);
		}
		else if (op == JitTokens::logicalAnd)
		{
			if (JITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Opcodes::And>(*asmCompiler, left, right);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}
		else if (op == JitTokens::logicalOr)
		{
			if (JITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Opcodes::Or>(*asmCompiler, left, right);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}

	}

	
	return nullptr;
}

TYPED_NODE FunctionParserBase::getTypedNode(BaseNodePtr node)
{
	ScopedBaseNodePointer n(node);

	auto r = dynamic_cast<TypedNodePtr>(node);

	if (r != nullptr)
	{
		n.release();
		return r;
	}
	
	if (JITTypeHelpers::is<float, T>() && 
		JITTypeHelpers::matchesType<double>(node->getType()))
	{
		if (auto r = getTypedNode<double>(node))
		{
			n.release();
			return AsmJitHelpers::EmitCast<double, T>(*asmCompiler, r);
		}
		else
			jassertfalse;
	}

	if (JITTypeHelpers::is<double, T>() &&
		JITTypeHelpers::matchesType<float>(node->getType()))
	{
		if (auto r = getTypedNode<float>(node))
		{
			n.release();
			return AsmJitHelpers::EmitCast<float, T>(*asmCompiler, r);
		}
		else
			jassertfalse;
	}

	String s;

	s << "Casting error. Expected: " << JITTypeHelpers::getTypeName<T>();
	s << " - Actual: " << JITTypeHelpers::getTypeName(node->getType());

	THROW_PARSING_ERROR(s, nullptr);
}



TYPED_NODE FunctionParserBase::getCastedNode(BaseNodePtr node)
{
#if 0
	TypeInfo nodeType = getTypeForNode(node);

	if (TYPE_MATCH(Buffer*, nodeType)) 
	{
		location.throwError("Can't cast buffers");
	}

	// Skip cast if not required
	if (TYPE_MATCH(T, nodeType)) return getTypedNode<T>(node);


#define MATCH_TYPE_AND_RETURN(type) if (TYPE_MATCH(type, nodeType)) return exprBase->Cast<T, type>(getTypedNode<type>(node));

	MATCH_TYPE_AND_RETURN(int);
	MATCH_TYPE_AND_RETURN(float);
	MATCH_TYPE_AND_RETURN(double);
	MATCH_TYPE_AND_RETURN(BooleanType);

#undef MATCH_TYPE_AND_RETURN

#endif

	location.throwError("No cast possible");
	return nullptr;

}

TypeInfo FunctionParserBase::getTypeForNode(BaseNodePtr node)
{
	return node->getType();
}

bool FunctionParserBase::nodesHaveSameType(BaseNodePtr a, BaseNodePtr b)
{
	return getTypeForNode(a) == getTypeForNode(b);
}

TYPED_NODE FunctionParserBase::parseTypedExpression()
{
	ScopedBaseNodePointer e = parseExpression();

	ScopedTypedNodePointer(T) typed = getTypedNode<T>(e.release());

	if (typed != nullptr)
		return typed.release();
	
	location.throwError("Expression type mismatch: Expected " + JITTypeHelpers::getTypeName<T>());
	return nullptr;
}

BaseNodePtr FunctionParserBase::parseBool()
{
#if INCLUDE_CONDITIONALS
	const bool isInverted = matchIf(JitTokens::logicalNot);

	ScopedBaseNodePointer result = parseLogicOperation();

	if (!isInverted)
	{
		return result.release();
	}
	else
	{
		ScopedPointer<AsmJitHelpers::TypedNode<BooleanType>> resultTyped = getTypedNode<BooleanType>(result.release());

		if (resultTyped != nullptr)
		{
			AsmJitHelpers::invertBool(*asmCompiler, resultTyped);

			return resultTyped.release();
		}
		else
		{
			location.throwError("Can't negate non boolean expressions");
		}
	}
#endif

	return nullptr;
}


BaseNodePtr FunctionParserBase::parseLogicOperation()
{
#if INCLUDE_CONDITIONALS
	ScopedBaseNodePointer left = parseComparation();

	if (matchIf(JitTokens::logicalAnd))
	{
		ScopedBaseNodePointer right = parseLogicOperation();
		return createBinaryNode(left, right, JitTokens::logicalAnd);
	}
	else if (matchIf(JitTokens::logicalOr))
	{
		ScopedBaseNodePointer right = parseLogicOperation();
		return createBinaryNode(left, right, JitTokens::logicalOr);
	}
	else
	{
		return left.release();
	}
#endif

	return nullptr;
}


BaseNodePtr FunctionParserBase::parseComparation()
{
#if INCLUDE_CONDITIONALS
	ScopedBaseNodePointer left = parseSum();

	if (currentType == JitTokens::greaterThan ||
		currentType == JitTokens::greaterThanOrEqual ||
		currentType == JitTokens::lessThan ||
		currentType == JitTokens::lessThanOrEqual ||
		currentType == JitTokens::equals ||
		currentType == JitTokens::notEquals)
	{
		TokenType op = currentType;
		skip();
		ScopedBaseNodePointer right = parseSum();
		return createBinaryNode(left, right, op);

	}
	else
	{
		return left.release();
	}
#endif

	return nullptr;
}

#if INCLUDE_BUFFERS
AsmJitHelpers::TypedNode<Buffer*>* FunctionParserBase::getBufferNode(const Identifier& id)
{
	auto r = scope->getGlobal(id);

#if 0

	auto& e1 = exprBase->Immediate(GlobalBase::getBuffer);
	auto& g = exprBase->Immediate<GlobalBase*>(r);

	return exprBase->Call(e1, g);
#endif

	return nullptr;
}

int bufferOverflow(int offsetLength, int maxSize)
{
	int x = 2;

	String errorMessage;

	errorMessage << "Buffer overflow: " << String(offsetLength) << ", size: " << String(maxSize);

	juce::Logger::getCurrentLogger()->writeToLog(errorMessage);

	return 1;
}

AsmJitHelpers::TypedNode<float>* FunctionParserBase::parseBufferAccess(const Identifier &id)
{

	ScopedTypedNodePointer(int) index = parseTypedExpression<int>();
	match(JitTokens::closeBracket);

	auto g = scope->getGlobal(id);

	if (JITTypeHelpers::matchesType<Buffer*>(g->getType())) // g->isConst && 
	{
		ScopedTypedNodePointer(uint64_t) existingDataNode = getBufferDataNode(id);

		

		if (existingDataNode == nullptr)
		{
			existingDataNode = AsmJitHelpers::getBufferData(*asmCompiler, g);
			existingDataNode->setId(id);
			bufferDataNodes.add(getTypedNode<uint64_t>(existingDataNode->clone()));
		}

		if (info.useSafeBufferFunctions)
		{
			ScopedTypedNodePointer(int) dataSize = AsmJitHelpers::getBufferDataSize(*asmCompiler, g);

			return AsmJitHelpers::BufferAccess(*asmCompiler, existingDataNode, index, dataSize, bufferOverflow);
		}
		else
		{
			return AsmJitHelpers::BufferAccess(*asmCompiler, existingDataNode, index);
		}

		
	}

	return nullptr;
}

template <typename T> BaseNodePtr FunctionParserBase::parseBufferFunction(const Identifier& id)
{
	const Identifier functionName = parseIdentifier();

	static const Identifier setSize("setSize");

	if (functionName == setSize)
	{
		auto b = getBufferNode(id);

	}

	return nullptr;
}

void FunctionParserBase::parseBufferAssignment(const Identifier &id)
{
	ScopedTypedNodePointer(int) index = parseTypedExpression<int>();
	
	match(JitTokens::closeBracket);
	match(JitTokens::assign_);

	ScopedTypedNodePointer(float) value = parseTypedExpression<float>();

	auto g = scope->getGlobal(id);

	if (true || !info.useSafeBufferFunctions && g->isConst && JITTypeHelpers::matchesType<Buffer*>(g->getType()))
	{
		ScopedTypedNodePointer(uint64_t) existingDataNode = getBufferDataNode(id);

		if (existingDataNode == nullptr)
		{
			existingDataNode = AsmJitHelpers::getBufferData(*asmCompiler, g);
			existingDataNode->setId(id);
			bufferDataNodes.add(getTypedNode<uint64_t>(existingDataNode->clone()));
		}

		if (info.useSafeBufferFunctions)
		{
			ScopedTypedNodePointer(int) dataSize = AsmJitHelpers::getBufferDataSize(*asmCompiler, g);

			AsmJitHelpers::BufferAssignment(*asmCompiler, existingDataNode, index, value, dataSize, bufferOverflow);
		}
		else
		{
			AsmJitHelpers::BufferAssignment(*asmCompiler, existingDataNode, index, value);
		}
	}

}
#endif

#if 0
template <typename T> BaseNodePtr FunctionParserBase::getGlobalNodeGetFunction(const Identifier &id)
{
	auto r = scope->getGlobal(id);

	if (!HiseJITTypeHelpers::matchesType<T>(r->type))
	{
		location.throwError(HiseJITTypeHelpers::getTypeMismatchErrorMessage<T>(r->type));
	}

    double* d = &r->data;
    
    auto& e1 = exprBase->Immediate<T*>(reinterpret_cast<T*>(d));
    auto& e2 = exprBase->Deref(e1);
    
    return &e2;
}
#endif



BaseNodePtr FunctionParserBase::getOrCreateVariableNode(const BaseScope::Reference& ref)
{
	if (auto a = getVariableNode(ref))
		return a;

	RefCountedBaseNodePtr newNode = AsmJitHelpers::GlobalReference(*asmCompiler, ref);
	
	jassert(newNode != nullptr);

	funkyExistingNodes.add(newNode);
	return funkyExistingNodes.getLast().get();
}


#if 0
BaseNodePtr FunctionParserBase::getGlobalReference(const Identifier& id)
{
	BaseNodePtr a = getGlobalNode(id);

	if (a != nullptr)
	{
		return a->clone();
	};

	auto g = scope->getGlobal(id);

	ScopedBaseNodePointer newNode;

	if (g->isConst)
	{
		if (JITTypeHelpers::matchesType<int>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<int>(g)); }
		else if (JITTypeHelpers::matchesType<float>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<float>(g)); }
		else if (JITTypeHelpers::matchesType<double>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<double>(g)); }
		else if (JITTypeHelpers::matchesType<BooleanType>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<BooleanType>(g)); }

		newNode->setConst();
	}
	else
	{
		if (JITTypeHelpers::matchesType<int>(g->getType())) newNode = AsmJitHelpers::GlobalReference<int>(*asmCompiler, g->getDataPointer());
		else if (JITTypeHelpers::matchesType<float>(g->getType())) newNode = AsmJitHelpers::GlobalReference<float>(*asmCompiler, g->getDataPointer());
		else if (JITTypeHelpers::matchesType<double>(g->getType())) newNode = AsmJitHelpers::GlobalReference<double>(*asmCompiler, g->getDataPointer());
		else if (JITTypeHelpers::matchesType<BooleanType>(g->getType())) newNode = AsmJitHelpers::GlobalReference<BooleanType>(*asmCompiler, g->getDataPointer());
	}

	jassert(newNode != nullptr);

	newNode->setId(id);

	globalNodes.add(newNode.release());
	
	return globalNodes.getLast()->clone();
}
#endif

BaseNodePtr FunctionParserBase::parseLiteral()
{
	TypeInfo t = JITTypeHelpers::getTypeForLiteral(currentString);

	if (JITTypeHelpers::matchesType<int>(t)) return AsmJitHelpers::Immediate(*asmCompiler, (int)currentValue);
	if (JITTypeHelpers::matchesType<float>(t)) return AsmJitHelpers::Immediate(*asmCompiler, (float)currentValue);
	if (JITTypeHelpers::matchesType<double>(t)) return AsmJitHelpers::Immediate(*asmCompiler, (double)currentValue);

	THROW_PARSING_ERROR("Type mismatch at parsing literal: " + JITTypeHelpers::getTypeName(currentString), nullptr);
}

#define PARAMETER_IS_TYPE(type, index) HiseJITTypeHelpers::matchesType<type>(pi.types[index])

#define CHECK_AND_CREATE_FUNCTION_0(returnType) if (HiseJITTypeHelpers::matchesType<returnType>(pi.returnType)) return parseFunctionParameterList<returnType>();

#define CHECK_AND_CREATE_FUNCTION_1(p1Type) if (HiseJITTypeHelpers::matchesType<p1Type>(pi.types[0])) return parseFunctionParameterList<T, p1Type>(b, pi.nodes[0]);


#if 0
BaseNodePtr FunctionParserBase::parseFunctionCall(BaseFunction* b)
{
	match(JitTokens::openParen);

	AsmJitHelpers::ParameterInfo pi;

	while (currentType != JitTokens::closeParen)
	{
		auto e = parseExpression();

		TypeInfo t = getTypeForNode(e);

		pi.data.args.add(JITTypeHelpers::convertToHnodeType(t));
		pi.nodes.add(e);

		matchIf(JitTokens::comma);
	}

	pi.data.returnType = JITTypeHelpers::convertToHnodeType(b->getReturnType());

	return parseFunctionCall(b, pi);
}
#endif

#if 0
hnode::jit::BaseNodePtr FunctionParserBase::parseMemberFunctionCall(JITMemory::ObjectFunction* objFunc, const AsmJitHelpers::ParameterInfo& pi)
{
	match(JitTokens::closeParen);

	switch (objFunc->data.returnType)
	{
	case Types::ID::Float: return AsmJitHelpers::CallWithObject<float>(*asmCompiler, objFunc, pi);
	case Types::ID::Double: return AsmJitHelpers::CallWithObject<double>(*asmCompiler, objFunc, pi);
	case Types::ID::Integer: return AsmJitHelpers::CallWithObject<int>(*asmCompiler, objFunc, pi);
	case Types::ID::Void:	 return AsmJitHelpers::CallWithObject<void>(*asmCompiler, objFunc, pi);
	}

	location.throwError("No valid return type...");
}
#endif



BaseNodePtr FunctionParserBase::parseFunctionCall(const AsmJitHelpers::ParameterInfo& pi)
{
	switch (pi.data.returnType)
	{
	case Types::ID::Float: return AsmJitHelpers::Call<float>(*asmCompiler, pi);
	case Types::ID::Double: return AsmJitHelpers::Call<double>(*asmCompiler, pi);
	case Types::ID::Integer: return AsmJitHelpers::Call<int>(*asmCompiler, pi);
	case Types::ID::Void:	return AsmJitHelpers::Call<void>(*asmCompiler, pi);
	}

	jassertfalse;
	return nullptr;

#if 0
	switch (pi.types.size())
	{
	case 0:
	{
#define MATCH_TYPE_AND_RETURN(type) if (TYPE_MATCH(type, pi.returnType)) return parseFunctionParameterList<type>(b);
		MATCH_TYPE_AND_RETURN(void);
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

	}
	case 1:
	{


#define FUNCTION_CHECK_WITH_1_ARGUMENT(rt, p1Type) if (TYPE_MATCH(rt, pi.returnType) && TYPE_MATCH(p1Type, pi.types[0])) return parseFunctionParameterList<rt, p1Type>(b, pi.nodes[0]);

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(float, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(double, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(int, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(void, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(Buffer*, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN
#endif

#if INCLUDE_CONDITIONALS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(BooleanType, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN
#endif

		location.throwError("No type deducted");
	}
	case 2:
	{

#define FUNCTION_CHECK_WITH_2_ARGUMENTS(rt, p1, p2) if (TYPE_MATCH(rt, pi.returnType) && TYPE_MATCH(p1, pi.types[0]) && TYPE_MATCH(p2, pi.types[1])) return parseFunctionParameterList<rt, p1, p2>(b, pi.nodes[0], pi.nodes[1]);

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, float, type);
		MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
        
#if INCLUDE_CONDITIONALS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
        
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
      
#if INCLUDE_CONDITIONALS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
      
#if INCLUDE_CONDITIONALS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
        

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(void, float, type);
			MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(void, double, type);
			MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(void, int, type);
			MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN

#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(void, Buffer*, type);
			MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif

#if INCLUDE_CONDITIONALS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(void, BooleanType, type);
			MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif


#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(Buffer*, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(Buffer*, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(Buffer*, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(Buffer*, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(Buffer*, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
      
#if INCLUDE_CONDITIONALS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#if INCLUDE_BUFFERS
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
#endif

#undef FUNCTION_CHECK_WITH_2_ARGUMENTS
	}
	default:
	{
		location.throwError("Function parsing error");
	}
	}



	location.throwError("Not yet supported");

	return nullptr;
#endif
}

#if 0
template <typename R> BaseNodePtr FunctionParserBase::parseFunctionParameterList(BaseFunction* b)
{
	typedef R(*f_p)();

	match(JitTokens::closeParen);

	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		return AsmJitHelpers::Call0<R>(*asmCompiler, (void*)function);
	}
	else
	{
		THROW_PARSING_ERROR("Function type mismatch", nullptr);
	}
}


template <typename R, typename ParamType> BaseNodePtr FunctionParserBase::parseFunctionParameterList(FunkyFunctionData* b, BaseNodePtr param1)
{
	match(JitTokens::closeParen);

	checkParameterType<ParamType>(b, 0);

	typedef R(*f_p)(ParamType);
	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto p1 = getTypedNode<ParamType>(param1);

		if (p1 != nullptr)
		{
			return AsmJitHelpers::Call1<R, ParamType>(*asmCompiler, (void*)function, p1);
		}
		else
		{
			THROW_PARSING_ERROR("Parameter 1: Type mismatch. Expected: " + JITTypeHelpers::getTypeName<ParamType>(), nullptr);
		}
	}
	else
	{
		THROW_PARSING_ERROR("Function type mismatch", nullptr);
	}
}





template <typename R, typename ParamType1, typename ParamType2> BaseNodePtr FunctionParserBase::parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1, BaseNodePtr param2)
{
	match(JitTokens::closeParen);

	checkParameterType<ParamType1>(b, 0);
	checkParameterType<ParamType2>(b, 1);

	typedef R(*f_p)(ParamType1, ParamType2);

	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto p1 = getTypedNode<ParamType2>(param1);

		if (p1 == nullptr)
		{
			location.throwError("Parameter 1: Type mismatch. Expected: " + JITTypeHelpers::getTypeName<ParamType1>());
		}
		
		auto p2 = getTypedNode<ParamType2>(param2);

		if(p2 == nullptr)
		{
			location.throwError("Parameter 2: Type mismatch. Expected: " + JITTypeHelpers::getTypeName<ParamType2>());
		}

		return AsmJitHelpers::Call2<R, ParamType1, ParamType2>(*asmCompiler, (void*)function, p1, p2);
	}
	else
	{
		THROW_PARSING_ERROR("Function type mismatch", nullptr);
	}
}

TYPED_VOID FunctionParserBase::checkParameterType(BaseFunction* b, int parameterIndex)
{
	if (b->getTypeForParameter(parameterIndex) != typeid(T))
	{
		location.throwError("Parameter " + String(parameterIndex + 1) + " type mismatch: " + JITTypeHelpers::getTypeName<T>() + ", Expected: " + b->getTypeForParameter(parameterIndex).name());
	}
}
#endif

#if 0
template <typename T> BaseNodePtr FunctionParserBase::getOSXDummyNode(BaseNodePtr node)
{
    auto& dummyReturn = exprBase->Immediate(GlobalBase::returnSameValue<T>);
    auto& exp2 = exprBase->Call(dummyReturn, *dynamic_cast<HiseJIT::Node<T>*>(node));
    return &exp2;
}
#endif


void FunctionParserBase::parseVariableAssignment(VariableStorage& v)
{
	jassertfalse;
}

#if 0
template <typename T>
void FunctionParserBase::parseGlobalAssignment(GlobalBase* g)
{
	if (g->isConst)
	{
		location.throwError("Can't assign to const variable " + g->id.toString());
	}

	parseIdentifier();

	bool isPostDec = matchIf(JitTokens::minusminus);
	bool isPostInc = matchIf(JitTokens::plusplus);

	if (isPostInc || isPostDec)
	{
		if (JITTypeHelpers::is<T, int>())
		{
			ScopedBaseNodePointer existingGlobalReference = getGlobalReference(g->id);

			if (isPostInc) AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(existingGlobalReference));
			else		   AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(existingGlobalReference));
			
			getGlobalNode(g->id)->setIsChangedGlobal();

			match(JitTokens::semicolon);
			return;
		}
		else
		{
			location.throwError("Can't increment / decrement non integer types!");
		}
	}



	TokenType assignType = parseAssignType();

	ScopedBaseNodePointer newNode = parseExpression();	
	ScopedBaseNodePointer existingGlobalReference = getGlobalReference(g->id);

	if (assignType != JitTokens::assign_)
	{
		
		ScopedBaseNodePointer bNode = createBinaryNode(existingGlobalReference, newNode, assignType);
		newNode = bNode.release();
	}

	match(JitTokens::semicolon);

	if(JITTypeHelpers::is<T, float>()) AsmJitHelpers::GlobalAssignment<float>(*asmCompiler, getGlobalNode(g->id), newNode);
	if (JITTypeHelpers::is<T, double>()) AsmJitHelpers::GlobalAssignment<double>(*asmCompiler, getGlobalNode(g->id), newNode);
	if (JITTypeHelpers::is<T, int>()) AsmJitHelpers::GlobalAssignment<int>(*asmCompiler, getGlobalNode(g->id), newNode);
	if (JITTypeHelpers::is<T, BooleanType>()) AsmJitHelpers::GlobalAssignment<BooleanType>(*asmCompiler, getGlobalNode(g->id), newNode);
}
#endif



hnode::jit::BaseNodePtr FunctionParserBase::getVariableNode(const BaseScope::Reference& ref)
{
	for (auto n : funkyExistingNodes)
	{
		if (n->isReferenceToVariable(ref))
			return n;
	}

	return nullptr;
}

BaseNodePtr FunctionParserBase::getGlobalNode(const Identifier& id)
{
	for (int i = 0; i < globalNodes.size(); i++)
	{
		if (globalNodes[i]->getId() == id)
			return globalNodes[i];
	}

	return nullptr;
}


AsmJitHelpers::TypedNode<uint64_t>* FunctionParserBase::getBufferDataNode(const Identifier& id)
{
	for (int i = 0; i < bufferDataNodes.size(); i++)
	{
		if (bufferDataNodes[i]->getId() == id)
		{
			return getTypedNode<uint64_t>(bufferDataNodes[i]->clone());
		}
	}

	return nullptr;
}

BaseNodePtr FunctionParserBase::getLine(const Identifier& id)
{
	for (int i = 0; i < lines.size(); i++)
	{
		if (lines[i]->getId() == id) return lines[i];
	}

	return nullptr;
}


} // end namespace jit
} // end namespace hnode
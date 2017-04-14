/*
  ==============================================================================

    FunctionParserBase.cpp
    Created: 7 Mar 2017 10:44:39pm
    Author:  Christoph

  ==============================================================================
*/

// Handy macro. Use this to #define a if statement that returns a typed node and undef it afterwards...

#if INCLUDE_BUFFERS
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int) MATCH_TYPE_AND_RETURN(Buffer*) MATCH_TYPE_AND_RETURN(BooleanType)
#elif INCLUDE_CONDITIONALS
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int) MATCH_TYPE_AND_RETURN(BooleanType)
#else
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int)
#endif



#if 0
struct FunctionParserBase::MissingOperatorFunctions
{
	template <typename T> static T divideOp(T a, T b)
	{
		return a / b;
	}

	static int moduloOp(int a, int b)
	{
		return a % b;
	}

	using ModuloFunction = int(*)(int, int);

	template <typename T> using DivideFunction = T(*)(T, T);

	template <typename T> using FunctionNode = HiseJIT::Node<T>;

	template <typename T> FunctionNode<DivideFunction<T>>& getDivideFunction(HiseJIT::ExpressionNodeFactory* expr)
	{
		if (HiseJITTypeHelpers::is<T, int>())
		{
			if (divideIntFunction == nullptr)
				divideIntFunction = &expr->Immediate(divideOp<int>);

			return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideIntFunction);
		}
		else if (HiseJITTypeHelpers::is<T, float>())
		{
			if (divideFloatFunction == nullptr)
				divideFloatFunction = &expr->Immediate(divideOp<float>);

			return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideFloatFunction);
		}
		else if (HiseJITTypeHelpers::is<T, double>())
		{
			if (divideDoubleFunction == nullptr)
				divideDoubleFunction = &expr->Immediate(divideOp<double>);

			return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideDoubleFunction);
		}

		return expr->template Immediate<DivideFunction<T>>(0);
	}

	HiseJIT::Node<ModuloFunction>& getModuloFunction(HiseJIT::ExpressionNodeFactory* expr)
	{
		if (moduloIntFunction == nullptr)
		{
			moduloIntFunction = &expr->Immediate(moduloOp);
		}

		return *moduloIntFunction;
	};

private:

	FunctionNode<ModuloFunction>* moduloIntFunction = nullptr;

	FunctionNode<DivideFunction<int>>* divideIntFunction = nullptr;
	FunctionNode<DivideFunction<float>>* divideFloatFunction = nullptr;
	FunctionNode<DivideFunction<double>>* divideDoubleFunction = nullptr;
};
#endif

#if 0
struct FunctionParserBase::NamedNode
{
	NamedNode(const Identifier& id_, const X86Reg& node_, bool isConst_, TypeInfo type_) :
		id(id_),
		node(node_),
		isConst(isConst_),
		type(type_)
	{};

	const Identifier id;
	X86Reg node;
	const bool isConst;
	TypeInfo type;

};
#endif


FunctionParserBase::FunctionParserBase(HiseJITScope::Pimpl* scope_, const FunctionInfo& info_) :
	TokenIterator(info_.code),
	scope(scope_),
	info(info_)
{

	
}

void FunctionParserBase::parseFunctionBody()
{
	lines.clear();

	while (currentType != HiseJitTokens::eof && currentType != HiseJitTokens::closeBrace)
	{

		if (currentType == HiseJitTokens::identifier)
		{
			const Identifier id = Identifier(currentValue);

			if (auto g = scope->getGlobal(id))
			{
				if (HiseJITTypeHelpers::matchesType<float>(g->type)) parseGlobalAssignment<float>(g);
				if (HiseJITTypeHelpers::matchesType<double>(g->type)) parseGlobalAssignment<double>(g);
				if (HiseJITTypeHelpers::matchesType<int>(g->type)) parseGlobalAssignment<int>(g);
				if (HiseJITTypeHelpers::matchesType<BooleanType>(g->type)) parseGlobalAssignment<BooleanType>(g);
#if INCLUDE_BUFFERS
				if (HiseJITTypeHelpers::matchesType<Buffer*>(g->type)) parseBufferLine(id);
#endif
			}
			else
			{
				if (auto l = getLine(id))
				{
					if (HiseJITTypeHelpers::matchesType<float>(l->getType())) parseLineAssignment<float>(l);
					if (HiseJITTypeHelpers::matchesType<double>(l->getType())) parseLineAssignment<double>(l);
					if (HiseJITTypeHelpers::matchesType<int>(l->getType())) parseLineAssignment<int>(l);
					if (HiseJITTypeHelpers::matchesType<BooleanType>(l->getType())) parseLineAssignment<BooleanType>(l);
				}
				else
				{
					parseUntypedLine();
				}
			}
		}
		else if (matchIf(HiseJitTokens::const_))
		{
			if (matchIf(HiseJitTokens::float_))		parseLine<float>(true);
			else if (matchIf(HiseJitTokens::int_))	parseLine<int>(true);
			else if (matchIf(HiseJitTokens::double_))	parseLine<double>(true);
			else if (matchIf(HiseJitTokens::bool_))	parseLine<BooleanType>(true);

		}
		else if (matchIf(HiseJitTokens::float_))		parseLine<float>(false);
		else if (matchIf(HiseJitTokens::int_))	parseLine<int>(false);
		else if (matchIf(HiseJitTokens::double_))	parseLine<double>(false);
		else if (matchIf(HiseJitTokens::bool_))	parseLine<BooleanType>(false);
		else if (matchIf(HiseJitTokens::return_)) parseReturn();
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

	match(HiseJitTokens::assign_);

	ScopedTypedNodePointer(T) r = parseTypedExpression<T>();
	
	r->setId(id);

	if (isConst)
		r->setConst();
	
	lines.add(r.release());

	match(HiseJitTokens::semicolon);
}


TYPED_VOID FunctionParserBase::parseLineAssignment(AsmJitHelpers::BaseNode* l)
{
	if (l->isConst())
	{
		location.throwError("Can't assign to const variable " + l->getId());
	}

	Identifier id = parseIdentifier();

	bool isPostDec = matchIf(HiseJitTokens::minusminus);
	bool isPostInc = matchIf(HiseJitTokens::plusplus);

	if (isPostInc || isPostDec)
	{
		if (HiseJITTypeHelpers::is<T, int>())
		{
			if (isPostInc) AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(l));
			else		   AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(l));

			match(HiseJitTokens::semicolon);
			return;
		}
		else
		{
			location.throwError("Can't increment / decrement non integer types!");
		}
	}

	TokenType assignType = parseAssignType();
	ScopedTypedNodePointer(T) n = parseTypedExpression<T>();

	if (assignType != HiseJitTokens::assign_)
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

	match(HiseJitTokens::semicolon);
}

void FunctionParserBase::parseUntypedLine()
{
	ScopedPointer<AsmJitHelpers::BaseNode> expr = parseExpression();
	match(HiseJitTokens::semicolon);
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

	if (currentType == HiseJitTokens::plus)
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

	if (currentType == HiseJitTokens::minus)
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

	if (currentType == HiseJitTokens::times ||
		currentType == HiseJitTokens::divide ||
		currentType == HiseJitTokens::modulo)
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

	if (matchIf(HiseJitTokens::question))
	{
		auto conditionTyped = getTypedNode<BooleanType>(condition);

		if (conditionTyped != nullptr)
		{
			if (conditionTyped->isImmediateValue())
			{
				const bool isTrue = conditionTyped->getImmediateValue<BooleanType>() > 0;

				if (isTrue)
				{
					ScopedBaseNodePointer trueBranch = parseExpression();
					match(HiseJitTokens::colon);

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
					match(HiseJitTokens::colon);

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
			
			if (HiseJITTypeHelpers::matchesType<int>(left->getType()))				AsmJitHelpers::BinaryOpInstructions::Int::store(*asmCompiler, intResult, left);
			else if (HiseJITTypeHelpers::matchesType<float>(left->getType()))		AsmJitHelpers::BinaryOpInstructions::Float::store(*asmCompiler, floatResult, left);
			else if (HiseJITTypeHelpers::matchesType<double>(left->getType()))		AsmJitHelpers::BinaryOpInstructions::Double::store(*asmCompiler, doubleResult, left);
			else if (HiseJITTypeHelpers::matchesType<BooleanType>(left->getType())) AsmJitHelpers::BinaryOpInstructions::Bool::store(*asmCompiler, boolResult, left);

			ASSERT_ASM_OK;

			error = asmCompiler->jmp(e);
			ASSERT_ASM_OK;

			match(HiseJitTokens::colon);

			error = asmCompiler->bind(r);
			ASSERT_ASM_OK;

			ScopedBaseNodePointer right = parseExpression();
			
			if (HiseJITTypeHelpers::matchesType<int>(right->getType()))				AsmJitHelpers::BinaryOpInstructions::Int::store(*asmCompiler, intResult, right);
			else if (HiseJITTypeHelpers::matchesType<float>(right->getType()))		AsmJitHelpers::BinaryOpInstructions::Float::store(*asmCompiler, floatResult, right);
			else if (HiseJITTypeHelpers::matchesType<double>(right->getType()))		AsmJitHelpers::BinaryOpInstructions::Double::store(*asmCompiler, doubleResult, right);
			else if (HiseJITTypeHelpers::matchesType<BooleanType>(right->getType())) AsmJitHelpers::BinaryOpInstructions::Bool::store(*asmCompiler, boolResult, right);
			
			error = asmCompiler->bind(e);
			ASSERT_ASM_OK;

			TypeInfo tl = getTypeForNode(left);
			TypeInfo tr = getTypeForNode(right);

			if (tr == tl)
			{
				if (HiseJITTypeHelpers::matchesType<int>(tr)) return new AsmJitHelpers::TypedNode<int>(intResult);
				if (HiseJITTypeHelpers::matchesType<double>(tr)) return new AsmJitHelpers::TypedNode<double>(doubleResult);
				if (HiseJITTypeHelpers::matchesType<float>(tr)) return new AsmJitHelpers::TypedNode<float>(floatResult);
				if (HiseJITTypeHelpers::matchesType<BooleanType>(tr)) return new AsmJitHelpers::TypedNode<BooleanType>(boolResult);
			}
			else
			{
				THROW_PARSING_ERROR(HiseJITTypeHelpers::getTypeMismatchErrorMessage(tl, tr), nullptr);
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
	match(HiseJitTokens::closeParen);

	ScopedBaseNodePointer source = parseTerm();

	TypeInfo sourceType = getTypeForNode(source);

	if (HiseJITTypeHelpers::matchesType<int>(sourceType))
		return AsmJitHelpers::EmitCast<int, ExpectedType>(*asmCompiler, getTypedNode<int>(source.get()));

    if (HiseJITTypeHelpers::matchesType<double>(sourceType))
        return AsmJitHelpers::EmitCast<double, ExpectedType>(*asmCompiler, getTypedNode<double>(source.get()));

    if (HiseJITTypeHelpers::matchesType<float>(sourceType))
        return AsmJitHelpers::EmitCast<float, ExpectedType>(*asmCompiler, getTypedNode<float>(source.get()));

	THROW_PARSING_ERROR("Unsupported type for cast", nullptr);	
}




BaseNodePtr FunctionParserBase::parseTerm()
{
	if (matchIf(HiseJitTokens::openParen))
	{
		if (matchIf(HiseJitTokens::float_))  return parseCast<float>();
		if (matchIf(HiseJitTokens::int_))    return parseCast<int>();
		if (matchIf(HiseJitTokens::double_)) return parseCast<double>();
		else
		{
			auto result = parseExpression();
            match(HiseJitTokens::closeParen);
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
    if (currentType == HiseJitTokens::identifier ||
        currentType == HiseJitTokens::literal ||
		currentType == HiseJitTokens::minus ||
		currentType == HiseJitTokens::plusplus ||
		currentType == HiseJitTokens::minusminus)
    {
        return parseFactor();
    }
#if INCLUDE_CONDITIONALS
	else if (matchIf(HiseJitTokens::true_))
	{
		return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 1);
	}
	else if (matchIf(HiseJitTokens::false_))
	{
		return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, 0);
	}
	else if (currentType == HiseJitTokens::logicalNot)
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
	const bool isMinus = matchIf(HiseJitTokens::minus);
	
	auto expr = parseSymbolOrLiteral();

	if(isMinus)
	{
		TypeInfo t = getTypeForNode(expr);

#define MATCH_TYPE_AND_RETURN(type) if(HiseJITTypeHelpers::matchesType<type>(t)) return AsmJitHelpers::Negate<type>(*asmCompiler, expr);

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

	if (matchIf(HiseJitTokens::literal))
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
	bool preInc = matchIf(HiseJitTokens::plusplus);
	bool preDec = matchIf(HiseJitTokens::minusminus);

	Identifier symbolId = parseIdentifier();

	bool postInc = matchIf(HiseJitTokens::plusplus);
	bool postDec = matchIf(HiseJitTokens::minusminus);


	if (info.parameterNames.indexOf(symbolId) != -1)		return parseParameterReference(symbolId);
	if (auto r = getLine(symbolId))
	{
		ScopedBaseNodePointer line = getNodeForLine(r);

		if (preInc || postInc)      AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(line));
		else if (preDec || postDec) AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(line));
		
		return line.release();
	}
	
	else if (auto g = scope->getGlobal(symbolId))
	{
		if (HiseJITTypeHelpers::matchesType<Buffer*>(g->getType()))
			return parseBufferOperation(symbolId);

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
	else if (auto b = scope->getExposedFunction(symbolId))	return parseFunctionCall(b);
	else if (auto cf = scope->getCompiledBaseFunction(symbolId))
	{
#define MATCH_TYPE_AND_RETURN(type) if(HiseJITTypeHelpers::matchesType<type>(cf->getReturnType())) return parseFunctionCall(cf);

		MATCH_TYPE_AND_RETURN(int);
		MATCH_TYPE_AND_RETURN(double);
		MATCH_TYPE_AND_RETURN(float);
		MATCH_TYPE_AND_RETURN(void);
		MATCH_TYPE_AND_RETURN(BooleanType);
		
#undef MATCH_TYPE_AND_RETURN

	}
	else
	{
		location.throwError("Unknown identifier " + symbolId.toString());
	}

	return nullptr;
}

BaseNodePtr FunctionParserBase::parseParameterReference(const Identifier &id)
{
	BaseNodePtr existingParameterNode = getParameterNode(id);

	if (existingParameterNode != nullptr)
	{
		return existingParameterNode->clone();
	}

	BaseNodePtr newParameterNode = parseParameterReferenceTyped(id);

	parameterNodes.add(newParameterNode);

	return newParameterNode->clone();
	
	
}


BaseNodePtr FunctionParserBase::parseBufferOperation(const Identifier &id)
{
	if (matchIf(HiseJitTokens::openBracket))
	{
		return parseBufferAccess(id);
	}
	else
	{
		match(HiseJitTokens::dot);

		return parseBufferFunction<float>(id);
	}
}

BaseNodePtr FunctionParserBase::createBinaryNode(BaseNodePtr a, BaseNodePtr b, TokenType op)
{
	TypeInfo ta = getTypeForNode(a);
	TypeInfo tb = getTypeForNode(b);

	if (ta != tb)
	{
		THROW_PARSING_ERROR("Type mismatch for binary operation " + String(op), nullptr);
	}
	
	if (HiseJITTypeHelpers::matchesType<int>(ta)) return createTypedBinaryNode<int>(getTypedNode<int>(a), getTypedNode<int>(b), op);
	else if (HiseJITTypeHelpers::matchesType<double>(ta)) return createTypedBinaryNode<double>(getTypedNode<double>(a), getTypedNode<double>(b), op);
	else if (HiseJITTypeHelpers::matchesType<float>(ta)) return createTypedBinaryNode<float>(getTypedNode<float>(a), getTypedNode<float>(b), op);
	else if (HiseJITTypeHelpers::matchesType<BooleanType>(ta)) return createTypedBinaryNode<BooleanType>(getTypedNode<BooleanType>(a), getTypedNode<BooleanType>(b), op);
	else
	{
		THROW_PARSING_ERROR("Unsupported type for binary operation", nullptr);
	}
}



template <typename T> BaseNodePtr FunctionParserBase::createTypedBinaryNode(TypedNodePtr left, TypedNodePtr right, TokenType op)
{
	if (left->isImmediateValue() && right->isImmediateValue())
	{
		if (op == HiseJitTokens::plus)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, left->getImmediateValue<T>() + right->getImmediateValue<T>());
		}
		if (op == HiseJitTokens::minus)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, left->getImmediateValue<T>() - right->getImmediateValue<T>());
		}
		if (op == HiseJitTokens::times)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, left->getImmediateValue<T>() * right->getImmediateValue<T>());
		}
		if (op == HiseJitTokens::divide)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, left->getImmediateValue<T>() / right->getImmediateValue<T>());
		}
		if (op == HiseJitTokens::modulo)
		{
			return AsmJitHelpers::Immediate<T>(*asmCompiler, (int)left->getImmediateValue<T>() % (int)right->getImmediateValue<T>());
		}
		else if (op == HiseJitTokens::greaterThan)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, left->getImmediateValue<T>() > right->getImmediateValue<T>() ? 1 : 0);
		}
		else if (op == HiseJitTokens::greaterThanOrEqual)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, left->getImmediateValue<T>() >= right->getImmediateValue<T>() ? 1 : 0);
		}
		else if (op == HiseJitTokens::lessThan)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, left->getImmediateValue<T>() < right->getImmediateValue<T>() ? 1 : 0);
		}
		else if (op == HiseJitTokens::lessThanOrEqual)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, left->getImmediateValue<T>() <= right->getImmediateValue<T>() ? 1 : 0);
		}
		else if (op == HiseJitTokens::equals)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, left->getImmediateValue<T>() == right->getImmediateValue<T>() ? 1 : 0);
		}
		else if (op == HiseJitTokens::notEquals)
		{
			return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, left->getImmediateValue<T>() != right->getImmediateValue<T>() ? 1 : 0);
		}
		else if (op == HiseJitTokens::logicalAnd)
		{
			if (HiseJITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, (left->getImmediateValue<T>() != 0) && (right->getImmediateValue<T>()) != 0 ? 1 : 0);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}
		else if (op == HiseJitTokens::logicalOr)
		{
			if (HiseJITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::Immediate<BooleanType>(*asmCompiler, (left->getImmediateValue<T>() != 0) || (right->getImmediateValue<T>()) || 0 ? 1 : 0);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}

	}
	else
	{
		if (op == HiseJitTokens::plus)
		{
			return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Add>(*asmCompiler, left, right);
		}
		if (op == HiseJitTokens::minus)
		{
			return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Sub>(*asmCompiler, left, right);
		}
		if (op == HiseJitTokens::times)
		{
			return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Mul>(*asmCompiler, left, right);
		}
		if (op == HiseJitTokens::divide)
		{
			return AsmJitHelpers::EmitBinaryOp < AsmJitHelpers::Div>(*asmCompiler, left, right);
		}
		if (op == HiseJitTokens::modulo)
		{
			return AsmJitHelpers::EmitBinaryOp < AsmJitHelpers::Mod>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::greaterThan)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::GT>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::greaterThanOrEqual)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::GTE>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::lessThan)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::LT>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::lessThanOrEqual)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::LTE>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::equals)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::EQ>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::notEquals)
		{
			return AsmJitHelpers::Compare<T, AsmJitHelpers::Opcodes::NEQ>(*asmCompiler, left, right);
		}
		else if (op == HiseJitTokens::logicalAnd)
		{
			if (HiseJITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Opcodes::And>(*asmCompiler, left, right);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}
		else if (op == HiseJitTokens::logicalOr)
		{
			if (HiseJITTypeHelpers::is<T, BooleanType>())
				return AsmJitHelpers::EmitBinaryOp<AsmJitHelpers::Opcodes::Or>(*asmCompiler, left, right);

			else
				location.throwError("Can't use logic operators on non boolean expressions");
		}

	}

	
	return nullptr;
}

TYPED_NODE FunctionParserBase::getTypedNode(BaseNodePtr node)
{
	auto r = dynamic_cast<TypedNodePtr>(node);

	if (r != nullptr)
	{
		return r;
	}
	
	THROW_PARSING_ERROR("Critical error in internal cast", nullptr);
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
#define MATCH_TYPE_AND_RETURN(type) if (auto r = dynamic_cast<AsmJitHelpers::TypedNode<type>*>(node)) return typeid(type);

	MATCH_AND_RETURN_ALL_TYPES()

#undef MATCH_TYPE_AND_RETURN

	THROW_PARSING_ERROR("Unknown type", typeid(void));
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
	{
		return typed.release();
	}
	
	location.throwError("Expression type mismatch: Expected " + HiseJITTypeHelpers::getTypeName<T>());
	return nullptr;
}

BaseNodePtr FunctionParserBase::parseBool()
{
#if INCLUDE_CONDITIONALS
	const bool isInverted = matchIf(HiseJitTokens::logicalNot);

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

	if (matchIf(HiseJitTokens::logicalAnd))
	{
		ScopedBaseNodePointer right = parseLogicOperation();
		return createBinaryNode(left, right, HiseJitTokens::logicalAnd);
	}
	else if (matchIf(HiseJitTokens::logicalOr))
	{
		ScopedBaseNodePointer right = parseLogicOperation();
		return createBinaryNode(left, right, HiseJitTokens::logicalOr);
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

	if (currentType == HiseJitTokens::greaterThan ||
		currentType == HiseJitTokens::greaterThanOrEqual ||
		currentType == HiseJitTokens::lessThan ||
		currentType == HiseJitTokens::lessThanOrEqual ||
		currentType == HiseJitTokens::equals ||
		currentType == HiseJitTokens::notEquals)
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
	match(HiseJitTokens::closeBracket);

	auto g = scope->getGlobal(id);

	if (HiseJITTypeHelpers::matchesType<Buffer*>(g->getType())) // g->isConst && 
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


#if 0
	if (info.useSafeBufferFunctions)
	{
		auto& r = exprBase->Immediate(BufferOperations::getSample);
		return exprBase->Call(r, b, index);
	}
	else
	{
		auto& r = exprBase->Immediate(BufferOperations::getSampleRaw);
		return exprBase->Call(r, b, index);
	}
#endif

	return nullptr;
}

template <typename T> BaseNodePtr FunctionParserBase::parseBufferFunction(const Identifier& id)
{
	const Identifier functionName = parseIdentifier();

	static const Identifier setSize("setSize");

	if (functionName == setSize)
	{
		auto b = getBufferNode(id);

#if 0
		auto& f = exprBase->Immediate(BufferOperations::setSize<T>);

		match(HiseJitTokens::openParen);
		auto& s = parseTypedExpression<int>();
		match(HiseJitTokens::closeParen);

		return &exprBase->Call(f, b, s);
#endif
	}

	return nullptr;
}

void FunctionParserBase::parseBufferAssignment(const Identifier &id)
{
	ScopedTypedNodePointer(int) index = parseTypedExpression<int>();
	
	match(HiseJitTokens::closeBracket);
	match(HiseJitTokens::assign_);

	ScopedTypedNodePointer(float) value = parseTypedExpression<float>();

	auto g = scope->getGlobal(id);

	if (true || !info.useSafeBufferFunctions && g->isConst && HiseJITTypeHelpers::matchesType<Buffer*>(g->getType()))
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


#if 0
		auto& b1 = exprBase->Immediate<float*>(bufferData);
		auto& b2 = exprBase->Add(b1, index);
		
		auto& f = exprBase->Immediate(GlobalBase::storeData<T, float>);

		return &exprBase->Call(f, b2, value);
#endif
	}

#if 0
	auto& b = getBufferNode(id);

	auto& f = exprBase->Immediate(BufferOperations::setSample<T>);
	return &exprBase->Call(f, b, index, value);
#endif
}


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
		if (HiseJITTypeHelpers::matchesType<int>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<int>(g)); }
		else if (HiseJITTypeHelpers::matchesType<float>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<float>(g)); }
		else if (HiseJITTypeHelpers::matchesType<double>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<double>(g)); }
		else if (HiseJITTypeHelpers::matchesType<BooleanType>(g->getType())) { newNode = AsmJitHelpers::Immediate(*asmCompiler, GlobalBase::get<BooleanType>(g)); }

		newNode->setConst();
	}
	else
	{
		if (HiseJITTypeHelpers::matchesType<int>(g->getType())) newNode = AsmJitHelpers::GlobalReference<int>(*asmCompiler, &g->data);
		else if (HiseJITTypeHelpers::matchesType<float>(g->getType())) newNode = AsmJitHelpers::GlobalReference<float>(*asmCompiler, &g->data);
		else if (HiseJITTypeHelpers::matchesType<double>(g->getType())) newNode = AsmJitHelpers::GlobalReference<double>(*asmCompiler, &g->data);
		else if (HiseJITTypeHelpers::matchesType<BooleanType>(g->getType())) newNode = AsmJitHelpers::GlobalReference<BooleanType>(*asmCompiler, &g->data);
	}

	jassert(newNode != nullptr);

	newNode->setId(id);

	globalNodes.add(newNode.release());
	
	return globalNodes.getLast()->clone();
}


BaseNodePtr FunctionParserBase::parseLiteral()
{
	TypeInfo t = HiseJITTypeHelpers::getTypeForLiteral(currentString);

	if (HiseJITTypeHelpers::matchesType<int>(t)) return AsmJitHelpers::Immediate(*asmCompiler, (int)currentValue);
	if (HiseJITTypeHelpers::matchesType<float>(t)) return AsmJitHelpers::Immediate(*asmCompiler, (float)currentValue);
	if (HiseJITTypeHelpers::matchesType<double>(t)) return AsmJitHelpers::Immediate(*asmCompiler, (double)currentValue);

	THROW_PARSING_ERROR("Type mismatch at parsing literal: " + HiseJITTypeHelpers::getTypeName(currentString), nullptr);
}

#define PARAMETER_IS_TYPE(type, index) HiseJITTypeHelpers::matchesType<type>(pi.types[index])

#define CHECK_AND_CREATE_FUNCTION_0(returnType) if (HiseJITTypeHelpers::matchesType<returnType>(pi.returnType)) return parseFunctionParameterList<returnType>();

#define CHECK_AND_CREATE_FUNCTION_1(p1Type) if (HiseJITTypeHelpers::matchesType<p1Type>(pi.types[0])) return parseFunctionParameterList<T, p1Type>(b, pi.nodes[0]);




BaseNodePtr FunctionParserBase::parseFunctionCall(BaseFunction* b)
{

	jassert(b != nullptr);

	match(HiseJitTokens::openParen);

	ParameterInfo pi(b->getReturnType());

	while (currentType != HiseJitTokens::closeParen)
	{
		auto e = parseExpression();

		TypeInfo t = getTypeForNode(e);

		pi.types.push_back(t);
		pi.nodes.add(e);
		
		matchIf(HiseJitTokens::comma);
	}

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
}

template <typename R> BaseNodePtr FunctionParserBase::parseFunctionParameterList(BaseFunction* b)
{
	typedef R(*f_p)();

	match(HiseJitTokens::closeParen);

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


template <typename R, typename ParamType> BaseNodePtr FunctionParserBase::parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1)
{
	match(HiseJitTokens::closeParen);

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
			THROW_PARSING_ERROR("Parameter 1: Type mismatch. Expected: " + HiseJITTypeHelpers::getTypeName<ParamType>(), nullptr);
		}
	}
	else
	{
		THROW_PARSING_ERROR("Function type mismatch", nullptr);
	}
}





template <typename R, typename ParamType1, typename ParamType2> BaseNodePtr FunctionParserBase::parseFunctionParameterList(BaseFunction* b, BaseNodePtr param1, BaseNodePtr param2)
{
	match(HiseJitTokens::closeParen);

	checkParameterType<ParamType1>(b, 0);
	checkParameterType<ParamType2>(b, 1);

	typedef R(*f_p)(ParamType1, ParamType2);

	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto p1 = getTypedNode<ParamType2>(param1);

		if (p1 == nullptr)
		{
			location.throwError("Parameter 1: Type mismatch. Expected: " + HiseJITTypeHelpers::getTypeName<ParamType1>());
		}
		
		auto p2 = getTypedNode<ParamType2>(param2);

		if(p2 == nullptr)
		{
			location.throwError("Parameter 2: Type mismatch. Expected: " + HiseJITTypeHelpers::getTypeName<ParamType2>());
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
		location.throwError("Parameter " + String(parameterIndex + 1) + " type mismatch: " + HiseJITTypeHelpers::getTypeName<T>() + ", Expected: " + b->getTypeForParameter(parameterIndex).name());
	}
}

#if 0
template <typename T> BaseNodePtr FunctionParserBase::getOSXDummyNode(BaseNodePtr node)
{
    auto& dummyReturn = exprBase->Immediate(GlobalBase::returnSameValue<T>);
    auto& exp2 = exprBase->Call(dummyReturn, *dynamic_cast<HiseJIT::Node<T>*>(node));
    return &exp2;
}
#endif

template <typename T>
void FunctionParserBase::parseGlobalAssignment(GlobalBase* g)
{
	if (g->isConst)
	{
		location.throwError("Can't assign to const variable " + g->id.toString());
	}

	parseIdentifier();

	bool isPostDec = matchIf(HiseJitTokens::minusminus);
	bool isPostInc = matchIf(HiseJitTokens::plusplus);

	if (isPostInc || isPostDec)
	{
		if (HiseJITTypeHelpers::is<T, int>())
		{
			ScopedBaseNodePointer existingGlobalReference = getGlobalReference(g->id);

			if (isPostInc) AsmJitHelpers::Inc(*asmCompiler, getTypedNode<int>(existingGlobalReference));
			else		   AsmJitHelpers::Dec(*asmCompiler, getTypedNode<int>(existingGlobalReference));
			
			getGlobalNode(g->id)->setIsChangedGlobal();

			match(HiseJitTokens::semicolon);
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

	if (assignType != HiseJitTokens::assign_)
	{
		
		ScopedBaseNodePointer bNode = createBinaryNode(existingGlobalReference, newNode, assignType);
		newNode = bNode.release();
	}

	match(HiseJitTokens::semicolon);

	if(HiseJITTypeHelpers::is<T, float>()) AsmJitHelpers::GlobalAssignment<float>(*asmCompiler, getGlobalNode(g->id), newNode);
	if (HiseJITTypeHelpers::is<T, double>()) AsmJitHelpers::GlobalAssignment<double>(*asmCompiler, getGlobalNode(g->id), newNode);
	if (HiseJITTypeHelpers::is<T, int>()) AsmJitHelpers::GlobalAssignment<int>(*asmCompiler, getGlobalNode(g->id), newNode);
	if (HiseJITTypeHelpers::is<T, BooleanType>()) AsmJitHelpers::GlobalAssignment<BooleanType>(*asmCompiler, getGlobalNode(g->id), newNode);
}

FunctionParserBase::TokenType FunctionParserBase::parseAssignType()
{
	TokenType assignType;

	if (matchIf(HiseJitTokens::minusEquals)) assignType = HiseJitTokens::minus;
	else if (matchIf(HiseJitTokens::divideEquals)) assignType = HiseJitTokens::divide;
	else if (matchIf(HiseJitTokens::timesEquals)) assignType = HiseJitTokens::times;
	else if (matchIf(HiseJitTokens::plusEquals)) assignType = HiseJitTokens::plus;
	else if (matchIf(HiseJitTokens::moduloEquals)) assignType = HiseJitTokens::modulo;
	else
	{
		match(HiseJitTokens::assign_);
		assignType = HiseJitTokens::assign_;
	}

	return assignType;
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


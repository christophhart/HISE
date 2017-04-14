/*
  ==============================================================================

    FunctionParserBase.cpp
    Created: 7 Mar 2017 10:44:39pm
    Author:  Christoph

  ==============================================================================
*/

// Handy macro. Use this to #define a if statement that returns a typed node and undef it afterwards...
#define MATCH_AND_RETURN_ALL_TYPES() MATCH_TYPE_AND_RETURN(float) MATCH_TYPE_AND_RETURN(double) MATCH_TYPE_AND_RETURN(int) MATCH_TYPE_AND_RETURN(Buffer*) MATCH_TYPE_AND_RETURN(BooleanType)

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

	template <typename T> using FunctionNode = NativeJIT::Node<T>;

	template <typename T> FunctionNode<DivideFunction<T>>& getDivideFunction(NativeJIT::ExpressionNodeFactory* expr)
	{
		if (NativeJITTypeHelpers::is<T, int>())
		{
			if (divideIntFunction == nullptr)
				divideIntFunction = &expr->Immediate(divideOp<int>);

			return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideIntFunction);
		}
		else if (NativeJITTypeHelpers::is<T, float>())
		{
			if (divideFloatFunction == nullptr)
				divideFloatFunction = &expr->Immediate(divideOp<float>);

			return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideFloatFunction);
		}
		else if (NativeJITTypeHelpers::is<T, double>())
		{
			if (divideDoubleFunction == nullptr)
				divideDoubleFunction = &expr->Immediate(divideOp<double>);

			return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideDoubleFunction);
		}

		return expr->template Immediate<DivideFunction<T>>(0);
	}

	NativeJIT::Node<ModuloFunction>& getModuloFunction(NativeJIT::ExpressionNodeFactory* expr)
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

struct FunctionParserBase::NamedNode
{
	NamedNode(const Identifier& id_, NativeJIT::NodeBase* node_, bool isConst_, TypeInfo type_) :
		id(id_),
		node(node_),
		isConst(isConst_),
		type(type_)
	{};

	const Identifier id;
	NativeJIT::NodeBase* node;
	const bool isConst;
	TypeInfo type;

};



FunctionParserBase::FunctionParserBase(NativeJITScope::Pimpl* scope_, const FunctionInfo& info_) :
	TokenIterator(info_.code),
	scope(scope_),
	info(info_),
	missingOperatorFunctions(new MissingOperatorFunctions())
{

	
}

void FunctionParserBase::parseFunctionBody()
{
	lines.clear();

	while (currentType != NativeJitTokens::eof && currentType != NativeJitTokens::closeBrace)
	{

		if (currentType == NativeJitTokens::identifier)
		{
			const Identifier id = Identifier(currentValue);

			auto g = scope->getGlobal(id);

			if (auto g = scope->getGlobal(id))
			{
				if (NativeJITTypeHelpers::matchesType<float>(g->type)) parseGlobalAssignment<float>(g);
				if (NativeJITTypeHelpers::matchesType<double>(g->type)) parseGlobalAssignment<double>(g);
				if (NativeJITTypeHelpers::matchesType<int>(g->type)) parseGlobalAssignment<int>(g);
				if (NativeJITTypeHelpers::matchesType<BooleanType>(g->type)) parseGlobalAssignment<BooleanType>(g);
				if (NativeJITTypeHelpers::matchesType<Buffer*>(g->type)) parseBufferLine(id);
			}
			else
			{
				if (auto l = getLine(id))
				{
					if (NativeJITTypeHelpers::matchesType<float>(l->type)) parseLineAssignment<float>(l);
					if (NativeJITTypeHelpers::matchesType<double>(l->type)) parseLineAssignment<double>(l);
					if (NativeJITTypeHelpers::matchesType<int>(l->type)) parseLineAssignment<int>(l);
					if (NativeJITTypeHelpers::matchesType<BooleanType>(l->type)) parseLineAssignment<BooleanType>(l);
				}
				else
				{
					parseUntypedLine();
				}
			}
		}
		else if (matchIf(NativeJitTokens::const_))
		{
			if (matchIf(NativeJitTokens::float_))		parseLine<float>(true);
			else if (matchIf(NativeJitTokens::int_))	parseLine<int>(true);
			else if (matchIf(NativeJitTokens::double_))	parseLine<double>(true);
			else if (matchIf(NativeJitTokens::bool_))	parseLine<BooleanType>(true);

		}
		else if (matchIf(NativeJitTokens::float_))		parseLine<float>(false);
		else if (matchIf(NativeJitTokens::int_))	parseLine<int>(false);
		else if (matchIf(NativeJitTokens::double_))	parseLine<double>(false);
		else if (matchIf(NativeJitTokens::bool_))	parseLine<BooleanType>(false);
		else if (matchIf(NativeJitTokens::return_)) parseReturn();
		else
		{
			parseUntypedLine();
		}

		//else match(NativeJitTokens::eof);

	}
}

int FunctionParserBase::getParameterIndex(const Identifier& id)
{
	return info.parameterNames.indexOf(id);
}

template <typename T>
void FunctionParserBase::parseLine(bool isConst)
{
	Identifier id = parseIdentifier();

	

	auto existingLine = getLine(id);

	if (existingLine != nullptr)
	{
		location.throwError("Identifier " + id.toString() + " already defined");
	}

	match(NativeJitTokens::assign_);
	auto& r = parseTypedExpression<T>();
	

	lines.add(new NamedNode(id, &r, isConst, typeid(T)));

	match(NativeJitTokens::semicolon);
}


TYPED_NODE_VOID FunctionParserBase::parseLineAssignment(NamedNode* l)
{
	if (l->isConst)
	{
		location.throwError("Can't assign to const variable " + l->id);
	}

	Identifier id = parseIdentifier();
	TokenType assignType = parseAssignType();
	auto& n = parseTypedExpression<T>();

	if (assignType != NativeJitTokens::assign_)
	{
		auto n2 = createBinaryNode(l->node, &n, assignType);
		auto& n3 = getTypedNode<T>(n2);
		l->node = &n3;
	}
	else
	{
		l->node = &n;
	}

	match(NativeJitTokens::semicolon);
}

void FunctionParserBase::parseUntypedLine()
{
	anonymousLines.add(parseExpression());
	match(NativeJitTokens::semicolon);
}

BASE_NODE FunctionParserBase::parseExpression()
{
	auto r = parseTernaryOperator();

	return r;
}

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

BASE_NODE FunctionParserBase::parseSum()
{
	auto left = parseProduct();

	if (currentType == NativeJitTokens::plus || currentType == NativeJitTokens::minus)
	{
		TokenType op = currentType;
		skip();

		auto right = parseSum();

		return createBinaryNode(left, right, op);
		
	}
	else
	{
		return left;
	}
}

BASE_NODE FunctionParserBase::parseProduct()
{
	auto left = parseTerm();

	if (currentType == NativeJitTokens::times ||
		currentType == NativeJitTokens::divide ||
		currentType == NativeJitTokens::modulo)
	{
		TokenType op = currentType;
		skip();

		auto right = parseProduct();

		return createBinaryNode(left, right, op);
	}
	else
	{
		return left;
	}
}


BASE_NODE FunctionParserBase::parseTernaryOperator()
{
	auto condition = parseBool();

	if (matchIf(NativeJitTokens::question))
	{
		jassert(yes != nullptr);

		auto conditionTyped = dynamic_cast<NativeJIT::Node<BooleanType>*>(condition);

		if (conditionTyped != nullptr)
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JE>(*conditionTyped, *yes);

			auto left = parseExpression();

			match(NativeJitTokens::colon);

			auto right = parseExpression();

			TypeInfo tl = getTypeForNode(left);
			TypeInfo tr = getTypeForNode(right);

			if (tr == tl)
			{
#define MATCH_TYPE_AND_RETURN(type) if(NativeJITTypeHelpers::matchesType<type>(tl)) return &exprBase->Conditional(flag, getTypedNode<type>(left), getTypedNode<type>(right));

				MATCH_AND_RETURN_ALL_TYPES()

#undef MATCH_TYPE_AND_RETURN
			}
			else
			{
				location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage(tl, tr));
			}
		}
		else
		{
			location.throwError("Condition must be a bool");
		}

	}
	else
	{
		return condition;

	}
}



template <typename ExpectedType> NativeJIT::NodeBase* FunctionParserBase::parseCast()
{
	match(NativeJitTokens::closeParen);

	auto source = parseTerm();

	TypeInfo sourceType = getTypeForNode(source);

#define MATCH_TYPE_AND_RETURN(type) if (NativeJITTypeHelpers::matchesType<type>(sourceType)) return &exprBase->Cast<ExpectedType, type>(getTypedNode<type>(source));
	
	MATCH_AND_RETURN_ALL_TYPES();
	
#undef MATCH_TYPE_AND_RETURN
	
	location.throwError("Unsupported type for cast");
	
}




BASE_NODE FunctionParserBase::parseTerm()
{
	if (matchIf(NativeJitTokens::openParen))
	{
		if (matchIf(NativeJitTokens::float_))  return parseCast<float>();
		if (matchIf(NativeJitTokens::int_))    return parseCast<int>();
		if (matchIf(NativeJitTokens::double_)) return parseCast<double>();
		else
		{
			auto result = parseExpression();
            match(NativeJitTokens::closeParen);
            return result;
		}
	}
    else
    {
        return parseUnary();
    }

	return nullptr;
}

BASE_NODE FunctionParserBase::parseUnary()
{
    if (currentType == NativeJitTokens::identifier ||
        currentType == NativeJitTokens::literal ||
		currentType == NativeJitTokens::minus)
    {
        return parseFactor();
    }
	else if (matchIf(NativeJitTokens::true_))
	{
		return yes;
	}
	else if (matchIf(NativeJitTokens::false_))
	{
		return no;
	}
	else
    {
        location.throwError("Parsing error");
    }
    
	return nullptr;
}

BASE_NODE FunctionParserBase::parseFactor()
{
	
	const bool isMinus = matchIf(NativeJitTokens::minus);
	
	auto expr = parseSymbolOrLiteral();

	if(isMinus)
	{
		TypeInfo type = getTypeForNode(expr);

		if (NativeJITTypeHelpers::matchesType<int>(type))
		{
			auto& minus1 = exprBase->Immediate(static_cast<int>(-1.0));
			auto& negate = exprBase->Mul(minus1, getTypedNode<int>(expr));
			return &negate;
		}
		if (NativeJITTypeHelpers::matchesType<double>(type))
		{
			auto& minus1 = exprBase->Immediate(static_cast<double>(-1.0));
			auto& negate = exprBase->Mul(minus1, getTypedNode<double>(expr));
			return &negate;
		}
		if (NativeJITTypeHelpers::matchesType<float>(type))
		{
			auto& minus1 = exprBase->Immediate(static_cast<float>(-1.0));
			auto& negate = exprBase->Mul(minus1, getTypedNode<float>(expr));
			return &negate;

		}
		else
		{
			location.throwError("Can't multiply a buffer or bool with -1");
		}
	}
	else
	{
		return expr;
	}

}

BASE_NODE FunctionParserBase::parseSymbolOrLiteral()
{
	if (matchIf(NativeJitTokens::identifier))	return parseSymbol();
	else
	{
		match(NativeJitTokens::literal);
		return parseLiteral();
	}
}

BASE_NODE FunctionParserBase::getNodeForLine(NamedNode* r)
{
	return r->node;
}

BASE_NODE FunctionParserBase::parseSymbol()
{
	Identifier symbolId = Identifier(currentValue);

	if (info.parameterNames.indexOf(symbolId) != -1)		return parseParameterReference(symbolId);
	if (auto r = getLine(symbolId))							return getNodeForLine(r);
	else if (auto gn = getGlobalNode(symbolId))				return gn->getLastNodeUntyped();
	else if (auto g = scope->getGlobal(symbolId))
	{
		if (NativeJITTypeHelpers::matchesType<Buffer*>(g->getType()))
		{
			return parseBufferOperation(symbolId);
		}
		else
		{
			return getGlobalReference(symbolId);
		}	
	}
	else if (auto b = scope->getExposedFunction(symbolId))	return parseFunctionCall(b);
	else if (auto cf = scope->getCompiledBaseFunction(symbolId))
	{
#define MATCH_TYPE_AND_RETURN(type) if(NativeJITTypeHelpers::matchesType<type>(cf->getReturnType())) return parseFunctionCall(cf);

		MATCH_TYPE_AND_RETURN(int);
		MATCH_TYPE_AND_RETURN(double);
		MATCH_TYPE_AND_RETURN(float);
		MATCH_TYPE_AND_RETURN(BooleanType);
		
#undef MATCH_TYPE_AND_RETURN

	}
	else
	{
		location.throwError("Unknown identifier " + symbolId.toString());
	}

	return nullptr;
}

BASE_NODE FunctionParserBase::parseParameterReference(const Identifier &id)
{
	return parseParameterReferenceTyped(id);
}

BASE_NODE FunctionParserBase::parseBufferOperation(const Identifier &id)
{
	if (matchIf(NativeJitTokens::openBracket))
	{
		return &parseBufferAccess(id);
	}
	else
	{
		match(NativeJitTokens::dot);

		return parseBufferFunction<float>(id);
	}
}


NativeJIT::NodeBase* FunctionParserBase::createBinaryNode(NativeJIT::NodeBase* a, NativeJIT::NodeBase* b, TokenType op)
{
	TypeInfo ta = getTypeForNode(a);
	TypeInfo tb = getTypeForNode(b);

	if (ta != tb)
	{
		location.throwError("Type mismatch for binary operation " + String(op));
	}
	
#define MATCH_TYPE_AND_RETURN(type) if (NativeJITTypeHelpers::matchesType<type>(ta)) return createTypedBinaryNode(getTypedNode<type>(a), getTypedNode<type>(b), op);

	MATCH_AND_RETURN_ALL_TYPES()

#undef MATCH_TYPE_AND_RETURN
}



template <typename T> BASE_NODE FunctionParserBase::createTypedBinaryNode(NativeJIT::Node<T>& left, NativeJIT::Node<T>& right, TokenType op)
{
	if (op == NativeJitTokens::plus)
	{
		return &exprBase->Add(left, right);
	}
	if (op == NativeJitTokens::minus)
	{
		return &exprBase->Sub(left, right);
	}
	if (op == NativeJitTokens::times)
	{
		return &exprBase->Mul(left, right);
	}
	if (op == NativeJitTokens::divide)
	{
		auto& div = missingOperatorFunctions->template getDivideFunction<T>(exprBase);

		auto& left1 = exprBase->Mul(left, exprBase->Immediate<T>((T)1));

		return &exprBase->Call(div, left1, right);
	}
	if (op == NativeJitTokens::modulo)
	{
		if (NativeJITTypeHelpers::is<T, int>())
		{
			auto& mod = missingOperatorFunctions->getModuloFunction(exprBase);

			auto castedLeft = dynamic_cast<NativeJIT::Node<int>*>(&left);
			auto castedRight = dynamic_cast<NativeJIT::Node<int>*>(&right);

			return &exprBase->Call(mod, *castedLeft, *castedRight);
		}
		else
		{
			throw String("Modulo operation on " + NativeJITTypeHelpers::getTypeName<T>() + " type");
		}

		return &getEmptyNode<T>();
	}
	else if (op == NativeJitTokens::greaterThan)
	{
		const bool isFPType = NativeJITTypeHelpers::is<T, float>() || NativeJITTypeHelpers::is<T, double>();

		if (isFPType)
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JA>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
		else
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JG>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
	}
	else if (op == NativeJitTokens::greaterThanOrEqual)
	{
		const bool isFPType = NativeJITTypeHelpers::is<T, float>() || NativeJITTypeHelpers::is<T, double>();

		if (isFPType)
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JAE>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
		else
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JGE>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
	}
	else if (op == NativeJitTokens::lessThan)
	{
		const bool isFPType = NativeJITTypeHelpers::is<T, float>() || NativeJITTypeHelpers::is<T, double>();

		if (isFPType)
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JB>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
		else
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JL>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
	}
	else if (op == NativeJitTokens::lessThanOrEqual)
	{
		const bool isFPType = NativeJITTypeHelpers::is<T, float>() || NativeJITTypeHelpers::is<T, double>();

		if (isFPType)
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JBE>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
		else
		{
			auto& flag = exprBase->Compare<NativeJIT::JccType::JLE>(left, right);
			return &exprBase->Conditional(flag, *yes, *no);
		}
	}
	else if (op == NativeJitTokens::equals)
	{
		auto& flag = exprBase->Compare<NativeJIT::JccType::JE>(left, right);
		return &exprBase->Conditional(flag, *yes, *no);
		
	}
	else if (op == NativeJitTokens::notEquals)
	{
		auto& flag = exprBase->Compare<NativeJIT::JccType::JNE>(left, right);
		return &exprBase->Conditional(flag, *yes, *no);
	}
	else if (op == NativeJitTokens::logicalAnd)
	{
		auto leftAsBool = dynamic_cast<NativeJIT::Node<BooleanType>*>(&left);
		auto rightAsBool = dynamic_cast<NativeJIT::Node<BooleanType>*>(&right);

		if (leftAsBool != nullptr && rightAsBool != nullptr)
		{
			auto& sum = exprBase->Add(*leftAsBool, *rightAsBool);

			if (and_ == nullptr)
			{
				and_ = &exprBase->Immediate<BooleanType>((BooleanType)2);
			}

			auto& flag = exprBase->Compare<NativeJIT::JccType::JE>(sum, *and_);

			jassert(yes != nullptr);
			jassert(no != nullptr);

			return &exprBase->Conditional(flag, *yes, *no);
		}
		else
		{
			location.throwError("Can't use logic operators on non boolean expressions");
		}
	}
	else if (op == NativeJitTokens::logicalOr)
	{
		auto leftAsBool = dynamic_cast<NativeJIT::Node<BooleanType>*>(&left);
		auto rightAsBool = dynamic_cast<NativeJIT::Node<BooleanType>*>(&right);

		if (leftAsBool != nullptr && rightAsBool != nullptr)
		{
			auto& sum = exprBase->Add(*leftAsBool, *rightAsBool);

			jassert(yes != nullptr);
			jassert(no != nullptr);

			auto& flag = exprBase->Compare<NativeJIT::JccType::JE>(sum, *yes);

			return &exprBase->Conditional(flag, *yes, *no);
		}
		else
		{
			location.throwError("Can't use logic operators on non boolean expressions");
		}
	}
}

TYPED_NODE FunctionParserBase::getTypedNode(NativeJIT::NodeBase* node)
{
	auto r = dynamic_cast<NativeJIT::Node<T>*>(node);

	if (r != nullptr)
	{
		return *r;
	}
	else
	{
		location.throwError("Critical error in internal cast");
	}

	return getEmptyNode<T>();
}



TYPED_NODE FunctionParserBase::getCastedNode(NativeJIT::NodeBase* node)
{
	TypeInfo nodeType = getTypeForNode(node);

	if (TYPE_MATCH(Buffer*, nodeType)) 
	{
		return getEmptyNode<T>();
	}

	// Skip cast if not required
	if (TYPE_MATCH(T, nodeType)) return getTypedNode<T>(node);


#define MATCH_TYPE_AND_RETURN(type) if (TYPE_MATCH(type, nodeType)) return exprBase->Cast<T, type>(getTypedNode<type>(node));

	MATCH_TYPE_AND_RETURN(int);
	MATCH_TYPE_AND_RETURN(float);
	MATCH_TYPE_AND_RETURN(double);
	MATCH_TYPE_AND_RETURN(BooleanType);

#undef MATCH_TYPE_AND_RETURN
	

	location.throwError("No cast possible");
	return getEmptyNode<T>();
}

TypeInfo FunctionParserBase::getTypeForNode(NativeJIT::NodeBase* node)
{
#define MATCH_TYPE_AND_RETURN(type) if (auto r = dynamic_cast<NativeJIT::Node<type>*>(node)) return typeid(type);

	MATCH_AND_RETURN_ALL_TYPES()

#undef MATCH_TYPE_AND_RETURN

	else location.throwError("Unknown type");

	return typeid(int);
}

bool FunctionParserBase::nodesHaveSameType(NativeJIT::NodeBase* a, NativeJIT::NodeBase* b)
{
	return getTypeForNode(a) == getTypeForNode(b);
}

TYPED_NODE FunctionParserBase::parseTypedExpression()
{
	auto e = parseExpression();

	auto typed = dynamic_cast<NativeJIT::Node<T>*>(e);

	if (typed != nullptr)
	{
		return *typed;
	}
	else
	{
		location.throwError("Expression type mismatch: Expected " + NativeJITTypeHelpers::getTypeName<T>());
	}

	return getEmptyNode<T>();
}

BASE_NODE FunctionParserBase::parseBool()
{
	const bool isInverted = matchIf(NativeJitTokens::logicalNot);

	if (yes == nullptr) // Create them the first time they are used
	{
		yes = &exprBase->Immediate<BooleanType>((BooleanType)1);
		no = &exprBase->Immediate<BooleanType>((BooleanType)0);
	}

	auto result = parseLogicOperation();

	if (!isInverted)
	{
		return result;
	}
	else
	{
		auto resultTyped = dynamic_cast<NativeJIT::Node<BooleanType>*>(result);

		if (resultTyped != nullptr)
		{
			return &exprBase->Sub(*yes, *resultTyped);
		}
		else
		{
			location.throwError("Can't negate non boolean expressions");
		}
	}
}


BASE_NODE FunctionParserBase::parseLogicOperation()
{
	auto left = parseComparation();

	if (matchIf(NativeJitTokens::logicalAnd))
	{
		auto right = parseLogicOperation();
		return createBinaryNode(left, right, NativeJitTokens::logicalAnd);
	}
	else if (matchIf(NativeJitTokens::logicalOr))
	{
		auto right = parseLogicOperation();
		return createBinaryNode(left, right, NativeJitTokens::logicalOr);
	}
	else
	{
		return left;
	}
}


BASE_NODE FunctionParserBase::parseComparation()
{
	auto left = parseSum();

	if (currentType == NativeJitTokens::greaterThan ||
		currentType == NativeJitTokens::greaterThanOrEqual ||
		currentType == NativeJitTokens::lessThan ||
		currentType == NativeJitTokens::lessThanOrEqual ||
		currentType == NativeJitTokens::equals ||
		currentType == NativeJitTokens::notEquals)
	{
		TokenType op = currentType;
		skip();
		auto right = parseSum();
		return createBinaryNode(left, right, op);

	}
	else
	{
		return left;
	}

	return nullptr;
}

NativeJIT::Node<Buffer*>& FunctionParserBase::getBufferNode(const Identifier& id)
{
	auto r = scope->getGlobal(id);

	auto& e1 = exprBase->Immediate(GlobalBase::getBuffer);
	auto& g = exprBase->Immediate<GlobalBase*>(r);

	return exprBase->Call(e1, g);
}

NativeJIT::Node<float>& FunctionParserBase::parseBufferAccess(const Identifier &id)
{


	auto& index = parseTypedExpression<int>();
	match(NativeJitTokens::closeBracket);

	auto g = scope->getGlobal(id);

	if (g->isConst && NativeJITTypeHelpers::matchesType<Buffer*>(g->getType()))
	{
		float* bufferData = GlobalBase::getBuffer(g)->b->buffer.getWritePointer(0);

		auto& b1 = exprBase->Immediate<float*>(bufferData);
		auto& b2 = exprBase->Add(b1, index);
		return exprBase->Deref(b2);
	}


	auto& b = getBufferNode(id);



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
}

template <typename T> BASE_NODE FunctionParserBase::parseBufferFunction(const Identifier& id)
{
	const Identifier functionName = parseIdentifier();

	static const Identifier setSize("setSize");

	if (functionName == setSize)
	{
		auto& b = getBufferNode(id);
		auto& f = exprBase->Immediate(BufferOperations::setSize<T>);

		match(NativeJitTokens::openParen);
		auto& s = parseTypedExpression<int>();
		match(NativeJitTokens::closeParen);

		return &exprBase->Call(f, b, s);
	}

	return &getEmptyNode<T>();
}

template <typename T> BASE_NODE FunctionParserBase::parseBufferAssignment(const Identifier &id)
{
	

	auto& index = parseTypedExpression<int>();
	match(NativeJitTokens::closeBracket);

	match(NativeJitTokens::assign_);

#if JUCE_WINDOWS
	auto& value = parseTypedExpression<float>();
#else
    auto& unwrapped = parseTypedExpression<float>();
    auto& dummyF = exprBase->Immediate(GlobalBase::returnSameValue<float>);
    auto& value = exprBase->Call(dummyF, unwrapped);
#endif

	auto g = scope->getGlobal(id);

	if (!info.useSafeBufferFunctions && g->isConst && NativeJITTypeHelpers::matchesType<Buffer*>(g->getType()))
	{
		float* bufferData = GlobalBase::getBuffer(g)->b->buffer.getWritePointer(0);

		auto& b1 = exprBase->Immediate<float*>(bufferData);
		auto& b2 = exprBase->Add(b1, index);
		
		auto& f = exprBase->Immediate(GlobalBase::storeData<T, float>);

		return &exprBase->Call(f, b2, value);
	}

	auto& b = getBufferNode(id);

	auto& f = exprBase->Immediate(BufferOperations::setSample<T>);
	return &exprBase->Call(f, b, index, value);
}


template <typename T> BASE_NODE FunctionParserBase::getGlobalNodeGetFunction(const Identifier &id)
{
	auto r = scope->getGlobal(id);

	if (!NativeJITTypeHelpers::matchesType<T>(r->type))
	{
		location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<T>(r->type));
	}

    double* d = &r->data;
    
    auto& e1 = exprBase->Immediate<T*>(reinterpret_cast<T*>(d));
    auto& e2 = exprBase->Deref(e1);
    
    return &e2;
}

BASE_NODE FunctionParserBase::getGlobalReference(const Identifier& id)
{
	auto g = scope->getGlobal(id);

	if (g->isConst)
	{
#define MATCH_TYPE_AND_RETURN(type) if(NativeJITTypeHelpers::matchesType<type>(g->getType())) return &exprBase->Immediate<type>(GlobalBase::get<type>(g));

		MATCH_TYPE_AND_RETURN(double);
		MATCH_TYPE_AND_RETURN(float);
		MATCH_TYPE_AND_RETURN(int);
		MATCH_TYPE_AND_RETURN(BooleanType);

#undef MATCH_TYPE_AND_RETURN
	}

	auto existingNode = getGlobalNode(id);

	const bool globalWasChanged = existingNode != nullptr;

	if (globalWasChanged)
	{
		return existingNode->getLastNodeUntyped();
	}
	else
	{
		
#define MATCH_TYPE_AND_RETURN(type) if (NativeJITTypeHelpers::matchesType<type>(g->getType())) return getGlobalNodeGetFunction<type>(id);

		MATCH_AND_RETURN_ALL_TYPES()

#undef MATCH_TYPE_AND_RETURN
	}
}

BASE_NODE FunctionParserBase::parseLiteral()
{
	TypeInfo t = NativeJITTypeHelpers::getTypeForLiteral(currentString);

#define MATCH_TYPE_AND_RETURN(type) if (NativeJITTypeHelpers::matchesType<type>(t)) return &exprBase->Immediate((type)currentValue);

	MATCH_TYPE_AND_RETURN(int);
	MATCH_TYPE_AND_RETURN(double);
	MATCH_TYPE_AND_RETURN(float);
	
#undef MATCH_TYPE_AND_RETURN

	location.throwError("Type mismatch at parsing literal: " + NativeJITTypeHelpers::getTypeName(currentString));
}

#define PARAMETER_IS_TYPE(type, index) NativeJITTypeHelpers::matchesType<type>(pi.types[index])

#define CHECK_AND_CREATE_FUNCTION_0(returnType) if (NativeJITTypeHelpers::matchesType<returnType>(pi.returnType)) return parseFunctionParameterList<returnType>();

#define CHECK_AND_CREATE_FUNCTION_1(p1Type) if (NativeJITTypeHelpers::matchesType<p1Type>(pi.types[0])) return parseFunctionParameterList<T, p1Type>(b, pi.nodes[0]);




BASE_NODE FunctionParserBase::parseFunctionCall(BaseFunction* b)
{
	jassert(b != nullptr);

	match(NativeJitTokens::openParen);

	ParameterInfo pi(b->getReturnType());

	while (currentType != NativeJitTokens::closeParen)
	{
		auto e = parseExpression();

		TypeInfo t = getTypeForNode(e);

		pi.types.push_back(t);
		pi.nodes.add(e);
		
		matchIf(NativeJitTokens::comma);
	}

	switch (pi.types.size())
	{
	case 0:
	{
#define MATCH_TYPE_AND_RETURN(type) if (TYPE_MATCH(type, pi.returnType)) return parseFunctionParameterList<type>(b);
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN;

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

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(Buffer*, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_1_ARGUMENT(BooleanType, type)
		MATCH_AND_RETURN_ALL_TYPES();
#undef MATCH_TYPE_AND_RETURN

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
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(float, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(double, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(int, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
        
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
        
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, float, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, double, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, int, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, Buffer*, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#define MATCH_TYPE_AND_RETURN(type) FUNCTION_CHECK_WITH_2_ARGUMENTS(BooleanType, BooleanType, type);
        MATCH_AND_RETURN_ALL_TYPES()
#undef	MATCH_TYPE_AND_RETURN
        
#undef FUNCTION_CHECK_WITH_2_ARGUMENTS
	}
	default:
	{
		location.throwError("Function parsing error");
	}
	}

	return nullptr;
}

template <typename R> BASE_NODE FunctionParserBase::parseFunctionParameterList(BaseFunction* b)
{
	typedef R(*f_p)();

	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto& e2 = exprBase->Immediate(function);
		auto& e3 = exprBase->Call(e2);
		match(NativeJitTokens::closeParen);
		return &e3;
	}
	else
	{
		location.throwError("Function type mismatch");
	}

	nullptr;
}


template <typename R, typename ParamType> NativeJIT::NodeBase* FunctionParserBase::parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1)
{
	checkParameterType<ParamType>(b, 0);

	typedef R(*f_p)(ParamType);
	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto& e1 = exprBase->Immediate(function);

		auto e2 = dynamic_cast<NativeJIT::Node<ParamType>*>(param1);

		if (e2 != nullptr)
		{
			auto& e3 = exprBase->template Call<R, ParamType>(e1, *e2);
			match(NativeJitTokens::closeParen);
			return &e3;
		}
		else
		{
			location.throwError("Parameter 1: Type mismatch. Expected: " + NativeJITTypeHelpers::getTypeName<ParamType>());
		}
	}
	else
	{
		location.throwError("Function type mismatch");
	}


	return &getEmptyNode<R>();
}





template <typename R, typename ParamType1, typename ParamType2> NativeJIT::NodeBase* FunctionParserBase::parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1, NativeJIT::NodeBase* param2)
{
	checkParameterType<ParamType1>(b, 0);
	checkParameterType<ParamType2>(b, 1);

	typedef R(*f_p)(ParamType1, ParamType2);

	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto& e1 = exprBase->Immediate(function);

		auto e2 = dynamic_cast<NativeJIT::Node<ParamType1>*>(param1);

		if (e2 == nullptr)
		{
			location.throwError("Parameter 1: Type mismatch. Expected: " + NativeJITTypeHelpers::getTypeName<ParamType1>());
		}

		auto e3 = dynamic_cast<NativeJIT::Node<ParamType2>*>(param2);

		if (e3 != nullptr)
		{
			auto& e4 = exprBase->Call(e1, *e2, *e3);
			match(NativeJitTokens::closeParen);
			return &e4;
		}
		else
		{
			location.throwError("Parameter 2: Type mismatch. Expected: " + NativeJITTypeHelpers::getTypeName<ParamType2>());
		}
	}
	else
	{
		location.throwError("Function type mismatch");
	}


	return &getEmptyNode<R>();
}

TYPED_NODE_VOID FunctionParserBase::checkParameterType(BaseFunction* b, int parameterIndex)
{
	if (b->getTypeForParameter(parameterIndex) != typeid(T))
	{
		location.throwError("Parameter " + String(parameterIndex + 1) + " type mismatch: " + NativeJITTypeHelpers::getTypeName<T>() + ", Expected: " + b->getTypeForParameter(parameterIndex).name());
	}
}

template <typename T> BASE_NODE FunctionParserBase::getOSXDummyNode(NativeJIT::NodeBase* node)
{
    auto& dummyReturn = exprBase->Immediate(GlobalBase::returnSameValue<T>);
    auto& exp2 = exprBase->Call(dummyReturn, *dynamic_cast<NativeJIT::Node<T>*>(node));
    return &exp2;
}

template <typename T>
void FunctionParserBase::parseGlobalAssignment(GlobalBase* g)
{
	if (g->isConst)
	{
		location.throwError("Can't assign to const variable " + g->id.toString());
	}

	parseIdentifier();

	TokenType assignType = parseAssignType();

	NativeJIT::NodeBase* newNode = parseExpression();	
    TypeInfo newType = getTypeForNode(newNode);
	NativeJIT::NodeBase* old = nullptr;

	auto existingNode = getGlobalNode(g->id);

	if (existingNode == nullptr) old = getGlobalReference(g->id);
	else						 old = existingNode->getLastNodeUntyped();

	if (assignType != NativeJitTokens::assign_)
	{
		newNode = createBinaryNode(old, newNode, assignType);
	}

#if JUCE_MAC
    
    // OSX returns wrong values for double / floats if the expression is not wrapped into a dummy function call
    
#define GET_DUMMY_NODE(type) if (TYPE_MATCH(type, newType)) newNode = getOSXDummyNode<type>(newNode);
    
    GET_DUMMY_NODE(double);
    GET_DUMMY_NODE(float);
    GET_DUMMY_NODE(int);
    GET_DUMMY_NODE(BooleanType);
    GET_DUMMY_NODE(Buffer*);
    
#undef GET_DUMMY_NODE
    
    
    
#endif
    
	match(NativeJitTokens::semicolon);

	
	OptionalScopedPointer<GlobalNode> pointerToUse;
	
	if (existingNode != nullptr) pointerToUse.setNonOwned(existingNode);
	else pointerToUse.setOwned(new GlobalNode(g));

	

#define ADD_IF_MATCH(type) if (TYPE_MATCH(type, newType)) pointerToUse->addNode<type>(exprBase, newNode)

	ADD_IF_MATCH(double);
	ADD_IF_MATCH(float);
	ADD_IF_MATCH(int);
	ADD_IF_MATCH(Buffer*);
	ADD_IF_MATCH(BooleanType);

#undef ADD_IF_MATCH

	if (existingNode == nullptr)
	{
		globalNodes.add(pointerToUse.release());
	}
}


FunctionParserBase::TokenType FunctionParserBase::parseAssignType()
{
	TokenType assignType;

	if (matchIf(NativeJitTokens::minusEquals)) assignType = NativeJitTokens::minus;
	else if (matchIf(NativeJitTokens::divideEquals)) assignType = NativeJitTokens::divide;
	else if (matchIf(NativeJitTokens::timesEquals)) assignType = NativeJitTokens::times;
	else if (matchIf(NativeJitTokens::plusEquals)) assignType = NativeJitTokens::plus;
	else if (matchIf(NativeJitTokens::moduloEquals)) assignType = NativeJitTokens::modulo;
	else
	{
		match(NativeJitTokens::assign_);
		assignType = NativeJitTokens::assign_;
	}

	return assignType;
}

GlobalNode* FunctionParserBase::getGlobalNode(const Identifier& id)
{
	for (int i = 0; i < globalNodes.size(); i++)
	{
		if (globalNodes[i]->matchesName(id))
			return globalNodes[i];
	}

	return nullptr;


}


TYPED_NODE FunctionParserBase::getAnonymousLine(int index)
{
	NativeJIT::NodeBase* untyped = anonymousLines[index];

	if (untyped == nullptr)
	{
		location.throwError("Anonymous line not found");
		return getEmptyNode<T>();
	}

	TypeInfo type = getTypeForNode(untyped);

	if (TYPE_MATCH(T, type))
	{
		auto& l = getTypedNode<T>(untyped);
		auto& r = exprBase->Immediate<T>((T)0);
		return exprBase->Add(l, r);
	}
	else
	{
		auto& castedL = getCastedNode<T>(untyped);
		auto& r = exprBase->Immediate<T>((T)0);
		return exprBase->Add(castedL, r);
	}
}

FunctionParserBase::NamedNode* FunctionParserBase::getLine(const Identifier& id)
{
	for (int i = 0; i < lines.size(); i++)
	{
		if (lines[i]->id == id) return lines[i];
	}

	return nullptr;
}

TypeInfo FunctionParserBase::peekFirstType()
{
	const String trimmedCode(location.location);

	TokenIterator peeker(location.location);

	while (peeker.currentType == NativeJitTokens::openParen ||
           peeker.currentType == NativeJitTokens::minus)
		peeker.skip();

	if (peeker.currentType == NativeJitTokens::identifier)
	{
		const Identifier currentId(currentValue);

		int pIndex = info.parameterNames.indexOf(currentId);

		if (pIndex != -1)
		{
			Identifier typeId = info.parameterTypes[pIndex];



			if (typeId.toString() == NativeJitTokens::float_) return typeid(float);
			if (typeId.toString() == NativeJitTokens::double_) return typeid(double);
			if (typeId.toString() == NativeJitTokens::int_) return typeid(int);
			if (typeId.toString() == NativeJitTokens::bool_) return typeid(BooleanType);
		}
		if (auto r = getLine(currentId))
		{
			if (dynamic_cast<NativeJIT::Node<float>*>(r->node)) return typeid(float);
			if (dynamic_cast<NativeJIT::Node<double>*>(r->node)) return typeid(double);
			if (dynamic_cast<NativeJIT::Node<int>*>(r->node)) return typeid(int);
			if (dynamic_cast<NativeJIT::Node<BooleanType>*>(r->node)) return typeid(BooleanType);
		}

		if (auto f = scope->getExposedFunction(currentId))
		{
			return f->getReturnType();
		}

		if (auto f = scope->getCompiledBaseFunction(currentId))
		{
			return f->getReturnType();
		}

		if (auto g = scope->getGlobal(currentId))
		{
			if (NativeJITTypeHelpers::matchesType<Buffer*>(g->getType()))
			{
				return typeid(float);
			}
			else return g->getType();
		}
	}

	if (peeker.currentType == NativeJitTokens::double_) return typeid(double);
	if (peeker.currentType == NativeJitTokens::int_) return typeid(int);
	if (peeker.currentType == NativeJitTokens::float_) return typeid(float);
	if (peeker.currentType == NativeJitTokens::true_) return typeid(BooleanType);
	if (peeker.currentType == NativeJitTokens::false_) return typeid(BooleanType);

	return NativeJITTypeHelpers::getTypeForLiteral(peeker.currentString);
}

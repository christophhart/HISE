/*
  ==============================================================================

    FunctionParserBase.cpp
    Created: 7 Mar 2017 10:44:39pm
    Author:  Christoph

  ==============================================================================
*/



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
			const Identifier id = parseIdentifier();

			auto g = scope->getGlobal(id);

			if (g != nullptr)
			{
				if (NativeJITTypeHelpers::matchesType<float>(g->type)) parseGlobalAssignment<float>(g);
				if (NativeJITTypeHelpers::matchesType<double>(g->type)) parseGlobalAssignment<double>(g);
				if (NativeJITTypeHelpers::matchesType<int>(g->type)) parseGlobalAssignment<int>(g);
				if (NativeJITTypeHelpers::matchesType<Buffer*>(g->type)) parseBufferLine(id);
			}
			else
			{
				location.throwError("Global variable not found");
			}
		}
		else if (matchIf(NativeJitTokens::const_))
		{
			if (matchIf(NativeJitTokens::float_))		parseLine<float>(true);
			else if (matchIf(NativeJitTokens::int_))	parseLine<int>(true);
			else if (matchIf(NativeJitTokens::double_))	parseLine<double>(true);

		}
		else if (matchIf(NativeJitTokens::float_))		parseLine<float>(false);
		else if (matchIf(NativeJitTokens::int_))	parseLine<int>(false);
		else if (matchIf(NativeJitTokens::double_))	parseLine<double>(false);
		else if (matchIf(NativeJitTokens::return_)) parseReturn();

		else match(NativeJitTokens::eof);

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
	auto& r = parseExpression<T>();
	match(NativeJitTokens::semicolon);

	auto lastLine = getLine(lastParsedLine);

	if (isConst)
	{
		if (lastLine != nullptr)
		{
			auto& wrapped = exprBase->Dependent(r, *lastLine->node);
			lines.add(new NamedNode(id, &wrapped, isConst, typeid(T)));
		}
		else
		{
			lines.add(new NamedNode(id, &r, isConst, typeid(T)));
		}
	}

	lastParsedLine = id;
}


template <typename LineType> NativeJIT::Node<LineType>& FunctionParserBase::parseExpression()
{
	return parseSum<LineType>();
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

template <typename T> NativeJIT::Node<T>& FunctionParserBase::parseSum()
{
	auto& left = parseProduct<T>();

	if (matchIf(NativeJitTokens::plus))
	{
		auto& right = parseSum<T>();
		return exprBase->Add(left, right);
	}
	else if (matchIf(NativeJitTokens::minus))
	{
		auto& right = parseSum<T>();
		return exprBase->Sub(left, right);
	}
	else
	{
		return left;
	}
}

template <typename T> NativeJIT::Node<T>& FunctionParserBase::parseProduct()
{
	auto& left = parseCondition<T>();

	if (matchIf(NativeJitTokens::times))
	{
		auto& right = parseProduct<T>();
		return exprBase->Mul(left, right);
	}
	else if (matchIf(NativeJitTokens::divide))
	{

		auto& right = parseProduct<T>();
		auto& f1 = missingOperatorFunctions->template getDivideFunction<T>(exprBase);

		return exprBase->Call(f1, left, right);
	}
	else if (matchIf(NativeJitTokens::modulo))
	{
		if (NativeJITTypeHelpers::is<T, int>())
		{
			auto& right = parseProduct<T>();

			auto& f1 = missingOperatorFunctions->getModuloFunction(exprBase);

			auto castedLeft = dynamic_cast<NativeJIT::Node<int>*>(&left);
			auto castedRight = dynamic_cast<NativeJIT::Node<int>*>(&right);

			auto& result = exprBase->Call(f1, *castedLeft, *castedRight);

			return *dynamic_cast<NativeJIT::Node<T>*>(&result);
		}
		else
		{
			throw String("Modulo operation on " + NativeJITTypeHelpers::getTypeName<T>() + " type");
		}
	}
	else
	{
		return left;
	}
}

template <typename T> NativeJIT::Node<T>& FunctionParserBase::parseCondition()
{
	if (NativeJITTypeHelpers::matchesType<float>(peekFirstType()))		 return parseTernaryOperator<T, float>();
	else if (NativeJITTypeHelpers::matchesType<double>(peekFirstType())) return parseTernaryOperator<T, double>();
	else if (NativeJITTypeHelpers::matchesType<int>(peekFirstType()))	 return parseTernaryOperator<T, int>();
	else location.throwError("Parsing error");

	return getEmptyNode<T>();
}

template <typename T, typename ConditionType, NativeJIT::JccType compareFlag> NativeJIT::Node<T>& FunctionParserBase::parseBranches(NativeJIT::Node<ConditionType>& left)
{
	auto& right = parseTerm<ConditionType>();
	auto& a = exprBase->template Compare<compareFlag>(left, right);

	match(NativeJitTokens::question);

	auto& true_b = parseTerm<T>();

	match(NativeJitTokens::colon);

	auto& false_b = parseTerm<T>();

	return exprBase->Conditional(a, true_b, false_b);
}

template <typename T, typename ConditionType> NativeJIT::Node<T>& FunctionParserBase::parseTernaryOperator()
{
	auto& left = parseTerm<ConditionType>();

	if (matchIf(NativeJitTokens::greaterThan))
	{
		return parseBranches<T, ConditionType, NativeJIT::JccType::JG>(left);
	}
	else if (matchIf(NativeJitTokens::greaterThanOrEqual))
	{
		return parseBranches<T, ConditionType, NativeJIT::JccType::JGE>(left);
	}
	else if (matchIf(NativeJitTokens::lessThan))
	{
		return parseBranches<T, ConditionType, NativeJIT::JccType::JL>(left);
	}
	else if (matchIf(NativeJitTokens::lessThanOrEqual))
	{
		return parseBranches<T, ConditionType, NativeJIT::JccType::JLE>(left);
	}
	else if (matchIf(NativeJitTokens::equals))
	{
		return parseBranches<T, ConditionType, NativeJIT::JccType::JE>(left);
	}
	else if (matchIf(NativeJitTokens::notEquals))
	{
		return parseBranches<T, ConditionType, NativeJIT::JccType::JNE>(left);
	}
	else
	{
		if (NativeJITTypeHelpers::is<T, ConditionType>())
		{
			return *dynamic_cast<NativeJIT::Node<T>*>(&left);
		}
		else
		{
			location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<T, ConditionType>());
		}
	}

	return getEmptyNode<T>();
}


template <typename TargetType, typename ExpectedType> NativeJIT::Node<TargetType>& FunctionParserBase::parseCast()
{
	if (!NativeJITTypeHelpers::is<TargetType, ExpectedType>())
	{
		location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType, TargetType>());
	}

	match(NativeJitTokens::closeParen);

	auto sourceType = peekFirstType();

	if (NativeJITTypeHelpers::matchesType<float>(sourceType))
	{
		auto &e1 = parseTerm<float>();
		return exprBase->template Cast<TargetType>(e1);
	}
	else if (NativeJITTypeHelpers::matchesType<double>(sourceType))
	{
		auto &e1 = parseTerm<double>();
		return exprBase->template Cast<TargetType>(e1);
	}
	else if (NativeJITTypeHelpers::matchesType<int>(sourceType))
	{
		auto &e1 = parseTerm<int>();
		return exprBase->template Cast<TargetType>(e1);
	}
	else
	{
		location.throwError("Unsupported type");
	}

	return getEmptyNode<TargetType>();
}


TYPED_NODE FunctionParserBase::parseTerm()
{
	if (matchIf(NativeJitTokens::openParen))
	{
		if (matchIf(NativeJitTokens::float_))       return parseCast<T, float>();
		else if (matchIf(NativeJitTokens::int_))    return parseCast<T, int>();
		else if (matchIf(NativeJitTokens::double_)) return parseCast<T, double>();
		else
		{
			auto& result = parseSum<T>();
			match(NativeJitTokens::closeParen);
			return result;
		}
	}
	else if (currentType == NativeJitTokens::identifier || currentType == NativeJitTokens::literal)
		return parseFactor<T>();
	else
	{
		location.throwError("Parsing error");
	}

	return getEmptyNode<T>();
}

TYPED_NODE FunctionParserBase::parseFactor()
{
	TypeInfo t = peekFirstType();

	if (NativeJITTypeHelpers::matchesType<T>(t))
	{
		auto& e = parseSymbolOrLiteral<T>();

		return e;
	}
	else
	{
		location.throwError("Parsing error");
	}

	return getEmptyNode<T>();
}

TYPED_NODE FunctionParserBase::parseSymbolOrLiteral()
{
	if (matchIf(NativeJitTokens::identifier))	return parseSymbol<T>();
	else
	{
		match(NativeJitTokens::literal);
		return parseLiteral<T>();
	}
}

TYPED_NODE FunctionParserBase::getNodeForLine(NamedNode* r)
{
	if (NativeJIT::Node<T>* n = dynamic_cast<NativeJIT::Node<T>*>(r->node))
		return *n;
	else
		location.throwError("Type mismatch for Identifier " + currentValue.toString());

	return getEmptyNode<T>();
}

TYPED_NODE FunctionParserBase::parseSymbol()
{
	Identifier symbolId = Identifier(currentValue);

	if (info.parameterNames.indexOf(symbolId) != -1)		return parseParameterReference<T>(symbolId);
	if (auto r = getLine(symbolId))					return getNodeForLine<T>(r);
	else if (auto gn = getGlobalNode(symbolId))				return gn->template getLastNode<T>();
	else if (auto g = scope->getGlobal(symbolId))			return NativeJITTypeHelpers::matchesType<Buffer*>(g->getType()) ?
		parseBufferOperation<T>(symbolId) :
		getGlobalReference<T>(symbolId);
	else if (auto b = scope->getExposedFunction(symbolId))	return parseFunctionCall<T>(b);
	else if (auto cf = scope->getCompiledBaseFunction(symbolId)) return parseFunctionCall<T>(cf);
	else
	{
		location.throwError("Unknown identifier " + symbolId.toString());
	}

	return getEmptyNode<T>();
}

TYPED_NODE FunctionParserBase::parseParameterReference(const Identifier &id)
{
	auto x = parseParameterReferenceTyped(id);

	if (auto t = dynamic_cast<NativeJIT::Node<T>*>(x))
	{
		return *t;
	}
	else
	{
		location.throwError("Parameter type mismatch: " + NativeJITTypeHelpers::getTypeName<T>());
	}

	return getEmptyNode<T>();
}

TYPED_NODE FunctionParserBase::parseBufferOperation(const Identifier &id)
{
	if (matchIf(NativeJitTokens::openBracket))
	{
		if (NativeJITTypeHelpers::is<float, T>())
		{
			return *dynamic_cast<NativeJIT::Node<T>*>(&parseBufferAccess(id));
		}
		else
		{
			location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<T, float>());
		}

		return getEmptyNode<T>();
	}
	else
	{
		match(NativeJitTokens::dot);

		return parseBufferFunction<T>(id);
	}
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
	auto& b = getBufferNode(id);

	auto& index = parseSum<int>();
	match(NativeJitTokens::closeBracket);

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

TYPED_NODE FunctionParserBase::parseBufferFunction(const Identifier& id)
{
	const Identifier functionName = parseIdentifier();

	static const Identifier setSize("setSize");

	if (functionName == setSize)
	{
		auto& b = getBufferNode(id);
		auto& f = exprBase->Immediate(BufferOperations::setSize<T>);

		match(NativeJitTokens::openParen);
		auto& s = parseSum<int>();
		match(NativeJitTokens::closeParen);

		return exprBase->Call(f, b, s);
	}

	return getEmptyNode<T>();
}

TYPED_NODE FunctionParserBase::parseBufferAssignment(const Identifier &id)
{
	auto& b = getBufferNode(id);

	auto& index = parseSum<int>();
	match(NativeJitTokens::closeBracket);

	match(NativeJitTokens::assign_);

#if JUCE_WINDOWS
	auto& value = parseSum<float>();
#else
    auto& unwrapped = parseSum<float>();
    auto& dummyF = exprBase->Immediate(GlobalBase::returnSameValue<float>);
    auto& value = exprBase->Call(dummyF, unwrapped);
#endif

	if (info.useSafeBufferFunctions)
	{
		auto& f = exprBase->Immediate(BufferOperations::setSample<T>);
		return exprBase->Call(f, b, index, value);
	}
	else
	{
		auto& f = exprBase->Immediate(BufferOperations::setSampleRaw<T>);
		return exprBase->Call(f, b, index, value);
	}
}

TYPED_NODE FunctionParserBase::getGlobalReference(const Identifier& id)
{
	auto existingNode = getGlobalNode(id);

	const bool globalWasChanged = existingNode != nullptr;

	if (globalWasChanged)
	{
		return existingNode->template getLastNode<T>();
	}
	else
	{
		auto r = scope->getGlobal(id);

		if (!NativeJITTypeHelpers::matchesType<T>(r->type))
		{
			location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<T>(r->type));
		}

		auto& e1 = exprBase->Immediate(GlobalBase::get<T>);
		auto& g = exprBase->Immediate(r);

		return exprBase->Call(e1, g);
	}
}

TYPED_NODE FunctionParserBase::parseLiteral()
{
	T value = (T)currentValue;

	if (!NativeJITTypeHelpers::matchesType<T>(currentString))
	{
		location.throwError("Type mismatch: " + NativeJITTypeHelpers::getTypeName(currentString) + ", Expected: " + NativeJITTypeHelpers::getTypeName<T>());
	}

	return exprBase->Immediate(value);
}

TYPED_NODE FunctionParserBase::parseFunctionCall(BaseFunction* b)
{
	jassert(b != nullptr);

	match(NativeJitTokens::openParen);

	Array<NativeJIT::NodeBase*> parameterNodes;
	std::vector<TypeInfo> parameterTypes;

	while (currentType != NativeJitTokens::closeParen)
	{
		auto t = peekFirstType();

		parameterTypes.push_back(t);

		if (NativeJITTypeHelpers::matchesType<float>(t)) parameterNodes.add(&parseSum<float>());
		else if (NativeJITTypeHelpers::matchesType<double>(t)) parameterNodes.add(&parseSum<double>());
		else if (NativeJITTypeHelpers::matchesType<int>(t)) parameterNodes.add(&parseSum<int>());
		else location.throwError("Type unknown");

		matchIf(NativeJitTokens::comma);
	}

	switch (parameterTypes.size())
	{
	case 0:
	{
		return parseFunctionParameterList<T>(b);
	}
	case 1:
	{
		if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[0])) return parseFunctionParameterList<T, float>(b, parameterNodes[0]);
		else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[0])) return parseFunctionParameterList<T, int>(b, parameterNodes[0]);
		else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[0])) return parseFunctionParameterList<T, double>(b, parameterNodes[0]);
		else
		{
			location.throwError("No type deducted");
		}
	}
	case 2:
	{
		if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[0]))
		{
			if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, float, float>(b, parameterNodes[0], parameterNodes[1]);
			else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, float, int>(b, parameterNodes[0], parameterNodes[1]);
			else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, float, double>(b, parameterNodes[0], parameterNodes[1]);
			else
			{
				location.throwError("No type deducted");
			}
		}
		else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[0]))
		{
			if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, int, float>(b, parameterNodes[0], parameterNodes[1]);
			else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, int, int>(b, parameterNodes[0], parameterNodes[1]);
			else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, int, double>(b, parameterNodes[0], parameterNodes[1]);
			else
			{
				location.throwError("No type deducted");
			}
		}
		else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[0]))
		{
			if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, double, float>(b, parameterNodes[0], parameterNodes[1]);
			else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, double, int>(b, parameterNodes[0], parameterNodes[1]);
			else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, double, double>(b, parameterNodes[0], parameterNodes[1]);
			else
			{
				location.throwError("No type deducted");
			}
		}
	}
	default:
	{
		location.throwError("Function parsing error");
	}
	}

	return getEmptyNode<T>();
}

template <typename R> NativeJIT::Node<R>& FunctionParserBase::parseFunctionParameterList(BaseFunction* b)
{
	typedef R(*f_p)();

	auto function = (f_p)(b->func);

	if (function != nullptr)
	{
		auto& e2 = exprBase->Immediate(function);
		auto& e3 = exprBase->Call(e2);
		match(NativeJitTokens::closeParen);
		return e3;
	}
	else
	{
		location.throwError("Function type mismatch");
	}


	return getEmptyNode<R>();
}


template <typename R, typename ParamType> NativeJIT::Node<R>& FunctionParserBase::parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1)
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
			return e3;
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


	return getEmptyNode<R>();
}


template <typename R, typename ParamType1, typename ParamType2> NativeJIT::Node<R>& FunctionParserBase::parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1, NativeJIT::NodeBase* param2)
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
			return e4;
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


	return getEmptyNode<R>();
}

TYPED_NODE_VOID FunctionParserBase::checkParameterType(BaseFunction* b, int parameterIndex)
{
	if (b->getTypeForParameter(parameterIndex) != typeid(T))
	{
		location.throwError("Parameter " + String(parameterIndex + 1) + " type mismatch: " + NativeJITTypeHelpers::getTypeName<T>() + ", Expected: " + b->getTypeForParameter(parameterIndex).name());
	}
}

template <typename T>
void FunctionParserBase::parseGlobalAssignment(GlobalBase* g)
{
	enum AssignType
	{
		Assign = 0,
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		numAssignTypes
	};

	AssignType assignType;

	if (matchIf(NativeJitTokens::minusEquals)) assignType = Sub;
	else if (matchIf(NativeJitTokens::divideEquals)) assignType = Div;
	else if (matchIf(NativeJitTokens::timesEquals)) assignType = Mul;
	else if (matchIf(NativeJitTokens::plusEquals)) assignType = Add;
	else if (matchIf(NativeJitTokens::moduloEquals)) assignType = Mod;
	else
	{
		match(NativeJitTokens::assign_);
		assignType = Assign;
	}

	auto& exp = parseExpression<T>();

#if JUCE_MAC
    
    // OSX returns wrong values for double / floats if the expression is not wrapped into a dummy function call
    auto& dummyReturn = exprBase->Immediate(GlobalBase::returnSameValue<T>);
    auto& exp2 = exprBase->Call(dummyReturn, exp);
	NativeJIT::Node<T>* newNode = &exp2;
#else
    NativeJIT::Node<T>* newNode = &exp;
#endif
    
	NativeJIT::Node<T>* old = nullptr;

	auto existingNode = getGlobalNode(g->id);

	if (existingNode == nullptr) old = &getGlobalReference<T>(g->id);
	else						 old = &existingNode->template getLastNode<T>();

	switch (assignType)
	{

	case Add:	newNode = &exprBase->Add(*old, *newNode); break;
	case Sub:	newNode = &exprBase->Sub(*old, *newNode); break;
	case Mul:	newNode = &exprBase->Add(*old, *newNode); break;
	case Div:	newNode = &exprBase->Call(missingOperatorFunctions->template getDivideFunction<T>(exprBase), *old, *newNode); break;
	case Mod:	newNode = dynamic_cast<NativeJIT::Node<T>*>(&exprBase->Call(missingOperatorFunctions->getModuloFunction(exprBase), *dynamic_cast<NativeJIT::Node<int>*>(old), *dynamic_cast<NativeJIT::Node<int>*>(newNode))); break;
	case numAssignTypes:
	case Assign:
	default:
		break;
	}

	match(NativeJitTokens::semicolon);

	if (existingNode != nullptr)
	{
		existingNode->template addNode<T>(exprBase, *newNode);
	}
	else
	{
		ScopedPointer<GlobalNode> newGlobalNode = new GlobalNode(g);

		newGlobalNode->template addNode<T>(exprBase, *newNode);

		globalNodes.add(newGlobalNode.release());
	}
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

	while (peeker.currentType == NativeJitTokens::openParen)
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
		}
		if (auto r = getLine(currentId))
		{
			if (dynamic_cast<NativeJIT::Node<float>*>(r->node)) return typeid(float);
			if (dynamic_cast<NativeJIT::Node<double>*>(r->node)) return typeid(double);
			if (dynamic_cast<NativeJIT::Node<int>*>(r->node)) return typeid(int);
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

	return NativeJITTypeHelpers::getTypeForLiteral(peeker.currentString);
}

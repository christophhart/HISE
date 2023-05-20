namespace snex {
namespace jit {
using namespace juce;



TypeInfo BlockParser::getDotParentType(ExprPtr e, bool allowFunctionCallObjectExpression)
{
	if (auto dp = dynamic_cast<Operations::DotOperator*>(e.get()))
		return dp->getDotParent()->getTypeInfo();
	if (auto fc = dynamic_cast<Operations::FunctionCall*>(e.get()))
	{
		if (!allowFunctionCallObjectExpression)
			return {};

		if (auto e = fc->getObjectExpression())
			return e->getTypeInfo();
	}

	return {};
}

BlockParser::ExprPtr BlockParser::createBinaryNode(ExprPtr l, ExprPtr r, TokenType op)
{
	if (isVectorOp(op, l, r))
		return new Operations::VectorOp(location, l, op, r);
	else
		return new Operations::BinaryOp(location, l, r, op);
}

BlockParser::StatementPtr BlockParser::parseStatementList()
{
	matchIf(JitTokens::openBrace);

	auto list = new SyntaxTree(location, compiler->namespaceHandler.getCurrentNamespaceIdentifier());
	StatementPtr p = list;

	list->setParentScopeStatement(getCurrentScopeStatement());

	ScopedScopeStatementSetter svs(this, list);

	while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
	{
		if(matchIf(JitTokens::using_))
		{
			parseUsingAlias();
			continue;
		}

		if (matchIf(JitTokens::enum_))
		{
			parseEnum();
			continue;
		}

		list->addStatement(parseStatement());
	}

	matchIf(JitTokens::closeBrace);

	finaliseSyntaxTree(list);

	return p;
}



snex::jit::BlockParser::StatementPtr BlockParser::parseFunction(const Symbol& s)
{
	// The only function call that's allowed here is a Type definition
	auto typeDef = new Operations::ComplexTypeDefinition(location, { s.id }, s.typeInfo);

	InitialiserList::Ptr l = new InitialiserList();

	while (currentType != JitTokens::closeParen)
	{
		l->addChild(new InitialiserList::ExpressionChild(parseExpression()));

		if (!matchIf(JitTokens::comma))
		{
			match(JitTokens::closeParen);
			break;
		}
	}

	typeDef->addInitValues(l);

	return addConstructorToComplexTypeDef(typeDef, {s.id});
}

snex::VariableStorage BlockParser::parseConstExpression(bool isTemplateArgument)
{
	if (currentScope == nullptr)
	{
		if (currentType == JitTokens::identifier)
		{
			SymbolParser sp(*this, compiler->namespaceHandler);
			auto id = sp.parseExistingSymbol(true);
			return compiler->namespaceHandler.getConstantValue(id.id);
		}

		return parseVariableStorageLiteral();
	}
	else
	{
		ScopedTemplateArgParser s(*this, isTemplateArgument);

		auto expr = parseExpression();

		expr->currentCompiler = compiler;
		expr->currentScope = currentScope;

		expr = Operations::evalConstExpr(expr);

		if (!expr->isConstExpr())
			location.throwError("Can't assign static constant to a dynamic expression");

		return expr->getConstExprValue();
	}
}



juce::Array<snex::jit::TemplateParameter> BlockParser::parseTemplateParameters(bool parseTemplateDefinition)
{
	Array<TemplateParameter> parameters;

	match(JitTokens::lessThan);

	while (currentType != JitTokens::greaterThan && !isEOF())
	{
		if (parseTemplateDefinition)
		{
			if (matchIf(JitTokens::int_))
			{
				auto vType = parseVariadicDots();

				auto templateId = NamespacedIdentifier(parseIdentifier());

				int defaultValue = 0;
				bool defined = false;

				if (matchIf(JitTokens::assign_))
				{
					defaultValue = parseConstExpression(true).toInt();
					defined = true;
				}

				parameters.add(TemplateParameter(templateId, defaultValue, defined, vType));
			}
			else
			{
				match(JitTokens::typename_);

				auto vType = parseVariadicDots();

				auto templateId = NamespacedIdentifier(parseIdentifier());

				TypeInfo defaultType;

				if (matchIf(JitTokens::assign_))
				{
					TypeParser tp(*this, compiler->namespaceHandler, {});
					tp.matchType();
					defaultType = tp.currentTypeInfo;
				}

				parameters.add(TemplateParameter(templateId, defaultType, vType));
			}
		}
		else
		{
			TypeParser tp(*this, compiler->namespaceHandler, {});

			if (tp.matchIfType())
			{
				auto vType = parseVariadicDots();
				TemplateParameter p(tp.currentTypeInfo, vType);

				parameters.add(p);
			}
			else
			{
				auto e = parseConstExpression(true);

				if (e.getType() != Types::ID::Integer)
					location.throwError("Can't use non-integers as template argument");

				TemplateParameter tp(e.toInt());

				parameters.add(tp);
			}
		}

		matchIf(JitTokens::comma);
	}

	match(JitTokens::greaterThan);

	return parameters;

#if 0
	Array<TemplateParameter> parameters;

	match(JitTokens::lessThan);

	while (currentType != JitTokens::greaterThan)
	{
		if (matchIfType())
		{
			TemplateParameter tp;
			tp.type = currentTypeInfo;
			parameters.add(tp);
		}

		if (currentType == JitTokens::literal)
		{
			auto v = parseVariableStorageLiteral();

			if (v.getType() == Types::ID::Integer)
			{
				TemplateParameter tp;
				tp.constant = v.toInt();
				parameters.add(tp);
			}
			else
				location.throwError("Can't use non-integers as template argument");
		}
		
		else
			location.throwError("Invalid template parameter: " + juce::String(currentType));

		matchIf(JitTokens::comma);
	}

	match(JitTokens::greaterThan);

	return parameters;
#endif
}

snex::jit::BlockParser::StatementPtr BlockParser::parseComplexTypeDefinition(bool mustBeDestructor)
{
    using namespace Operations;
    
	jassert(getCurrentComplexType() != nullptr);

	Array<NamespacedIdentifier> ids;

	

	auto typePtr = getCurrentComplexType();

	if (mustBeDestructor)
		currentTypeInfo = TypeInfo(Types::ID::Void);

	auto t = currentTypeInfo;

	auto rootId = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

	bool isConstructor = rootId.toString() == typePtr->toString();



	if (isConstructor)
	{
		if (mustBeDestructor)
			ids.add(rootId.getChildId(FunctionClass::getSpecialSymbol(rootId, FunctionClass::Destructor)));
		else
			ids.add(rootId.getChildId(FunctionClass::getSpecialSymbol(rootId, FunctionClass::Constructor)));
	}
	else
	{
		if (mustBeDestructor)
			location.throwError("Expected destructor()");

		ids.add(rootId.getChildId(parseIdentifier()));
	}
		

	if (matchIf(JitTokens::openParen))
	{
		

 		Symbol s(ids.getFirst(), t);

		CommentAttacher ca(*this);
		compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Function, ca.getInfo());

		auto st = parseFunction(s);

		return matchIfSemicolonAndReturn(st);
	}
	else
	{
		while (matchIf(JitTokens::comma))
			ids.add(rootId.getChildId(parseIdentifier()));

		auto n = new ComplexTypeDefinition(location, ids, currentTypeInfo);

		CommentAttacher ca(*this);

		for (auto id : ids)
			compiler->namespaceHandler.addSymbol(id, currentTypeInfo, NamespaceHandler::Variable, ca.getInfo());

		if (matchIf(JitTokens::assign_))
		{
			if (currentType == JitTokens::openBrace)
				n->addInitValues(parseInitialiserList());
			else
            {
                auto expr = parseExpression();
                
                if(auto fc = as<FunctionCall>(expr))
                {
                    if(auto fa = fc->extractFirstArgumentFromConstructor(compiler->namespaceHandler))
                    {
                        expr = fa;
                    }
                }
                
				n->addStatement(expr);
            }
		}

		return addConstructorToComplexTypeDef(n, ids);
	}
}

snex::jit::BlockParser::StatementPtr BlockParser::addConstructorToComplexTypeDef(StatementPtr def, const Array<NamespacedIdentifier>& ids, bool matchSemicolon)
{
	auto n = Operations::as<Operations::ComplexTypeDefinition>(def);

    currentTypeInfo = def->getTypeInfo();

	if (currentTypeInfo.isComplexType() && currentTypeInfo.getComplexType()->hasConstructor())
	{
		// If objects are created on the stack they might have not been finalised yet
		currentTypeInfo.getComplexType()->finaliseAlignment();

		StatementPtr cd = n;

		FunctionClass::Ptr fc = currentTypeInfo.getComplexType()->getFunctionClass();

		if (!fc->hasSpecialFunction(FunctionClass::Constructor))
		{
			location.throwError("Can't find constructor");
		}

		auto ab = new Operations::AnonymousBlock(location);
		ab->addStatement(n);

		for (auto id : ids)
		{
			FunctionClass::Ptr fc = currentTypeInfo.getComplexType()->getFunctionClass();

            FunctionData cf;
            
            if(auto singleArg = n->getSubExpr(0))
            {
                cf = fc->getConstructor({ singleArg->getTypeInfo() });
            }
            else
            {
                cf = fc->getConstructor(n->initValues);
            }
            

			if (!cf.id.isValid())
			{
				String error;
				error << "Can't find constructor that matches init values ";

				if (n->initValues != nullptr)
					error << n->initValues->toString();

				location.throwError(error);
			}
				
			
			auto call = new Operations::FunctionCall(location, nullptr, Symbol(cf.id, TypeInfo(Types::ID::Void)), {});
			auto obj = new Operations::VariableReference(location, Symbol(id, TypeInfo(currentTypeInfo)));

			call->setObjectExpression(obj);

			if (n->initValues != nullptr)
			{
				n->initValues->forEach([&](InitialiserList::ChildBase* cb)
                {
                    if (auto e = dynamic_cast<InitialiserList::ExpressionChild*>(cb))
                    {
                        call->addArgument(e->expression->clone(location));
                    }
                    else
                    {
                        VariableStorage immValue;

                        if (cb->getValue(immValue))
                            call->addArgument(new Operations::Immediate(location, immValue));
                    }

                    return false;
                });
			}
            else if(auto firstChild = n->getSubExpr(0))
            {
                // this will cover the
                // Type x = initValue;
                // syntax
                call->addArgument(firstChild->clone(location));
            }
            
			ab->addStatement(call);
		}

        if(matchSemicolon)
            match(JitTokens::semicolon);
        
		return ab;
	}

    if(matchSemicolon)
        match(JitTokens::semicolon);

	return n;
}

InitialiserList::Ptr BlockParser::parseInitialiserList()
{
	match(JitTokens::openBrace);

	InitialiserList::Ptr root = new InitialiserList();

	bool next = true;

	while (next)
	{
		if (currentType == JitTokens::openBrace)
			root->addChildList(parseInitialiserList());
		else
		{
			auto exp = parseExpression();

			if (exp->isConstExpr())
				root->addImmediateValue(exp->getConstExprValue());
			else
				root->addChild(new InitialiserList::ExpressionChild(exp));
		}
			

		next = matchIf(JitTokens::comma);
	}

	match(JitTokens::closeBrace);

	return root;
}




snex::NamespacedIdentifier BlockParser::getDotParentName(ExprPtr e)
{
	if (auto dp = dynamic_cast<Operations::DotOperator*>(e.get()))
	{
		if (auto ss = dynamic_cast<Operations::SymbolStatement*>(dp->getDotParent().get()))
		{
			return ss->getSymbol().id;
		}
	}

	return {};
}

TemplateInstance BlockParser::getTemplateInstanceFromParent(ExprPtr p, NamespacedIdentifier id)
{
	auto parentType = getDotParentType(p, true);

	TemplateParameter::List parentInstanceParameters;

	if (parentType.isTemplateType())
	{
		for (auto& tp : compiler->namespaceHandler.getCurrentTemplateParameters())
		{
			if (tp.argumentId == parentType.getTemplateId())
			{
				parentType = tp.type;
				break;
			}
		}
	}

	if (auto ct = parentType.getTypedIfComplexType<ComplexType>())
	{
		if (FunctionClass::Ptr vfc = ct->getFunctionClass())
		{
			id = vfc->getClassName().getChildId(id.getIdentifier());

			if (auto st = parentType.getTypedIfComplexType<StructType>())
			{
				parentInstanceParameters = st->getTemplateInstanceParametersForFunction(id);
			}
		}
	}

	return TemplateInstance(id, parentInstanceParameters);
}

void BlockParser::parseUsingAlias()
{
	if (matchIf(JitTokens::namespace_))
	{
		auto id = compiler->namespaceHandler.getRootId();

		id = id.getChildId(parseIdentifier());

		while (matchIf(JitTokens::colon))
		{
			match(JitTokens::colon);
			id = id.getChildId(parseIdentifier());
		}


		auto r = compiler->namespaceHandler.addUsedNamespace(id);

		if (!r.wasOk())
			location.throwError(r.getErrorMessage());

		match(JitTokens::semicolon);
		return;
	}
	else
	{
		auto s = parseNewSymbol(NamespaceHandler::UsingAlias);

		match(JitTokens::assign_);

		if (!matchIfType({}))
			location.throwError("Expected type");

		if (currentTypeInfo.isComplexType())
			currentTypeInfo.getComplexType()->setAlias(s.id);

		s.typeInfo = currentTypeInfo;
		match(JitTokens::semicolon);
		compiler->namespaceHandler.setTypeInfo(s.id, NamespaceHandler::UsingAlias, s.typeInfo);
	}
}

void BlockParser::parseEnum()
{
	bool isClassEnum = matchIf(JitTokens::class_);

	currentTypeInfo = TypeInfo(Types::ID::Integer, true, false);

	auto enumId = parseNewSymbol(NamespaceHandler::SymbolType::Enum);

	int value = 0;

	struct Item
	{
		Identifier id;
		int value;
	};

	Array<Item> items;

	match(JitTokens::openBrace);

	bool expectNext = true;

	enum class En
	{
		aas = 9
	};

	

	while (!isEOF() && currentType != JitTokens::closeBrace)
	{
		if (!expectNext)
			location.throwError("expected }");

		auto enumId = parseIdentifier();

		if (matchIf(JitTokens::assign_))
		{
			value = parseConstExpression(false).toInt();
		}

		items.add({ enumId, value });
		value++;
		expectNext = matchIf(JitTokens::comma);
	}

	auto& nh = compiler->namespaceHandler;

	{
		NamespaceHandler::ScopedNamespaceSetter sns(nh, enumId.getId());

		auto currentNamespace = nh.getCurrentNamespaceIdentifier();

		TypeInfo type(Types::ID::Integer, isClassEnum, false);

		for (auto i : items)
		{
			CommentAttacher ca(*this);
			auto idToUse = currentNamespace.getChildId(i.id);
			nh.addSymbol(idToUse, type, NamespaceHandler::EnumValue, ca.getInfo());
			nh.addConstant(idToUse, i.value);
		}
	}
	if (!isClassEnum)
	{
		// now populate the parent namespace with the enum values too...

		auto currentNamespace = nh.getCurrentNamespaceIdentifier();

		// We'll abuse the const attribute to mark class enums so that they
		// need to be casted to an int...
		TypeInfo type(Types::ID::Integer, false, false);

		for (auto i : items)
		{
			auto idToUse = currentNamespace.getChildId(i.id);
			CommentAttacher ca(*this);
			nh.addSymbol(idToUse, type, NamespaceHandler::EnumValue, ca.getInfo());
			nh.addConstant(idToUse, i.value);
		}
	}

	match(JitTokens::closeBrace);
	match(JitTokens::semicolon);
}

bool BlockParser::isVectorOp(TokenType t, BlockParser::ExprPtr l, BlockParser::ExprPtr r /*= nullptr*/)
{
	ignoreUnused(t);

	if (auto atb = l->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
	{
		return true;
	}

	if (r == nullptr)
		return false;

	if (auto atb = r->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
	{
		return true;
	}

	return false;
}

bool BlockParser::skipIfConsoleCall()
{
	if (currentType == JitTokens::identifier)
	{
		auto id = Identifier(currentValue.toString());

		static const Identifier console("Console");

		if (id == console)
		{
			// you monkey-clicked on a breakpoint outside a function, so skip until the next semicolon

			while (currentType != JitTokens::semicolon && !isEOF())
				skip();

			match(JitTokens::semicolon);

			location.calculatePosIfEnabled(compiler->namespaceHandler.shouldCalculateNumbers());

			ParserHelpers::Error e(location);
			e.errorMessage = "Console call outside function body";
			compiler->logMessage(BaseCompiler::Warning, e.toString());

			return true;
		}
	}

	return false;
}

Operations::Expression::Ptr BlockParser::parseExpression()
{
	return parseTernaryOperator();
}

BlockParser::ExprPtr BlockParser::parseTernaryOperator()
{
	ExprPtr condition = parseBool();

	if (matchIf(JitTokens::question))
	{
		ExprPtr trueBranch = parseExpression();
		match(JitTokens::colon);
		ExprPtr falseBranch = parseExpression();
		return new Operations::TernaryOp(location, condition, trueBranch, falseBranch);
	}

	return condition;
}


BlockParser::ExprPtr BlockParser::parseBool()
{
	const bool isInverted = matchIf(JitTokens::logicalNot);
	ExprPtr result = parseLogicOperation();

	if (!isInverted)
		return result;
	else
		return new Operations::LogicalNot(location, result);
}


BlockParser::ExprPtr BlockParser::parseLogicOperation()
{
	ExprPtr left = parseComparation();

	if (matchIf(JitTokens::logicalAnd))
	{
		ExprPtr right = parseLogicOperation();
		return new Operations::BinaryOp(location, left, right, JitTokens::logicalAnd);
	}
	else if (matchIf(JitTokens::logicalOr))
	{
		ExprPtr right = parseLogicOperation();
		return new Operations::BinaryOp(location, left, right, JitTokens::logicalOr);
	}
	else
		return left;
}

BlockParser::ExprPtr BlockParser::parseComparation()
{
	ExprPtr left = parseSum();

	if (currentType == JitTokens::greaterThan ||
		currentType == JitTokens::greaterThanOrEqual ||
		currentType == JitTokens::lessThan ||
		currentType == JitTokens::lessThanOrEqual ||
		currentType == JitTokens::equals ||
		currentType == JitTokens::notEquals)
	{
		// If this is true, we are parsing a template argument 
		// and don't want to consume the '>' token...
		if (isParsingTemplateArgument)
			return left;

		TokenType op = currentType;
		skip();
		ExprPtr right = parseSum();
		return new Operations::Compare(location, left, right, op);
	}
	else
		return left;
}


BlockParser::ExprPtr BlockParser::parseSum()
{
	ExprPtr left(parseDifference());

	if (currentType == JitTokens::plus)
	{
		TokenType op = currentType;		  skip();
		ExprPtr right(parseSum());
		return createBinaryNode(left, right, op);
	}
	else
		return left;
}


BlockParser::ExprPtr BlockParser::parseProduct()
{
	ExprPtr left(parseTerm());

	if (currentType == JitTokens::times ||
		currentType == JitTokens::divide ||
		currentType == JitTokens::modulo)
	{
		TokenType op = currentType;		  skip();
		ExprPtr right(parseProduct());
		return createBinaryNode(left, right, op);
	}
	else
		return left;
}



BlockParser::ExprPtr BlockParser::parseDifference()
{
	ExprPtr left(parseProduct());

	if (currentType == JitTokens::minus)
	{
		TokenType op = currentType;		  skip();
		ExprPtr right(parseDifference());
		return createBinaryNode(left, right, op);
	}
	else
		return left;
}

BlockParser::ExprPtr BlockParser::parseTerm()
{
	if (matchIf(JitTokens::openParen))
	{
		if (matchIfType({}))
		{
			if (currentTypeInfo.isTemplateType())
			{
				match(JitTokens::closeParen);
				return parseExpression();
			}

			if (currentTypeInfo.isComplexType())
				location.throwError("Can't cast to " + currentTypeInfo.toString());

			return parseCast(currentTypeInfo.getType());
		}
		else
		{
			auto result = parseExpression();
			match(JitTokens::closeParen);
			return result;
		}
	}
	else
		return parseUnary();
}


BlockParser::ExprPtr BlockParser::parseCast(Types::ID type)
{
	match(JitTokens::closeParen);
	ExprPtr source(parseTerm());
	return new Operations::Cast(location, source, type);
}


BlockParser::ExprPtr BlockParser::parseUnary()
{
	if (matchIf(JitTokens::times))
	{
		match(JitTokens::this_);

		auto p = parseThis();

		return new Operations::PointerAccess(location, p);
	}

	if (matchIf(JitTokens::this_))
	{
		auto p = parseThis();

		if (matchIf(JitTokens::pointer_))
		{
			auto c = parseReference(false);
			auto d = new Operations::DotOperator(location, p, c);

			return parseDotOperator(d);
		}
	}

	if (currentType == JitTokens::identifier ||
		currentType == JitTokens::literal ||
		currentType == JitTokens::minus ||
		currentType == JitTokens::plusplus ||
		currentType == JitTokens::minusminus)
	{
		return parseFactor();
	}
	else if (matchIf(JitTokens::true_))
	{
		return new Operations::Immediate(location, 1);
	}
	else if (matchIf(JitTokens::false_))
	{
		return new Operations::Immediate(location, 0);
	}
	else if (currentType == JitTokens::logicalNot)
	{
		return parseBool();
	}
	else
	{
		location.throwError("Parsing error");
	}

	return nullptr;
}


snex::jit::BlockParser::ExprPtr BlockParser::parseThis()
{
	if (auto fs = currentScope->getParentScopeOfType<FunctionScope>())
	{
		if (auto typePtr = fs->getClassType())
			return new Operations::ThisPointer(location, TypeInfo(typePtr, false, true));
        else
        {
            jassertfalse;
        }
	}

	location.throwError("Can't use this pointer outside of class method");
	return nullptr;
}

BlockParser::ExprPtr BlockParser::parseFactor()
{
	if (matchIf(JitTokens::plusplus))
	{
		ExprPtr expr = parseReference();
		return new Operations::Increment(location, expr, true, false);
	}
	if (matchIf(JitTokens::minusminus))
	{
		ExprPtr expr = parseReference();
		return new Operations::Increment(location, expr, true, true);
	}
	if (matchIf(JitTokens::minus))
	{
		if (currentType == JitTokens::literal)
			return parseLiteral(true);
		else
		{
			ExprPtr expr = parseReference();
			return new Operations::Negation(location, expr);
		}
	}
	//	else return parseSymbolOrLiteral();
	else if (currentType == JitTokens::identifier)
	{
		return parsePostSymbol();
	}
	else
		return parseLiteral();
}


BlockParser::ExprPtr BlockParser::parseDotOperator(ExprPtr p)
{
	while (matchIf(JitTokens::dot))
	{
		

		auto e = parseReference(false);
		auto dp = new Operations::DotOperator(location, p, e);
		p = dp;
		dp->tryToResolveType(compiler);
	}

	return parseSubscript(p);
}


BlockParser::ExprPtr BlockParser::parseSubscript(ExprPtr p)
{
	bool found = false;

	while (matchIf(JitTokens::openBracket))
	{
		ExprPtr idx = parseExpression();

		match(JitTokens::closeBracket);
		p = new Operations::Subscript(location, p, idx);
		found = true;
	}

	return found ? parseDotOperator(p) : parseCall(p);
}



BlockParser::ExprPtr BlockParser::parseCall(ExprPtr p)
{
	using namespace Operations;

	bool found = false;
	auto r = Result::ok();

	Array<TemplateParameter> templateParameters;

	auto fSymbol = getCurrentSymbol();


	if (currentType == JitTokens::lessThan)
	{
		auto tId = getTemplateInstanceFromParent(p, fSymbol.id);

		NamespaceHandler::ScopedTemplateParameterSetter psp(compiler->namespaceHandler, tId.tp);

		if (compiler->namespaceHandler.isTemplateFunction(tId))
		{
			templateParameters = parseTemplateParameters(false);

			auto numProvided = templateParameters.size();

			if (TemplateParameter::ListOps::isVariadicList(templateParameters))
			{
				auto currentParameters = compiler->namespaceHandler.getCurrentTemplateParameters();
				TemplateParameter::ListOps::expandIfVariadicParameters(templateParameters, currentParameters);
				numProvided = templateParameters.size();
			}

			auto to = compiler->namespaceHandler.getTemplateObject(tId, numProvided);

			if (TemplateParameter::ListOps::isValidTemplateAmount(to.argList, numProvided))
			{
				compiler->namespaceHandler.createTemplateFunction(tId, templateParameters, r);
				location.test(r);
			}
		}
	}

	// Template type deduction...
	bool resolveAfterArgumentParsing = false;

	if (auto ct = getDotParentType(p, false).getTypedIfComplexType<ComplexType>())
	{
		if (auto tc = dynamic_cast<TemplatedComplexType*>(ct))
		{
			auto ctp = compiler->namespaceHandler.getCurrentTemplateParameters();

			auto tp = tc->getTemplateInstanceParameters();

			for (auto& a : tp)
			{
				auto idToUse = a.type.getTemplateId();

				for (auto t : ctp)
				{
					if (t.argumentId == idToUse)
					{
						a = t;
					}
				}
			}

			TemplateParameter::ListOps::isParameter(tp);

			auto r = Result::ok();

			ct = compiler->namespaceHandler.registerComplexTypeOrReturnExisting(tc->createTemplatedInstance(tp, r)).get();

			location.test(r);
		}

		FunctionClass::Ptr vfc = ct->getFunctionClass();

		auto mId = vfc->getClassName().getChildId(fSymbol.getName());

		Array<FunctionData> matches;
		vfc->addMatchingFunctions(matches, mId);

		auto f = matches.getFirst();



		if (!f.templateParameters.isEmpty() && templateParameters.isEmpty())
		{
			if (currentType == JitTokens::lessThan)
			{
				templateParameters = parseTemplateParameters(false);
			}
			else
			{
				resolveAfterArgumentParsing = true;
			}
		}

		if (matches.size() > 1 && !templateParameters.isEmpty())
		{
			for (const auto& m : matches)
			{
				if (m.matchesTemplateArguments(templateParameters))
				{
					f = m;
					break;
				}
			}
		}
		else
		{
			if (!resolveAfterArgumentParsing &&
				TemplateParameter::ListOps::isArgument(f.templateParameters))
			{
				// externally defined functions might not have template arguments, but are already in parameter
				// format

				templateParameters = TemplateParameter::ListOps::merge(f.templateParameters, templateParameters, r);
				location.test(r);
			}
		}

		if (auto il = f.inliner)
		{
			if (il->returnTypeFunction)
			{
				ReturnTypeInlineData rt(f);

				rt.object = p->getSubExpr(0); // can be either dot parent or object expression of function call
				rt.object->currentCompiler = compiler;
				rt.templateParameters = templateParameters;

				r = il->process(&rt);
				location.test(r);

				f.templateParameters = TemplateParameter::ListOps::merge(f.templateParameters, templateParameters, r);
				location.test(r);

				fSymbol = Symbol(f.id, f.returnType);
				resolveAfterArgumentParsing = f.returnType.isDynamic();
			}
		}

		

		if (!f.returnType.isDynamic())
		{
			fSymbol = Symbol(f.id, f.returnType);
			jassert(fSymbol.resolved);
		}

	}



	FunctionData nf;

	if (auto nfc = compiler->getInbuiltFunctionClass())
	{
		nf = compiler->getInbuiltFunctionClass()->getNonOverloadedFunction(fSymbol.id);
	}



	if (nf.isResolved())
	{
		if (nf.returnType.isDynamic())
		{
			if (nf.inliner == nullptr || !nf.inliner->returnTypeFunction)
				location.throwError("Can't deduce return type");

			if (templateParameters.isEmpty())
				resolveAfterArgumentParsing = true;
			else
				jassertfalse;
		}
	}



	while (matchIf(JitTokens::openParen))
	{
		if (auto dot = as<DotOperator>(p))
		{
			if (auto s = as<SymbolStatement>(dot->getDotParent()))
			{
				auto symbol = s->getSymbol();
				auto parentSymbol = symbol.id;

				if (compiler->namespaceHandler.isStaticFunctionClass(parentSymbol))
				{
					fSymbol.id = parentSymbol.getChildId(fSymbol.id.getIdentifier());
					fSymbol.resolved = false;
					p = nullptr;
				}
			}
		}

		

		auto f = new FunctionCall(location, p, fSymbol, templateParameters);

		while (!isEOF() && currentType != JitTokens::closeParen)
		{
			f->addArgument(parseExpression());
			matchIf(JitTokens::comma);
		};

		p = f;
		found = true;

		if (resolveAfterArgumentParsing)
		{
			f->tryToResolveType(compiler);
		}

		match(JitTokens::closeParen);
	}

	if (auto f = as<FunctionCall>(p))
	{
		auto fId = getTemplateInstanceFromParent(f, f->function.id);

		if (compiler->namespaceHandler.isTemplateFunction(fId))
		{
			auto numProvided = f->function.templateParameters.size();

			auto to = compiler->namespaceHandler.getTemplateObject(fId, numProvided);

			if (!to.id.isValid())
			{
				auto matches = compiler->namespaceHandler.getAllTemplateObjectsWith(fId);

				if (matches.size() == 1)
					to = matches.getFirst();
				else
				{
					for (auto& m : matches)
					{
						if (TemplateParameter::ListOps::isValidTemplateAmount(m.argList, numProvided))
						{
							to = m;
							break;
						}
					}
				}
			}

			if (!to.id.isValid())
			{
				String s;
				s << "Can't deduce template function" << f->function.getSignature({});
				location.throwError(s);
			}

			if (numProvided != to.argList.size())
			{
				TypeInfo::List callParameters;

				for (int i = 0; i < f->getNumArguments(); i++)
					callParameters.add(f->getArgument(i)->getTypeInfo());

				auto originalList = to.functionArgs();

				if (callParameters.size() < originalList.size())
				{
					// We need to add default values...
					int numDefinedArgs = callParameters.size();

					for (int i = numDefinedArgs; i < originalList.size(); i++)
					{
						callParameters.add(f->function.args[i].typeInfo);
					}
				}

				templateParameters = TemplateParameter::ListOps::mergeWithCallParameters(to.argList, templateParameters, originalList, callParameters, r);

				if (!templateParameters.isEmpty())
				{
					templateParameters = TemplateParameter::ListOps::merge(to.argList, templateParameters, r);
					location.test(r);

					f->function.templateParameters = templateParameters;

					compiler->namespaceHandler.createTemplateFunction(fId, templateParameters, r);
					location.test(r);
				}
			}
		}


	}



	return found ? parseDotOperator(p) : p;
}



BlockParser::ExprPtr BlockParser::parsePostSymbol()
{
	auto expr = parseReference();

	expr = parseDotOperator(expr);

	if (matchIf(JitTokens::plusplus))
		expr = new Operations::Increment(location, expr, false, false);
	else if (matchIf(JitTokens::minusminus))
		expr = new Operations::Increment(location, expr, false, true);

	return expr;
}




BlockParser::ExprPtr BlockParser::parseReference(bool mustBeResolvedAtCompileTime)
{
	if (matchIf(JitTokens::this_))
	{
		match(JitTokens::pointer_);

		if (auto fs = currentScope->getParentScopeOfType<FunctionScope>())
		{
			if (auto typePtr = fs->getClassType())
			{
				auto p = new Operations::ThisPointer(location, TypeInfo(typePtr, false, true));
				auto c = parseReference(false);
				auto d = new Operations::DotOperator(location, p, c);

				return parseDotOperator(d);
			}
		}

		location.throwError("Can't use this pointer outside of class method");
	}

	

	if (mustBeResolvedAtCompileTime)
		parseExistingSymbol();
	else
	{
		currentTypeInfo = {};
		currentSymbol = Symbol(parseIdentifier());
	}

	for (const auto& tp : compiler->namespaceHandler.getCurrentTemplateParameters())
	{
		if (tp.argumentId == currentSymbol.id)
		{
			jassert(tp.constantDefined);
			return new Operations::Immediate(location, VariableStorage(tp.constant));
		}
	}

	return new Operations::VariableReference(location, getCurrentSymbol());
}

BlockParser::ExprPtr BlockParser::parseLiteral(bool isNegative)
{
	auto v = parseVariableStorageLiteral();

	if (isNegative)
	{
		if (v.getType() == Types::ID::Integer) v = VariableStorage(v.toInt() * -1);
		else if (v.getType() == Types::ID::Float)   v = VariableStorage(v.toFloat() * -1.0f);
		else if (v.getType() == Types::ID::Double)  v = VariableStorage(v.toDouble() * -1.0);
	}

	return new Operations::Immediate(location, v);
}



}
}


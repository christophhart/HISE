namespace snex {
namespace jit {
using namespace juce;

int Compiler::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	auto c = source.nextChar();

	if (c == '|')
	{
		while (!source.isEOF() && source.peekNextChar() != '{')
			source.skip();

		return BaseCompiler::MessageType::ValueName;
	}
	if (c == '{')
	{
		source.skipToEndOfLine();
		return BaseCompiler::MessageType::ValueDump;
	}
	if (c == 'P')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::PassMessage;
	}
	if (c == 'W')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Warning;
	}
	if (c == 'O')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::AsmJitMessage;
	}
	if (c == 'E')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Error;
	}
	if (c == '-')
	{
		c = source.nextChar();

		source.skipToEndOfLine();

		if (c == '-')
			return BaseCompiler::MessageType::VerboseProcessMessage;
		else
			return BaseCompiler::MessageType::ProcessMessage;
	}

	return BaseCompiler::MessageType::ProcessMessage;
}



class ClassCompiler final: public BaseCompiler
{
public:

	ClassCompiler(BaseScope* parentScope_, NamespaceHandler& handler, const NamespacedIdentifier& classInstanceId = {}) :
		BaseCompiler(handler),
		instanceId(classInstanceId),
		parentScope(parentScope_),
		lastResult(Result::ok())
	{
		if (auto gs = parentScope->getGlobalScope())
		{
			const auto& optList = gs->getOptimizationPassList();

			if (!optList.isEmpty())
			{
				OptimizationFactory f;

				for (const auto& id : optList)
					addOptimization(f.createOptimization(id), id);
			}
		}

		newScope = new JitCompiledFunctionClass(parentScope, classInstanceId);
	};

	virtual ~ClassCompiler()
	{
		syntaxTree = nullptr;
	}

	void setFunctionCompiler(asmjit::X86Compiler* cc)
	{
		asmCompiler = cc;
	}

	asmjit::Runtime* getRuntime()
	{
		if (parentRuntime != nullptr)
			return parentRuntime;

		return newScope->pimpl->runtime;
	}

	bool parseOnly = false;
	asmjit::Runtime* parentRuntime = nullptr;

	JitCompiledFunctionClass* compileAndGetScope(const ParserHelpers::CodeLocation& classStart, int length)
	{
		NewClassParser parser(this, classStart, length);

		if (newScope == nullptr)
		{
			newScope = new JitCompiledFunctionClass(parentScope, instanceId);
		}

		newScope->pimpl->handler = &namespaceHandler;

		try
		{
			parser.currentScope = newScope->pimpl;

			setCurrentPass(BaseCompiler::Parsing);

			NamespaceHandler::ScopedNamespaceSetter sns(namespaceHandler, Identifier());

			syntaxTree = parser.parseStatementList();

			auto sTree = dynamic_cast<SyntaxTree*>(syntaxTree.get());

			executePass(ComplexTypeParsing, newScope->pimpl, sTree);
			executePass(DataSizeCalculation, newScope->pimpl, sTree);

			newScope->pimpl->getRootData()->finalise();

			auto d = (int*)newScope->pimpl->getRootData()->data.get();



			executePass(DataAllocation, newScope->pimpl, sTree);
			executePass(DataInitialisation, newScope->pimpl, sTree);
			executePass(PreSymbolOptimization, newScope->pimpl, sTree);
			executePass(ResolvingSymbols, newScope->pimpl, sTree);
			executePass(TypeCheck, newScope->pimpl, sTree);

			if (parseOnly)
			{
				lastResult = Result::ok();
				return nullptr;
			}

			executePass(SyntaxSugarReplacements, newScope->pimpl, sTree);
			executePass(PostSymbolOptimization, newScope->pimpl, sTree);
			
			executePass(FunctionTemplateParsing, newScope->pimpl, sTree);
			executePass(FunctionParsing, newScope->pimpl, sTree);

			// Optimize now

			executePass(FunctionCompilation, newScope->pimpl, sTree);

			lastResult = newScope->pimpl->getRootData()->callRootConstructors();
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			syntaxTree = nullptr;

			logMessage(BaseCompiler::Error, e.toString());
			lastResult = Result::fail(e.toString());
		}

		return newScope.release();
	}

	JitCompiledFunctionClass* compileAndGetScope(const juce::String& code)
	{
		

		ParserHelpers::CodeLocation loc(code.getCharPointer(), code.getCharPointer());

		return compileAndGetScope(loc, code.length());
	}

	Result getLastResult() { return lastResult; }

	ScopedPointer<JitCompiledFunctionClass> newScope;

	asmjit::X86Compiler* asmCompiler;

	juce::String assembly;

	Result lastResult;

	BaseScope* parentScope;
	NamespacedIdentifier instanceId;

	Operations::Statement::Ptr syntaxTree;
};


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

void NewClassParser::registerTemplateArguments(TemplateParameter::List& templateList, const NamespacedIdentifier& scopeId)
{
	jassert(compiler->namespaceHandler.getCurrentNamespaceIdentifier() == scopeId);

	for (auto& tp : templateList)
	{
		jassert(tp.isTemplateArgument());
		jassert(tp.argumentId.isExplicit() || tp.argumentId.getParent() == scopeId);

		tp.argumentId = scopeId.getChildId(tp.argumentId.getIdentifier());

		jassert(tp.argumentId.getParent() == scopeId);

		if (tp.t == TemplateParameter::TypeTemplateArgument)
		{
			compiler->namespaceHandler.addSymbol(tp.argumentId, tp.type, NamespaceHandler::TemplateType);
		}
		else
		{
			compiler->namespaceHandler.addSymbol(tp.argumentId, TypeInfo(Types::ID::Integer), NamespaceHandler::TemplateConstant);
		}
	}
}

snex::jit::BlockParser::StatementPtr NewClassParser::addConstructorToComplexTypeDef(StatementPtr def, const Array<NamespacedIdentifier>& ids)
{
	// Do not add a constructor in a class definition
	match(JitTokens::semicolon);
	return def;
}

BlockParser::StatementPtr NewClassParser::parseStatement()
{
	if (auto noop = parseVisibility())
		return noop;

	if (matchIf(JitTokens::__internal_property))
	{
		match(JitTokens::openParen);
		
		if (currentValue.isString())
		{

			Identifier id(currentValue.toString());
			skip();

			match(JitTokens::comma);

			var v;

			if (matchIf(JitTokens::identifier))
			{
				v = currentValue;
			}
			else
			{
				auto value = parseConstExpression(false);
				v = value.toInt();
			}

			match(JitTokens::closeParen);

			return matchSemicolonAndReturn(new Operations::InternalProperty(location, id, v));
		}
	}

	if (matchIf(JitTokens::template_))
		templateArguments = parseTemplateParameters(true);
	else
		templateArguments = {};

	if (matchIf(JitTokens::namespace_))
	{
		CommentAttacher ca(*this);

		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, parseIdentifier());

		auto startPos = location.getXYPosition();

		match(JitTokens::openBrace);

		auto sb = new Operations::StatementBlock(location, compiler->namespaceHandler.getCurrentNamespaceIdentifier());

		while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
		{
			CommentAttacher ca(*this);
			auto p = parseStatement();
			sb->addStatement(ca.withComment(p));
		}

		auto endPos = location.getXYPosition();
		match(JitTokens::closeBrace);

		compiler->namespaceHandler.setNamespacePosition(compiler->namespaceHandler.getCurrentNamespaceIdentifier(), startPos, endPos, ca.getInfo());
		
		

		return sb;
	}
	
	if (matchIf(JitTokens::using_))
	{
		parseUsingAlias();
		return new Operations::Noop(location);
	}

	if (matchIf(JitTokens::struct_))
	{
		return parseSubclass(NamespaceHandler::Visibility::Public);
	}

	if (matchIf(JitTokens::class_))
	{
		return parseSubclass(NamespaceHandler::Visibility::Private);
	}

	if (matchIfType(templateArguments))
	{
		if (currentTypeInfo.isComplexType())
			return parseComplexTypeDefinition();
		else
			return parseVariableDefinition();
	}

	location.throwError("Can't parse statement");
	return nullptr;
}


BlockParser::StatementPtr NewClassParser::parseVariableDefinition()
{
	auto s = parseNewSymbol(NamespaceHandler::Variable);

	if (matchIf(JitTokens::openParen))
	{
		if (!compiler->namespaceHandler.changeSymbolType(s.id, NamespaceHandler::Function))
			location.throwError("Can't find function");

		auto st = parseFunction(s);
		return matchIfSemicolonAndReturn(st);
	}
	else
	{
		if (s.typeInfo.isStatic())
		{
			if (!s.typeInfo.isConst())
				location.throwError("Can't define non-const static variables");

			compiler->namespaceHandler.changeSymbolType(s.id, NamespaceHandler::Constant);

			match(JitTokens::assign_);

			auto v = parseConstExpression(false);
			compiler->namespaceHandler.addConstant(s.id, v);

			if (v.getType() == Types::ID::Integer)
			{
				return matchSemicolonAndReturn(new Operations::InternalProperty(location, s.id.getIdentifier(), v.toInt()));
			}
			else
			{
				return matchSemicolonAndReturn(new Operations::Noop(location));
			}
		}

		if (matchIf(JitTokens::assign_))
		{
			auto target = new Operations::VariableReference(location, s);

			ExprPtr expr;

			expr = new Operations::Immediate(location, parseConstExpression(false));

			return matchSemicolonAndReturn(new Operations::Assignment(location, target, JitTokens::assign_, expr, true));
		}

		if (!s.typeInfo.isTemplateType())
		{
			location.throwError("Expected initialiser for non-templated member");
		}
		else
		{
			return matchSemicolonAndReturn(new Operations::ComplexTypeDefinition(location, s.id, s.typeInfo));
		}
	}
}

BlockParser::StatementPtr NewClassParser::parseFunction(const Symbol& s)
{
	using namespace Operations;

	FunctionDefinitionBase* func;

	bool isTemplateFunction = !templateArguments.isEmpty();

	if (isTemplateFunction)
		func = new TemplatedFunction(location, s, templateArguments);
	else
	{
		func = new Function(location, s);
		
	}
		
	

	StatementPtr newStatement = dynamic_cast<Statement*>(func);

	auto& fData = func->data;

	fData.id = func->data.id;
	fData.returnType = currentTypeInfo;
	fData.object = nullptr;
	CommentAttacher ca(*this);
	ca.withComment(fData);
	
	if (FunctionClass::isConstructor(fData.id))
	{
		fData.returnType = TypeInfo(Types::ID::Void);
	}

	jassert(compiler->namespaceHandler.getCurrentNamespaceIdentifier() == s.id.getParent());

	{
		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, s.id);

		if (isTemplateFunction)
		{
			registerTemplateArguments(as<TemplatedFunction>(newStatement)->templateParameters, s.id);
			templateArguments = as<TemplatedFunction>(newStatement)->templateParameters;
		}
			 

		while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
		{
			matchType(templateArguments);

			auto s = parseNewSymbol(NamespaceHandler::Variable);
			fData.args.add(s);
			func->parameters.add(s.id.id);

			matchIf(JitTokens::comma);
		}
	}

	if (isTemplateFunction)
	{
		TemplateObject f({ s.id, compiler->namespaceHandler.getCurrentTemplateParameters() });
		f.argList = as<TemplatedFunction>(newStatement)->templateParameters;
		f.makeFunction = std::bind(&TemplatedFunction::createFunction, as<TemplatedFunction>(newStatement), std::placeholders::_1);

		TypeInfo::List callParameters;

		for (auto& a : fData.args)
			callParameters.add(a.typeInfo);

		f.functionArgs = [callParameters]()
		{
			return callParameters;
		};

		compiler->namespaceHandler.addTemplateFunction(f);
	}
	else
	{
		compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Function, ca.getInfo());
		compiler->namespaceHandler.setSymbolCode(s.id, fData.getCodeToInsert());
	}

	match(JitTokens::closeParen);

	bool isConstFunction = matchIf(JitTokens::const_);

	func->data.setConst(isConstFunction);

	func->code = location.location;

	auto startPos = location.getXYPosition();

	match(JitTokens::openBrace);
	int numOpenBraces = 1;

	while (currentType != JitTokens::eof && numOpenBraces > 0)
	{
		if (currentType == JitTokens::openBrace) numOpenBraces++;
		if (currentType == JitTokens::closeBrace) numOpenBraces--;
		skip();
	}

	auto endPos = location.getXYPosition();

	compiler->namespaceHandler.setNamespacePosition(s.id, startPos, endPos, ca.getInfo());
	
	func->codeLength = static_cast<int>(location.location - func->code);
	
	

	return matchIfSemicolonAndReturn(newStatement);
}


BlockParser::StatementPtr NewClassParser::parseSubclass(NamespaceHandler::Visibility defaultVisibility)
{
	NamespaceHandler::ScopedVisibilityState vs(compiler->namespaceHandler);

	auto startPos = location.getXYPosition();

	SymbolParser sp(*this, compiler->namespaceHandler);

	sp.parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();

	auto classId = sp.currentNamespacedIdentifier;

	

	if (templateArguments.isEmpty())
	{
		auto p = new StructType(classId, templateArguments);

		CommentAttacher ca(*this);

		compiler->namespaceHandler.addSymbol(classId, TypeInfo(p), NamespaceHandler::Struct, ca.getInfo());
		compiler->namespaceHandler.registerComplexTypeOrReturnExisting(p);

		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classId);
		compiler->namespaceHandler.setVisiblity(defaultVisibility);

		auto list = parseStatementList();

		compiler->namespaceHandler.setNamespacePosition(classId, startPos, location.getXYPosition(), ca.getInfo());

		return matchSemicolonAndReturn(new Operations::ClassStatement(location, p, list));
	}
	else
	{
		auto classTemplateArguments = templateArguments;



		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classId);
		

		registerTemplateArguments(classTemplateArguments, classId);
		
		StatementPtr list;

		CommentAttacher ca(*this);

		{
			NamespaceHandler::ScopedVisibilityState vs(compiler->namespaceHandler);
			compiler->namespaceHandler.setVisiblity(defaultVisibility);
			list = ca.withComment(parseStatementList());
		}
		
		while(currentType == JitTokens::semicolon)
			match(JitTokens::semicolon);

		TemplateInstance cId(classId, compiler->namespaceHandler.getCurrentTemplateParameters());

		auto tcs = new Operations::TemplateDefinition(location, cId, compiler->namespaceHandler, list);

		TemplateObject tc(cId);
		tc.makeClassType = std::bind(&Operations::TemplateDefinition::createTemplate, tcs, std::placeholders::_1);
		tc.argList = classTemplateArguments;

		compiler->namespaceHandler.setNamespacePosition(classId, startPos, location.getXYPosition(), ca.getInfo());

		compiler->namespaceHandler.addTemplateClass(tc);
		return tcs;
	}
}

NewClassParser::StatementPtr NewClassParser::parseVisibility()
{
	if (matchIf(JitTokens::public_))
		compiler->namespaceHandler.setVisiblity(NamespaceHandler::Visibility::Public);
	else if (matchIf(JitTokens::private_))
		compiler->namespaceHandler.setVisiblity(NamespaceHandler::Visibility::Private);
	else if (matchIf(JitTokens::protected_))
		compiler->namespaceHandler.setVisiblity(NamespaceHandler::Visibility::Protected);
	else
		return nullptr;

	match(JitTokens::colon);
	return new Operations::Noop(location);
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

	bool parseNextParameter = true;

	while (currentType != JitTokens::greaterThan && !isEOF())
	{
		TemplateParameter::ParameterType parameterType;

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

snex::jit::BlockParser::StatementPtr BlockParser::parseComplexTypeDefinition()
{
	jassert(getCurrentComplexType() != nullptr);

	Array<NamespacedIdentifier> ids;

	auto t = currentTypeInfo;

	auto typePtr = getCurrentComplexType();
	auto rootId = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

	bool isConstructor = rootId.toString() == typePtr->toString();

	if (isConstructor)
		ids.add(rootId.getChildId(FunctionClass::getSpecialSymbol(rootId, FunctionClass::Constructor)));
	else
		ids.add(rootId.getChildId(parseIdentifier()));

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

		auto n = new Operations::ComplexTypeDefinition(location, ids, currentTypeInfo);

		CommentAttacher ca(*this);

		for (auto id : ids)
			compiler->namespaceHandler.addSymbol(id, currentTypeInfo, NamespaceHandler::Variable, ca.getInfo());

		if (matchIf(JitTokens::assign_))
		{
			if (currentType == JitTokens::openBrace)
				n->addInitValues(parseInitialiserList());
			else
				n->addStatement(parseExpression());
		}

		return addConstructorToComplexTypeDef(n, ids);
	}
}

snex::jit::BlockParser::StatementPtr BlockParser::addConstructorToComplexTypeDef(StatementPtr def, const Array<NamespacedIdentifier>& ids)
{
	auto n = Operations::as<Operations::ComplexTypeDefinition>(def);

	if (currentTypeInfo.isComplexType() && currentTypeInfo.getComplexType()->hasConstructor())
	{
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

			auto cf = fc->getConstructor(n->initValues);

			if (!cf.id.isValid())
				location.throwError("Can't find constructor that matches init values " + n->initValues->toString());
			
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

			
			ab->addStatement(call);
		}

		match(JitTokens::semicolon);
		return ab;
	}

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
				parentInstanceParameters = st->getTemplateInstanceParameters();
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

juce::Array<snex::jit::TemplateParameter> TypeParser::parseTemplateParameters()
{
	Array<TemplateParameter> parameters;

	match(JitTokens::lessThan);

	bool parseNextParameter = true;

	while (currentType != JitTokens::greaterThan && !isEOF())
	{
		TypeParser tp(*this, namespaceHandler, {});

		if (tp.matchIfType())
		{


			TemplateParameter p(tp.currentTypeInfo);
			
			
			parameters.add(p);
		}
		else
		{
			if (currentType == JitTokens::identifier)
			{
				auto cId = namespaceHandler.getCurrentNamespaceIdentifier().getChildId(Identifier(currentValue.toString()));

				if (namespaceHandler.isTemplateConstantArgument(cId))
				{
					TypeInfo tti(cId);

					TemplateParameter tp(tti);
					parameters.add(tp);

					match(JitTokens::identifier);
					matchIf(JitTokens::comma);
					continue;
				}

			}

			auto e = parseConstExpression(true);

			if (e.getType() != Types::ID::Integer)
				location.throwError("Can't use non-integers as template argument");

			TemplateParameter tp(e.toInt());

			parameters.add(tp);
		}
		
		matchIf(JitTokens::comma);
	}

	match(JitTokens::greaterThan);

	return parameters;
}

snex::VariableStorage TypeParser::parseConstExpression(bool canBeTemplateParameter)
{
	if (currentType == JitTokens::identifier)
	{
		SymbolParser sp(*this, namespaceHandler);
		auto id = sp.parseExistingSymbol(true);
		return namespaceHandler.getConstantValue(id.id);
	}
	
	return parseVariableStorageLiteral();
}

snex::jit::Symbol SymbolParser::parseNewSymbol(NamespaceHandler::SymbolType t)
{
	auto type = other.currentTypeInfo;

	parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();

	auto s = Symbol(currentNamespacedIdentifier, type);

	BlockParser::CommentAttacher ca(*this);

	if (t != NamespaceHandler::Unknown)
		handler.addSymbol(s.id, type, t,ca.getInfo());

	if (s.typeInfo.isDynamic() && t != NamespaceHandler::UsingAlias)
		location.throwError("Can't resolve symbol type");

	return s;
}

}
}


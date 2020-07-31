namespace snex {
namespace jit {
using namespace juce;

int Compiler::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	auto c = source.nextChar();

	if (c == 'P')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::PassMessage;
	}
	if (c == 'W')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Warning;
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
					addOptimization(f.createOptimization(id));
			}
		}

		newScope = new JitCompiledFunctionClass(parentScope, classInstanceId);
	};

	~ClassCompiler()
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
			executePass(SyntaxSugarReplacements, newScope->pimpl, sTree);
			executePass(PostSymbolOptimization, newScope->pimpl, sTree);
			executePass(FunctionParsing, newScope->pimpl, sTree);

			// Optimize now

			executePass(FunctionCompilation, newScope->pimpl, sTree);

			lastResult = Result::ok();
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
		while (matchIf(JitTokens::using_))
		{
			parseUsingAlias();
		}

		auto s = parseStatement();
		list->addStatement(s);

		while (matchIf(JitTokens::using_))
		{
			parseUsingAlias();
		}
	}

	matchIf(JitTokens::closeBrace);

	finaliseSyntaxTree(list);

	return p;
}



BlockParser::StatementPtr NewClassParser::parseStatement()
{
	if (matchIf(JitTokens::namespace_))
	{
		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, parseIdentifier());

		match(JitTokens::openBrace);

		auto sb = new Operations::StatementBlock(location, compiler->namespaceHandler.getCurrentNamespaceIdentifier());

		while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
		{
			sb->addStatement(parseStatement());
		}

		match(JitTokens::closeBrace);

		return sb;
	}
	
	if (matchIf(JitTokens::using_))
	{
		parseUsingAlias();
		return new Operations::Noop(location);
	}

	if (matchIf(JitTokens::struct_))
	{
		return parseSubclass();
	}

	if (matchIf(JitTokens::static_))
	{
		if (!matchIfType())
			location.throwError("Expected type");

		if (!currentTypeInfo.isConst())
			location.throwError("Can't define non-const static variables");

		auto s = parseNewSymbol(NamespaceHandler::Constant);

		match(JitTokens::assign_);

		auto v = parseConstExpression(false);

		compiler->namespaceHandler.addConstant(s.id, v);

		match(JitTokens::semicolon);
		return new Operations::Noop(location);
	}

	if (matchIfType())
	{
		if (currentTypeInfo.isComplexType())
			return parseComplexTypeDefinition();
		else
			return parseVariableDefinition();
	}

	location.throwError("Can't parse statement");
	return nullptr;
}


BlockParser::StatementPtr NewClassParser::parseDefinition()
{
	StatementPtr s;

	if (matchIf(JitTokens::openParen))
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding function " + getCurrentSymbol().toString());
		s = parseFunction(getCurrentSymbol());
		matchIf(JitTokens::semicolon);
	}
	else
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding variable " + getCurrentSymbol().toString());
		s = parseVariableDefinition();
		match(JitTokens::semicolon);
	}

	return s;
}

snex::jit::BlockParser::ExprPtr NewClassParser::parseBufferInitialiser()
{
	if (auto cc = dynamic_cast<ClassCompiler*>(compiler.get()))
	{
		auto& handler = cc->parentScope->getGlobalScope()->getBufferHandler();
		auto id = parseIdentifier();

		if (id == Identifier("Buffer"))
		{
			match(JitTokens::dot);

			auto function = parseIdentifier();
			match(JitTokens::openParen);

			auto value = parseVariableStorageLiteral().toInt();
			match(JitTokens::closeParen);

			try
			{
				throw juce::String("Invalid buffer function");
#if 0
				if (function.toString() == "create")
				{
					return new Operations::Immediate(location, handler.create(getCurrentSymbol(false).getName(), value));
				}
				else if (function.toString() == "getAudioFile")
				{
					return new Operations::Immediate(location, handler.getAudioFile(value, getCurrentSymbol(false).id));
				}
				else if (function.toString() == "getTable")
				{
					return new Operations::Immediate(location, handler.getTable(value, getCurrentSymbol(false).id));
				}
				else
				{
					
				}
#endif
			}
			catch (juce::String& s)
			{
				location.throwError(s);
			}
		}
		else
			location.throwError("Expected Buffer function");
	}
}

BlockParser::StatementPtr NewClassParser::parseVariableDefinition()
{
	auto s = parseNewSymbol(NamespaceHandler::Variable);

	if (matchIf(JitTokens::openParen))
	{
		if (!compiler->namespaceHandler.changeSymbolType(s.id, NamespaceHandler::Function))
			location.throwError("Can't find function");

		auto st = parseFunction(s);

		matchIf(JitTokens::semicolon);

		return st;
	}
	else
	{
		auto target = new Operations::VariableReference(location, s);

		match(JitTokens::assign_);

		ExprPtr expr;

		expr = new Operations::Immediate(location, parseVariableStorageLiteral());
		match(JitTokens::semicolon);

		return new Operations::Assignment(location, target, JitTokens::assign_, expr, true);
	}
}

BlockParser::StatementPtr NewClassParser::parseFunction(const Symbol& s)
{
	auto func = new Operations::Function(location, s);

	StatementPtr newStatement = func;

	auto& fData = func->data;

	fData.id = func->id.id;
	fData.returnType = currentTypeInfo;
	fData.object = nullptr;

	jassert(compiler->namespaceHandler.getCurrentNamespaceIdentifier() == s.id.getParent());

	{
		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, s.id);

		while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
		{
			matchType();

			auto s = parseNewSymbol(NamespaceHandler::Variable);
			fData.args.add(s);
			func->parameters.add(s.id.id);

			matchIf(JitTokens::comma);
		}
	}

	compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Function);

	match(JitTokens::closeParen);

	func->code = location.location;

	match(JitTokens::openBrace);
	int numOpenBraces = 1;

	while (currentType != JitTokens::eof && numOpenBraces > 0)
	{
		if (currentType == JitTokens::openBrace) numOpenBraces++;
		if (currentType == JitTokens::closeBrace) numOpenBraces--;
		skip();
	}

	
	func->codeLength = static_cast<int>(location.location - func->code);
	
	return newStatement;
}

BlockParser::StatementPtr NewClassParser::parseSubclass()
{
	SymbolParser sp(*this, compiler->namespaceHandler);

	sp.parseNamespacedIdentifier<NamespaceResolver::MustBeNew>();

	auto classId = sp.currentNamespacedIdentifier;

	auto p = new StructType(classId);

	compiler->namespaceHandler.addSymbol(classId, TypeInfo(p), NamespaceHandler::Struct);
	compiler->namespaceHandler.registerComplexTypeOrReturnExisting(p);

	NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, classId);

	auto list = parseStatementList();

	match(JitTokens::semicolon);

	return new Operations::ClassStatement(location, p, list);
}

juce::Array<snex::jit::TemplateParameter> BlockParser::parseTemplateParameters()
{
	Array<TemplateParameter> parameters;

	match(JitTokens::lessThan);

	while (currentType != JitTokens::greaterThan)
	{
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
		else if (matchIfType())
		{
			TemplateParameter tp;
			tp.type = currentTypeInfo;
			parameters.add(tp);
		}
		else
			location.throwError("Invalid template parameter: " + juce::String(currentType));

		matchIf(JitTokens::comma);
	}

	match(JitTokens::greaterThan);

	return parameters;
}

snex::jit::BlockParser::StatementPtr BlockParser::parseComplexTypeDefinition()
{
	jassert(getCurrentComplexType() != nullptr);

	Array<NamespacedIdentifier> ids;

	auto typePtr = getCurrentComplexType();
	auto rootId = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

	ids.add(rootId.getChildId(parseIdentifier()));
	
	while (matchIf(JitTokens::comma))
		ids.add(rootId.getChildId(parseIdentifier()));

	auto n = new Operations::ComplexTypeDefinition(location, ids, currentTypeInfo);

	for (auto id : ids)
		compiler->namespaceHandler.addSymbol(id, currentTypeInfo, NamespaceHandler::Variable);

	if (matchIf(JitTokens::assign_))
	{
		if (currentType == JitTokens::openBrace)
			n->initValues = parseInitialiserList();
		else
			n->addStatement(parseExpression());
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
			root->addImmediateValue(parseConstExpression(false));

		next = matchIf(JitTokens::comma);
	}

	match(JitTokens::closeBrace);

	return root;
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

		if (!matchIfType())
			location.throwError("Expected type");

		if (currentTypeInfo.isComplexType())
			currentTypeInfo.getComplexType()->setAlias(s.id);

		s.typeInfo = currentTypeInfo;
		match(JitTokens::semicolon);
		compiler->namespaceHandler.setTypeInfo(s.id, NamespaceHandler::UsingAlias, s.typeInfo);
	}
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

}
}


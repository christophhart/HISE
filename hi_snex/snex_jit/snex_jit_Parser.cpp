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

	ClassCompiler(BaseScope* parentScope_, const Symbol& classInstanceId=Symbol()) :
		BaseCompiler(),
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
		NewClassParser parser(this, classStart, length, instanceId);

		if (newScope == nullptr)
			newScope = new JitCompiledFunctionClass(parentScope, instanceId);
		try
		{
			parser.currentScope = newScope->pimpl;

			setCurrentPass(BaseCompiler::Parsing);
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
	Symbol instanceId;

	Operations::Statement::Ptr syntaxTree;
};


BlockParser::StatementPtr BlockParser::parseStatementList()
{
	matchIf(JitTokens::openBrace);

	auto list = new SyntaxTree(location);

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
		pushNamespace(parseIdentifier());
		match(JitTokens::openBrace);

		auto sb = new Operations::StatementBlock(location);

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

	bool isStatic = matchIf(JitTokens::static_);
	bool isConst = matchIf(JitTokens::const_);

	if (isStatic && !isConst)
		location.throwError("Can't define non-const static variables");

	
	if (isStatic)
	{
		currentHnodeType = matchType();

		matchSymbol();
		auto s = getCurrentSymbol(true);

		match(JitTokens::assign_);
		currentScope->addConstant(s.id, parseConstExpression(false));

		match(JitTokens::semicolon);
		return parseStatement();
	}

	parseNamespacePrefix();

	if (matchIfSimpleType())
	{
		auto s = parseVariableDefinition(isConst);
		return s;
	}

	if (matchIfComplexType())
	{

		return parseComplexTypeDefinition(isConst);
	}

	

	location.throwError("Can't parse statement");
	return nullptr;
}


BlockParser::StatementPtr NewClassParser::parseDefinition(bool isConst, Types::ID type, bool isWrappedBuffer, bool isSmoothedVariable)
{
	StatementPtr s;
	currentHnodeType = type;

	if (matchIf(JitTokens::openParen))
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding function " + getCurrentSymbol(true).toString());
		s = parseFunction(getCurrentSymbol(true));
		matchIf(JitTokens::semicolon);
	}
	else
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding variable " + getCurrentSymbol(true).toString());
		s = parseVariableDefinition(isConst);
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
				if (function.toString() == "create")
				{
					return new Operations::Immediate(location, handler.create(getCurrentSymbol(false).id, value));
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
					throw juce::String("Invalid buffer function");
				}
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

BlockParser::StatementPtr NewClassParser::parseVariableDefinition(bool /*isConst*/)
{
	auto type = currentHnodeType;

	clearSymbol();
	matchSymbol();

	auto s = getCurrentSymbol(true);

	if (matchIf(JitTokens::openParen))
	{
		auto st = parseFunction(s);

		matchIf(JitTokens::semicolon);

		return st;
	}
	else
	{
		auto target = new Operations::VariableReference(location, s);

		match(JitTokens::assign_);

		ExprPtr expr;

		if (type == Types::ID::Block)
			expr = parseBufferInitialiser();
		else
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
	fData.returnType = TypeInfo(currentHnodeType);
	fData.object = nullptr;

	while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
	{
		TypeInfo t;

		Types::ID type;
		ComplexType::Ptr typePtr;

		bool isConst = matchIf(JitTokens::const_);

		if (matchIfSimpleType())
		{
			type = currentHnodeType;
		}
		else if (matchIfComplexType())
		{
			typePtr = getCurrentComplexType();
			type = Types::ID::Pointer;
		}

		bool isRef = matchIf(JitTokens::bitwiseAnd);

		if (typePtr != nullptr)
			t = TypeInfo(typePtr, isConst, isRef);
		else
			t = TypeInfo(type, isConst, isRef);

		auto id = parseIdentifier();

		auto s = Symbol({ id }, t);

		fData.args.add(s);
		func->parameters.add(id);
		
		matchIf(JitTokens::comma);
	}

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
	matchSymbol();

	auto classSymbol = getCurrentSymbol(true);

	auto p = new StructType(classSymbol);

	compiler->complexTypes.add(p);

	ScopedChildRoot scs(*this, classSymbol.id);

	auto list = parseStatementList();

	match(JitTokens::semicolon);

	return new Operations::ClassStatement(location, p, list);

#if 0
	auto c = new Operations::ClassStatement(location, classId);
	auto classStart = location.location;

	match(JitTokens::openBrace);
	int numOpenBraces = 1;

	while (currentType != JitTokens::eof && numOpenBraces > 0)
	{
		if (currentType == JitTokens::openBrace) numOpenBraces++;
		if (currentType == JitTokens::closeBrace) numOpenBraces--;
		skip();
	}

	match(JitTokens::semicolon);

	c->endLocation = location;
	return c;
#endif
}

#if 0
snex::jit::BlockParser::StatementPtr NewClassParser::parseWrapDefinition()
{
	match(JitTokens::lessThan);

	auto size = parseConstExpression(true).toInt();
	auto wt = compiler->getWrapType(size);

	if (wt == nullptr)
	{
		wt = new WrapType(size);
		compiler->complexTypes.add(wt);
	}
	
	match(JitTokens::greaterThan);

	matchSymbol();

	auto s = getCurrentSymbol(true).withComplexType(wt);
	s.type = Types::ID::Integer;

	InitialiserList::Ptr l;

	if (matchIf(JitTokens::assign_))
	{
		l = parseInitialiserList();
	}

	

	match(JitTokens::semicolon);

	return new Operations::WrapDefinition(location, s, l);
}
#endif

snex::VariableStorage BlockParser::parseVariableStorageLiteral()
{
	bool isMinus = matchIf(JitTokens::minus);

	auto type = Types::Helpers::getTypeFromStringValue(currentString);

	juce::String stringValue = currentString;
	match(JitTokens::literal);

	if (type == Types::ID::Integer)
		return VariableStorage(stringValue.getIntValue() * (!isMinus ? 1 : -1));
	else if (type == Types::ID::Float)
		return VariableStorage(stringValue.getFloatValue() * (!isMinus ? 1.0f : -1.0f));
	else if (type == Types::ID::Double)
		return VariableStorage(stringValue.getDoubleValue() * (!isMinus ? 1.0 : -1.0));
	else if (type == Types::ID::Event)
    {
        HiseEvent e;
		return VariableStorage(e);
    }
	else if (type == Types::ID::Block)
		return block();
	else
		return {};
}

snex::VariableStorage BlockParser::parseConstExpression(bool isTemplateArgument)
{
	ScopedTemplateArgParser s(*this, isTemplateArgument);

	auto expr = parseExpression();

	SyntaxTree bl(location);
	bl.addStatement(expr);


	BaseCompiler::ScopedPassSwitcher sp1(compiler, BaseCompiler::DataAllocation);
	compiler->executePass(BaseCompiler::DataAllocation, currentScope, &bl);

	BaseCompiler::ScopedPassSwitcher sp2(compiler, BaseCompiler::PreSymbolOptimization);
	compiler->optimize(expr, currentScope, false);

	BaseCompiler::ScopedPassSwitcher sp3(compiler, BaseCompiler::ResolvingSymbols);
	compiler->executePass(BaseCompiler::ResolvingSymbols, currentScope, &bl);


	BaseCompiler::ScopedPassSwitcher sp4(compiler, BaseCompiler::PostSymbolOptimization);
	compiler->optimize(expr, currentScope, false);

	expr = dynamic_cast<Operations::Expression*>(bl.getChildStatement(0).get());

	if (!expr->isConstExpr())
		location.throwError("Can't assign static constant to a dynamic expression");

	return expr->getConstExprValue();
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
		}
		else if (matchIfSimpleType())
		{
			TemplateParameter tp;
			tp.type = TypeInfo(currentHnodeType);
			parameters.add(tp);
		}
		else if (matchIfComplexType())
		{
			TemplateParameter tp;
			tp.type = TypeInfo(currentComplexType);
			parameters.add(tp);
		}
		else
			location.throwError("Invalid template parameter: " + juce::String(currentType));

		matchIf(JitTokens::comma);
	}

	match(JitTokens::greaterThan);

	return parameters;
}

snex::jit::BlockParser::StatementPtr BlockParser::parseComplexTypeDefinition(bool isConst)
{
	jassert(getCurrentComplexType() != nullptr);

	Array<Identifier> ids;

	auto typePtr = getCurrentComplexType();

	ids.add(parseIdentifier());
	
	while (matchIf(JitTokens::comma))
		ids.add(parseIdentifier());

	auto n = new Operations::ComplexTypeDefinition(location, ids, TypeInfo(typePtr, isConst));

	if (matchIf(JitTokens::assign_))
	{
		if (currentType == JitTokens::openBrace)
		{
			n->initValues = parseInitialiserList();
		}
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

ComplexType::Ptr BlockParser::parseComplexType(const juce::String& token)
{
	match(JitTokens::lessThan);

	ComplexType::Ptr newType;

	if (token == JitTokens::span_)
	{
		Types::ID type = Types::ID::Dynamic;
		ComplexType::Ptr child;

		if (currentType == JitTokens::identifier)
		{
			matchSymbol();
			auto classSymbol = getCurrentSymbol(false);

			auto cs = getCurrentScopeStatement();

			if (cs = cs->getScopedStatementForAlias(classSymbol.id))
			{
				child = cs->getAliasComplexType(classSymbol.id);

				if (child == nullptr)
					type = cs->getAliasNativeType(classSymbol.id);
			}
			else
			{
				type = Types::ID::Pointer;

				for (auto ct : compiler->complexTypes)
				{
					if (auto st = dynamic_cast<StructType*>(ct))
					{
						if (st->id == classSymbol)
						{
							child = st;
							break;
						}
					}
				}

				if (child == nullptr)
					location.throwError("Undefined type " + classSymbol.toString());
			}
		}
		else
		{
			if (currentType == JitTokens::span_ || currentType == JitTokens::wrap || currentType == JitTokens::dyn_)
			{
				type = Types::ID::Pointer;
				auto pt = currentType;
				skip();
				child = parseComplexType(pt);
			}
			else
				type = matchType();
		}

		match(JitTokens::comma);

		auto sizeValue = parseConstExpression(true);

		int size = (int)sizeValue;

		int spanLimit = 1024 * 1024;

		if (size > spanLimit)
			location.throwError("Span size can't exceed 1M");

		match(JitTokens::greaterThan);

		if (child != nullptr)
			newType = new SpanType(child, size);
		else
			newType = new SpanType(type, size);
	}
	else if(token == JitTokens::wrap)
	{
		auto sizeValue = parseConstExpression(true);

		int size = (int)sizeValue;

		int spanLimit = 1024 * 1024;

		if (size > spanLimit)
			location.throwError("Span size can't exceed 1M");

		newType = new WrapType(size);

		match(JitTokens::greaterThan);
	}
	else if (token == JitTokens::dyn_)
	{
		TypeInfo t;

		auto isConst = matchIf(JitTokens::const_);

		if (matchIfSimpleType())
		{
			t = TypeInfo(currentHnodeType, isConst);
		}
		else if (matchIfComplexType())
		{
			t = TypeInfo(getCurrentComplexType(), isConst);
		}

		newType = new DynType(t);

		match(JitTokens::greaterThan);
	}

	compiler->complexTypes.addIfNotAlreadyThere(newType);

	return newType;
}

void BlockParser::parseUsingAlias()
{
	auto s = Symbol::createRootSymbol(parseIdentifier());

	match(JitTokens::assign_);

	if (matchIf(JitTokens::span_))
	{
		auto cType = parseComplexType(JitTokens::span_);
		cType->setAlias(s.id);
		s = s.withComplexType(cType);
	}
	else if (matchIfSimpleType())
	{
		s.typeInfo.setType(currentHnodeType);
	}
	else if (currentType == JitTokens::identifier)
	{
		if (auto c = compiler->getComplexTypeForAlias(s.id))
		{
			s = s.withComplexType(c);
			c->setAlias(s.id);
		}
		else if (auto c = compiler->getStructType(Symbol::createRootSymbol(parseIdentifier())))
		{
			s = s.withComplexType(c);
			c->setAlias(s.id);
		}
	}

	match(JitTokens::semicolon);

	s.namespaces.addArray(getCurrentNamespaceList());

	DBG(s.toString());

	getCurrentScopeStatement()->addAlias(s);
}

}
}


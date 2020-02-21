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

			executePass(ComplexTypeParsing, newScope->pimpl, syntaxTree);
			executePass(DataSizeCalculation, newScope->pimpl, syntaxTree);

			newScope->pimpl->getRootData()->finalise();

			auto d = (int*)newScope->pimpl->getRootData()->data.get();

			executePass(DataAllocation, newScope->pimpl, syntaxTree);
			executePass(DataInitialisation, newScope->pimpl, syntaxTree);
			executePass(PreSymbolOptimization, newScope->pimpl, syntaxTree);
			executePass(ResolvingSymbols, newScope->pimpl, syntaxTree);
			executePass(TypeCheck, newScope->pimpl, syntaxTree);
			executePass(SyntaxSugarReplacements, newScope->pimpl, syntaxTree);
			executePass(PostSymbolOptimization, newScope->pimpl, syntaxTree);
			executePass(FunctionParsing, newScope->pimpl, syntaxTree);

			// Optimize now

			executePass(FunctionCompilation, newScope->pimpl, syntaxTree);

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

	ScopedPointer<SyntaxTree> syntaxTree;
};


SyntaxTree* BlockParser::parseStatementList()
{
	matchIf(JitTokens::openBrace);

	ScopedPointer<SyntaxTree> list = new SyntaxTree(location);

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
	}

	matchIf(JitTokens::closeBrace);

	finaliseSyntaxTree(list);

	return list.release();
}



BlockParser::StatementPtr NewClassParser::parseStatement()
{
	if (matchIf(JitTokens::struct_))
	{
		return parseSubclass();
	}

	if (matchIf(JitTokens::wrap))
	{
		return parseWrapDefinition();
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

	if (currentType == JitTokens::identifier)
	{
		auto loc = location;

		matchSymbol();
		auto classId = getCurrentSymbol(true);

		auto cs = getCurrentScopeStatement();
		jassert(cs != nullptr);

		if (cs = cs->getScopedStatementForAlias(classId.id))
		{
			if (auto cPtr = cs->getAliasComplexType(classId.id))
				return parseComplexTypeDefinition(cPtr);
			else
			{
				matchSymbol();
				return parseDefinition(isConst, cs->getAliasNativeType(classId.id), false, false);
			}
				
		}
		else
			return parseComplexTypeDefinition(compiler->getComplexTypeForAlias(classId.id));
	}

	bool isSmoothedType = isSmoothedVariableType();
	bool isWrappedBlType = isWrappedBlockType();

	currentHnodeType = matchType();
	
	if (currentHnodeType == Types::ID::Pointer)
	{
		auto spanTypeSymbol = parseSpanType();

		return parseComplexTypeDefinition(spanTypeSymbol);

#if 0
		matchSymbol();

		InitialiserList::Ptr p;

		if(matchIf(JitTokens::assign_))
		{
			p = parseInitialiserList();
		}

		match(JitTokens::semicolon);

		return new Operations::SpanDefinition(location, spanTypeSymbol, getCurrentSymbol(true), p);
#endif
	}

	clearSymbol();
	addSymbolChild(parseIdentifier());

	return parseDefinition(isConst, currentHnodeType, isWrappedBlType, isSmoothedType);

	
}


BlockParser::StatementPtr NewClassParser::parseDefinition(bool isConst, Types::ID type, bool isWrappedBuffer, bool isSmoothedVariable)
{
	StatementPtr s;
	currentHnodeType = type;

	if (matchIf(JitTokens::openParen))
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding function " + getCurrentSymbol(true).toString());
		s = parseFunction();
		matchIf(JitTokens::semicolon);
	}
	else
	{
		if (isSmoothedVariable)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding smoothed variable " + getCurrentSymbol(true).toString());
			s = parseSmoothedVariableDefinition();
			match(JitTokens::semicolon);
		}
		else if (isWrappedBuffer)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding wrapped block " + getCurrentSymbol(true).toString());
			s = parseWrappedBlockDefinition();
			match(JitTokens::semicolon);
		}
		else
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding variable " + getCurrentSymbol(true).toString());
			s = parseVariableDefinition(isConst);
			match(JitTokens::semicolon);
		}

	}

	return s;
}



snex::jit::BlockParser::StatementPtr NewClassParser::parseSmoothedVariableDefinition()
{
	StatementPtr s;

	match(JitTokens::assign_);
	auto value = parseVariableStorageLiteral();

	return new Operations::SmoothedVariableDefinition(location, getCurrentSymbol(true), currentHnodeType, value);
}

snex::jit::BlockParser::StatementPtr NewClassParser::parseWrappedBlockDefinition()
{
	StatementPtr s;
	match(JitTokens::assign_);

	auto value = parseBufferInitialiser();

	return new Operations::WrappedBlockDefinition(location, getCurrentSymbol(true), value);
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
	

	auto target = new Operations::VariableReference(location, getCurrentSymbol(true));
	//target->isLocalToScope = true;

	

	match(JitTokens::assign_);

	ExprPtr expr;
	
	if (type == Types::ID::Block)
	{
		expr = parseBufferInitialiser();
	}
	else
	{
		expr = new Operations::Immediate(location, parseVariableStorageLiteral());
	}
	
	return new Operations::Assignment(location, target, JitTokens::assign_, expr, true);
}

BlockParser::StatementPtr NewClassParser::parseFunction()
{
	auto func = new Operations::Function(location, getCurrentSymbol(true));

	StatementPtr newStatement = func;

	auto& fData = func->data;

	fData.id = func->id.id;
	fData.returnType = currentHnodeType;
	fData.object = nullptr;

	while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
	{
		auto t = matchType();

		bool isAlias = matchIf(JitTokens::bitwiseAnd);

		fData.args.add({ t, isAlias });
		func->parameters.add(parseIdentifier());

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

snex::jit::BlockParser::StatementPtr NewClassParser::parseSubclass()
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

BlockParser::StatementPtr NewClassParser::parseComplexTypeDefinition(ComplexType::Ptr p)
{
	Symbol rootSymbol;

	if (auto cc = dynamic_cast<ClassCompiler*>(compiler.get()))
		rootSymbol = cc->instanceId;

	auto loc = location;

	Array<Symbol> instanceIds;

	instanceIds.add(rootSymbol.getChildSymbol(parseIdentifier()));

	while (matchIf(JitTokens::comma))
		instanceIds.add(rootSymbol.getChildSymbol(parseIdentifier()));

	InitialiserList::Ptr initList;

	if (matchIf(JitTokens::assign_))
	{
		initList = parseInitialiserList();
	}

	match(JitTokens::semicolon);

	if (auto st = dynamic_cast<StructType*>(p.get()))
		return new Operations::ClassInstance(loc, st->id, instanceIds, initList);
	else if (auto sp = dynamic_cast<SpanType*>(p.get()))
		return new Operations::SpanDefinition(loc, sp, instanceIds.getFirst(), initList);
    
    location.throwError("Can't deduce complex type");
    return nullptr;
}

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

SpanType::Ptr BlockParser::parseSpanType()
{
	match(JitTokens::lessThan);

	Types::ID type;
	SpanType::Ptr child;

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
		type = matchType();

		if (type == Types::ID::Pointer)
			child = parseSpanType();
	}

	match(JitTokens::comma);

	auto sizeValue = parseConstExpression(true);;

	int size = (int)sizeValue;

	if (size > 65536)
		location.throwError("Span size can't exceed 65536");

	match(JitTokens::greaterThan);

	SpanType::Ptr newType;

	if (child != nullptr)
		newType = new SpanType(child, size);
	else
		newType = new SpanType(type, size);

	compiler->complexTypes.add(newType);

	return newType;
}

void BlockParser::parseUsingAlias()
{
	auto s = Symbol::createRootSymbol(parseIdentifier());

	match(JitTokens::assign_);

	if (matchIf(JitTokens::span_))
	{
		auto cType = parseSpanType();
		cType->setAlias(s.id);
		s = s.withComplexType(cType);
	}
	else if (matchIfTypeToken())
	{
		s.type = currentHnodeType;
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

	getCurrentScopeStatement()->addAlias(s);
}

}
}


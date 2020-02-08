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

	ClassScope* compileSubClass()
	{

	}

	JitCompiledFunctionClass* compileAndGetScope(const ParserHelpers::CodeLocation& classStart, int length)
	{
		NewClassParser parser(this, classStart, length, instanceId);

		if (newScope == nullptr)
			newScope = new JitCompiledFunctionClass(parentScope, instanceId);
		try
		{
			setCurrentPass(BaseCompiler::Parsing);
			syntaxTree = parser.parseStatementList();
			executePass(SubclassCompilation, newScope->pimpl, syntaxTree);
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

	compiler->logMessage(BaseCompiler::ProcessMessage, "Parsing statements");

	ScopedPointer<SyntaxTree> list = new SyntaxTree(location);

	while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
	{
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

	if (currentType == JitTokens::identifier)
	{
		auto loc = location;
		auto classId = parseIdentifier();

		Symbol rootSymbol;

		if (auto cc = dynamic_cast<ClassCompiler*>(compiler.get()))
			rootSymbol = cc->instanceId;


		Array<Symbol> instanceIds;

		instanceIds.add(rootSymbol.getChildSymbol(parseIdentifier()));

		while (matchIf(JitTokens::comma))
		{
			instanceIds.add(rootSymbol.getChildSymbol(parseIdentifier()));
		}

		match(JitTokens::semicolon);

		return new Operations::ClassInstance(loc, classId, instanceIds);
	}

	bool isConst = matchIf(JitTokens::const_);

	if (matchIf(JitTokens::span_))
	{
		match(JitTokens::lessThan);
		
		parseDecimalLiteral();

		int size = (int)currentValue;

		if (size > 65536)
			location.throwError("Span size can't exceed 65536");

		match(JitTokens::comma);

		auto type = matchType();

		juce::String spanClassType;

		spanClassType << "span_" << size << "_" << Types::Helpers::getTypeName(type);

		jassertfalse;
	}

	bool isSmoothedType = isSmoothedVariableType();
	bool isWrappedBlType = isWrappedBlockType();

	currentHnodeType = matchType();
	
	matchIfSymbol(isConst);

	StatementPtr s;

	
	if (matchIf(JitTokens::openParen))
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding function " + getCurrentSymbol(true).toString());
		s = parseFunction();
		matchIf(JitTokens::semicolon);
	}
	else
	{
		if (isSmoothedType)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding smoothed variable " + getCurrentSymbol(true).toString());
			s = parseSmoothedVariableDefinition();
			match(JitTokens::semicolon);
		}
		else if (isWrappedBlType)
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
	
	return new Operations::Assignment(location, target, JitTokens::assign_, expr);
}

BlockParser::StatementPtr NewClassParser::parseFunction()
{
	auto func = new Operations::Function(location, getCurrentSymbol(false).id);

	StatementPtr newStatement = func;

	auto& fData = func->data;

	fData.id = getCurrentSymbol(false).id;
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
	auto classId = parseIdentifier();

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



}
}


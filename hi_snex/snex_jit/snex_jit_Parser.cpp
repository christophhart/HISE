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


class ClassScope : public FunctionClass,
				   public BaseScope
{
public:

	

	ClassScope(GlobalScope& parent) :
		FunctionClass({}),
		BaseScope({}, &parent)
	{
		jassert(scopeType == BaseScope::Class);

		runtime = new asmjit::JitRuntime();

		addFunctionClass(new MessageFunctions());
		addFunctionClass(new MathFunctions());
		addFunctionClass(new BlockFunctions());
	};

	ScopedPointer<asmjit::JitRuntime> runtime;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassScope);
};

class ClassCompiler final: public BaseCompiler
{
public:

	ClassCompiler(GlobalScope& memoryPool_) :
		BaseCompiler(),
		memoryPool(memoryPool_),
		lastResult(Result::ok())
	{
		const auto& optList = memoryPool.getOptimizationPassList();

		if (!optList.isEmpty())
		{
			OptimizationFactory f;

			for (const auto& id : optList)
				addOptimization(f.createOptimization(id));
		}

		newScope = new JitCompiledFunctionClass(memoryPool);
	};


	void setFunctionCompiler(asmjit::X86Compiler* cc)
	{
		asmCompiler = cc;
	}

	asmjit::Runtime* getRuntime()
	{
		return newScope->pimpl->runtime;
	}

	JitCompiledFunctionClass* compileAndGetScope(const String& code)
	{
		NewClassParser parser(this, code);

		if(newScope == nullptr)
			newScope = new JitCompiledFunctionClass(memoryPool);

		try
		{
			setCurrentPass(BaseCompiler::Parsing);
			syntaxTree = parser.parseStatementList();
			executePass(PreSymbolOptimization, newScope->pimpl, syntaxTree);
			executePass(ResolvingSymbols, newScope->pimpl, syntaxTree);
			executePass(TypeCheck, newScope->pimpl, syntaxTree);
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

	Result getLastResult() { return lastResult; }

	ScopedPointer<JitCompiledFunctionClass> newScope;

	asmjit::X86Compiler* asmCompiler;

	String assembly;

	Result lastResult;

	GlobalScope& memoryPool;

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
	bool isConst = matchIf(JitTokens::const_);

	bool isSmoothedType = isSmoothedVariableType();
	currentHnodeType = matchType();
	
	matchIfSymbol();

	StatementPtr s;

	if (matchIf(JitTokens::openParen))
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding function " + currentSymbol.toString());
		s = parseFunction();
		matchIf(JitTokens::semicolon);
	}
	else
	{
		if (isSmoothedType)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding smoothed variable " + currentSymbol.toString());
			s = parseSmoothedVariableDefinition();
			match(JitTokens::semicolon);
		}
		else
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding variable " + currentSymbol.toString());
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

	return new Operations::SmoothedVariableDefinition(location, currentSymbol, currentHnodeType, value);
}

BlockParser::StatementPtr NewClassParser::parseVariableDefinition(bool /*isConst*/)
{
	auto type = currentHnodeType;
	auto target = new Operations::VariableReference(location, { {}, currentSymbol.id, type });
	target->isLocalToScope = true;

	match(JitTokens::assign_);
	ExprPtr expr = new Operations::Immediate(location, parseVariableStorageLiteral());
	return new Operations::Assignment(location, target, JitTokens::assign_, expr);
}

BlockParser::StatementPtr NewClassParser::parseFunction()
{
	auto func = new Operations::Function(location);

	StatementPtr newStatement = func;

	auto& fData = func->data;

	fData.id = currentSymbol.id;
	fData.returnType = currentHnodeType;
	fData.object = nullptr;

	while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
	{
		auto t = matchType();

		fData.args.add(t);
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

snex::VariableStorage BlockParser::parseVariableStorageLiteral()
{
	bool isMinus = matchIf(JitTokens::minus);

	auto type = Types::Helpers::getTypeFromStringValue(currentString);

	String stringValue = currentString;
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


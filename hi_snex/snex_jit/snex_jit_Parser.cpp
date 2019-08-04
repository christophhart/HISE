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
		addFunctionClass(new ConsoleFunctions());
		addFunctionClass(new BlockFunctions());
	};

	ScopedPointer<asmjit::JitRuntime> runtime;

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
			executePass(ResolvingSymbols, newScope->pimpl, syntaxTree);
			executePass(TypeCheck, newScope->pimpl, syntaxTree);
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
		list->add(s);
	}

	matchIf(JitTokens::closeBrace);

	finaliseSyntaxTree(list);

	return list.release();
}

BlockParser::StatementPtr NewClassParser::parseStatement()
{
	bool isConst = matchIf(JitTokens::const_);

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
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding variable " + currentSymbol.toString());
		s = parseVariableDefinition(isConst);
		match(JitTokens::semicolon);
	}

	return s;
}

BlockParser::StatementPtr NewClassParser::parseVariableDefinition(bool isConst)
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

	StatementPtr p = func;

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

	
	func->codeLength = location.location - func->code;
	
	return p;
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

void BaseCompiler::executePass(Pass p, BaseScope* scope, SyntaxTree* statements)
{
	setCurrentPass(p);

	for (auto s : *statements)
	{
		try
		{
			s->process(this, scope);
		}
		catch (DeadCodeException& d)
		{
			auto lineNumber = d.location.getLineNumber(d.location.program, d.location.location);

			String m;
			m << "Skipping removed expression at Line " << lineNumber;
			s->logOptimisationMessage(m);
		}
	}
}



snex::jit::Operations::FunctionCompiler& Operations::getFunctionCompiler(BaseCompiler* c)
{
	return *dynamic_cast<ClassCompiler*>(c)->asmCompiler;
}

BaseScope* Operations::findClassScope(BaseScope* scope)
{
	if (scope == nullptr)
		return nullptr;

	if (scope->getScopeType() == BaseScope::Class)
		return scope;
	else
		return findClassScope(scope->getParent());
}

snex::jit::BaseScope* Operations::findFunctionScope(BaseScope* scope)
{
	if (scope == nullptr)
		return nullptr;

	if (scope->getScopeType() == BaseScope::Function)
		return scope;
	else
		return findFunctionScope(scope->getParent());
}

asmjit::Runtime* Operations::getRuntime(BaseCompiler* c)
{
	return dynamic_cast<ClassCompiler*>(c)->getRuntime();
}



bool Operations::isOpAssignment(Expression::Ptr p)
{
	if (auto as = dynamic_cast<Assignment*>(p.get()))
		return as->assignmentType != JitTokens::assign_;

	return false;
}

Operations::Expression* Operations::findAssignmentForVariable(Expression::Ptr variable, BaseScope* scope)
{
	if (auto sBlock = findParentStatementOfType<SyntaxTree>(variable.get()))
	{
		for (auto s : *sBlock)
		{
			if (isStatementType<Assignment>(s))
				return dynamic_cast<Expression*>(s);
		}
	}
	else
	{
		return nullptr;
	}



#if 0
	if (auto fScope = dynamic_cast<FunctionScope*>(findFunctionScope(scope)))
	{
		auto pf = dynamic_cast<Function*>(fScope->parentFunction.get());

		auto vPtr = dynamic_cast<VariableReference*>(variable.get());

		for (auto s : *pf->statements)
		{
			if (auto as = dynamic_cast<Assignment*>(s))
			{
				auto tv = dynamic_cast<VariableReference*>(as->getSubExpr(0).get());

				if (tv->ref == vPtr->ref)
					return as;
			}
		}

		return nullptr;
	}
#endif
}


void Operations::Expression::checkAndSetType(int offset /*= 0*/, Types::ID expectedType)
{
	Types::ID exprType = expectedType;

	for (int i = offset; i < subExpr.size(); i++)
	{
		auto e = getSubExpr(i);

		auto thisType = e->getType();

		if (!Types::Helpers::matchesTypeStrict(thisType, exprType))
		{
			logWarning("Implicit cast, possible lost of data");

			if (e->isConstExpr())
			{
				subExpr.set(i, ConstExprEvaluator::evalCast(e.get(), exprType).get());
			}
			else
			{
				Ptr implCast = new Operations::Cast(e->location, e, exprType);
				implCast->attachAsmComment("Implicit cast from Line " + location.getLineNumber(location.program, location.location));
				subExpr.set(i, implCast.get());
			}
		}
		else
			exprType = Types::Helpers::getMoreRestrictiveType(exprType, thisType);
	}

	type = exprType;
}




void Operations::Expression::process(BaseCompiler* compiler, BaseScope* scope)
{
	Statement::process(compiler, scope);

	for (auto e : subExpr)
		e->process(compiler, scope);
}

snex::VariableStorage Operations::Expression::getConstExprValue() const
{
	if (isConstExpr())
		return dynamic_cast<const Immediate*>(this)->v;

	return {};
}

bool SyntaxTree::isFirstReference(Operations::Statement* v_) const
{
	Statement* lastRef = nullptr;

	for (auto v : variableReferences)
	{
		if (auto variable = dynamic_cast<Operations::VariableReference*>(v.get()))
		{
			if (variable->ref == dynamic_cast<Operations::VariableReference*>(v_)->ref)
				return v == v_;
		}
	}

	return false;
}

Operations::Statement* SyntaxTree::getLastVariableForReference(BaseScope::RefPtr ref) const
{
	Statement* lastRef = nullptr;

	for (auto v : variableReferences)
	{
		if (auto variable = dynamic_cast<Operations::VariableReference*>(v.get()))
		{
			if (ref == variable->ref)
				lastRef = variable;
		}
	}

	return lastRef;
}

Operations::Statement* SyntaxTree::getLastAssignmentForReference(BaseScope::RefPtr ref) const
{
	Statement* lastAssignment = nullptr;

	for (auto v : variableReferences)
	{
		if (auto variable = dynamic_cast<Operations::VariableReference*>(v.get()))
		{
			if (ref == variable->ref)
			{
				if (auto as = dynamic_cast<Operations::Assignment*>(v->parent.get()))
				{
					if (as->getTargetVariable() == variable)
						lastAssignment = as;
				}
			}
		}
	}

	return lastAssignment;
}

bool Operations::Statement::isConstExpr() const
{
	return isStatementType<Immediate>(static_cast<const Statement*>(this));
}



}
}


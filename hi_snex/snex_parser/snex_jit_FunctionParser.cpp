/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


snex::jit::BlockParser::StatementPtr CodeParser::parseStatementToBlock()
{
	if (matchIf(JitTokens::openBrace))
	{
		return parseStatementBlock();
	}
	else
	{
		auto scopeIdToUse = compiler->namespaceHandler.getCurrentNamespaceIdentifier();
		auto body = new Operations::StatementBlock(location, scopeIdToUse);
		body->addStatement(parseStatement());
		return body;
	}
}

snex::jit::BlockParser::StatementPtr CodeParser::parseStatementBlock()
{
	auto parentPath = getCurrentScopeStatement()->getPath();
	auto scopePath = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

	if (parentPath == scopePath)
		scopePath = compiler->namespaceHandler.createNonExistentIdForLocation(parentPath, location.getLine());

	location.calculateLineIfEnabled(compiler->namespaceHandler.shouldCalculateNumbers());
	auto startPos = location.getXYPosition();

	NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, scopePath);

	ScopedPointer<Operations::StatementBlock> b = new Operations::StatementBlock(location, scopePath);

	b->setParentScopeStatement(getCurrentScopeStatement());

	ScopedScopeStatementSetter svs(this, b.get());

	while (!isEOF() && currentType != JitTokens::closeBrace)
	{
		while (matchIf(JitTokens::using_))
			parseUsingAlias();

		b->addStatement(parseStatement().get());
	}

	CommentAttacher ca(*this);

	location.calculateLineIfEnabled(compiler->namespaceHandler.shouldCalculateNumbers());
	compiler->namespaceHandler.setNamespacePosition(scopePath, startPos, location.getXYPosition(), ca.getInfo());

	match(JitTokens::closeBrace);

	return b.release();
}

BlockParser::StatementPtr CodeParser::parseStatement(bool mustHaveSemicolon)
{
	while (currentType == JitTokens::semicolon)
		match(JitTokens::semicolon);

	if (matchIf(JitTokens::static_))
	{
		match(JitTokens::const_);

		if (!matchIfType({}))
			location.throwError("Expected type");

		if (currentTypeInfo.isComplexType())
			location.throwError("Can't define static const complex types");

		auto s = parseNewSymbol(NamespaceHandler::Constant);

		match(JitTokens::assign_);

		auto value = parseConstExpression(false);

		compiler->namespaceHandler.addConstant(s.id, value);

		match(JitTokens::semicolon);
		return parseStatement();
	}

	if (matchIf(JitTokens::if_))
		return(parseIfStatement());
	else if (matchIf(JitTokens::openBrace))
		return matchIfSemicolonAndReturn(parseStatementBlock());
	else if (matchIf(JitTokens::return_))
		return matchSemicolonAndReturn(parseReturnStatement(), mustHaveSemicolon);
	else if (matchIf(JitTokens::break_))
		return matchSemicolonAndReturn(new Operations::ControlFlowStatement(location, true), mustHaveSemicolon);
	else if (matchIf(JitTokens::continue_))
		return matchSemicolonAndReturn(new Operations::ControlFlowStatement(location, false), mustHaveSemicolon);
	else if (matchIf(JitTokens::while_))
		return parseWhileLoop();
	else if (matchIf(JitTokens::for_))
		return parseLoopStatement();

	if (matchIfType({}))
	{
		if (currentTypeInfo.isComplexType())
			return parseComplexTypeDefinition();
		else
			return matchSemicolonAndReturn(parseVariableDefinition(), mustHaveSemicolon);
	}
	else
		return matchSemicolonAndReturn(parseAssignment(), mustHaveSemicolon);
}

BlockParser::StatementPtr CodeParser::parseReturnStatement()
{
	ExprPtr rt;

	if (currentType == JitTokens::semicolon)
		return new Operations::ReturnStatement(location, nullptr);
	else
		return new Operations::ReturnStatement(location, parseExpression());

}

snex::jit::BlockParser::StatementPtr CodeParser::parseVariableDefinition()
{
    using namespace Operations;
    
	SymbolParser sp(*this, compiler->namespaceHandler);

	ExprPtr target;
	Symbol s;

	CommentAttacher ca(*this);

	if (currentTypeInfo.isDynamic())
		s = sp.parseNewDynamicSymbolSymbol(NamespaceHandler::Unknown);
	else
		s = sp.parseNewSymbol(NamespaceHandler::Unknown);

	auto varLocation = location;

	

	

	

	match(JitTokens::assign_);
	ExprPtr expr = parseExpression();

    if(auto fc = as<FunctionCall>(expr))
    {
        if(auto e = fc->extractFirstArgumentFromConstructor(compiler->namespaceHandler))
        {
            s.typeInfo = fc->getTypeInfo();
            expr = e;
        }
    }
    
	if (s.typeInfo.isDynamic())
	{
		if (expr->tryToResolveType(compiler))
		{
			bool isConst = s.isConst();
			bool isRef = s.isReference();

			s.typeInfo = expr->getTypeInfo().withModifiers(isConst, isRef);
		}
	}

	if (compiler->namespaceHandler.getSymbolType(s.id) != NamespaceHandler::Unknown)
		location.throwError("Duplicate symbol " + s.toString());
	
	compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Variable, ca.getInfo());

	if (s.typeInfo.isComplexType())
	{
        Array<NamespacedIdentifier> ids = { s.id };
		target = new Operations::ComplexTypeDefinition(location, ids, s.typeInfo);
		target->addStatement(expr);
        
        return addConstructorToComplexTypeDef(target, ids, false);
	}
	else
	{
		target = new Operations::VariableReference(varLocation, s);
		return new Operations::Assignment(location, target, JitTokens::assign_, expr, true);
	}
}

snex::jit::BlockParser::StatementPtr CodeParser::parseLoopStatement()
{
	match(JitTokens::openParen);

	matchType({});

	auto iteratorType = currentTypeInfo;

	// has to be done manually because the normal symbol parser consumes the ':' token...
	auto iteratorId = parseIdentifier();

	if(matchIf(JitTokens::colon))
	{
		ExprPtr loopBlock = parseExpression();

		match(JitTokens::closeParen);

		location.calculatePosition(false, false);

		auto id = compiler->namespaceHandler.createNonExistentIdForLocation({}, location.getLine());

		Symbol iteratorSymbol(id.getChildId(iteratorId), iteratorType);

		NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, id);

		lastComment = {};
		CommentAttacher ca(*this);

		compiler->namespaceHandler.addSymbol(id.getChildId(iteratorId), iteratorSymbol.typeInfo, NamespaceHandler::Variable, ca.getInfo());
		StatementPtr body = parseStatementToBlock();

		return new Operations::Loop(location, iteratorSymbol, loopBlock.get(), body);
	}

	match(JitTokens::assign_);
	auto initValue = parseExpression();
	match(JitTokens::semicolon);

	auto id = compiler->namespaceHandler.createNonExistentIdForLocation({}, location.getLine());
	Symbol iteratorSymbol(id.getChildId(iteratorId), iteratorType);
	NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, id);
	lastComment = {};
	CommentAttacher ca(*this);
	compiler->namespaceHandler.addSymbol(id.getChildId(iteratorId), iteratorSymbol.typeInfo, NamespaceHandler::Variable, ca.getInfo());

	auto iv = new Operations::VariableReference(location, iteratorSymbol);
	StatementPtr initAssignment = new Operations::Assignment(location, iv, JitTokens::assign_, initValue, true);
	auto condition = parseExpression();
	match(JitTokens::semicolon);
	auto postOp = parseStatement(false);
	match(JitTokens::closeParen);
	auto body = parseStatementToBlock();

	return new Operations::WhileLoop(location, initAssignment, condition, body, postOp);
}

snex::jit::BlockParser::StatementPtr CodeParser::parseWhileLoop()
{
	auto loc = location;

	match(JitTokens::openParen);

	ExprPtr cond = parseBool();

	match(JitTokens::closeParen);

	auto body = parseStatement();

	return new Operations::WhileLoop(loc, cond, body);
}

snex::jit::BlockParser::StatementPtr CodeParser::parseIfStatement()
{
	auto loc = location;
	match(JitTokens::openParen);

	ExprPtr cond = parseBool();

	match(JitTokens::closeParen);

	StatementPtr trueBranch = parseStatement();
	StatementPtr falseBranch;

	if (matchIf(JitTokens::else_))
	{
		falseBranch = parseStatement();
	}

	return new Operations::IfStatement(loc, cond, trueBranch, falseBranch);
}

void CodeParser::finaliseSyntaxTree(SyntaxTree* tree)
{
	auto lastStatement = tree->getLastStatement().get();

	while (auto bl = dynamic_cast<Operations::StatementBlock*>(lastStatement))
		lastStatement = bl->getLastStatement().get();

	auto isReturn = [](Operations::Statement* s)
	{
		return dynamic_cast<Operations::ReturnStatement*>(s) != nullptr;
	};

	if (isReturn(lastStatement))
		return;

	if (auto is = dynamic_cast<Operations::IfStatement*>(lastStatement))
	{
		if (!is->hasFalseBranch())
		{
			auto returnType = tree->getReturnType();

			if(returnType.isValid())
				is->throwError("Not all paths return a value");
		}
			

		auto lastTrueStatement = is->getTrueBranch();
		auto lastFalseStatement = is->getFalseBranch();

		auto hasReturn = [](Operations::Statement::Ptr p)
		{
			return as<Operations::ReturnStatement>(p) != nullptr;
		};


		auto trueStatementHasReturn = lastTrueStatement->forEachRecursive(hasReturn, Operations::IterationType::NoChildInlineFunctionBlocks);
		auto falseStatementHasReturn = lastFalseStatement != nullptr && lastFalseStatement->forEachRecursive(hasReturn, Operations::IterationType::NoChildInlineFunctionBlocks);

		if (trueStatementHasReturn || falseStatementHasReturn)
		{
			if (!trueStatementHasReturn)
			{
				auto ptr = new Operations::StatementBlock(lastTrueStatement->location, tree->getPath());
				ptr->addStatement(lastTrueStatement->clone(lastTrueStatement->location));
				ptr->addStatement(new Operations::ReturnStatement(lastTrueStatement->location, nullptr));

				lastTrueStatement->replaceInParent(ptr);
			}

			if (!falseStatementHasReturn && lastFalseStatement != nullptr)
			{
				auto ptr = new Operations::StatementBlock(lastFalseStatement->location, tree->getPath());
				ptr->addStatement(lastFalseStatement->clone(lastFalseStatement->location));
				ptr->addStatement(new Operations::ReturnStatement(lastFalseStatement->location, nullptr));

				lastFalseStatement->replaceInParent(ptr);
			}

			return;
		}
	}

	tree->addStatement(new Operations::ReturnStatement(location, nullptr));
}


snex::jit::BlockParser::StatementPtr CodeParser::parseAssignment()
{
	ExprPtr left = parseExpression();

	if (matchIfAssignmentType())
	{
		auto t = currentAssignmentType;
		ExprPtr right = parseExpression();

		if(isVectorOp(t, left))
			return new Operations::VectorOp(location, left, t, right);
		else
			return new Operations::Assignment(location, left, t, right, false);
	}

	return left;
}





SyntaxTreeInlineParser::SyntaxTreeInlineParser(InlineData* b_, const StringArray& originalParameters, const cppgen::Base& builder):
	CodeParser(b_->toSyntaxTreeData()->expression->currentCompiler, ""),
	b(b_),
	code(builder.toString()),
	originalArgs(originalParameters),
	originalLocation(b_->toSyntaxTreeData()->location)
{
	location.location = code.getCharPointer();
	location.program = code.getCharPointer();
	p = code.getCharPointer();
	program = code;
	length = code.length();
	endPointer = p + length;
}

juce::Result SyntaxTreeInlineParser::flush()
{
	using namespace Operations;

	auto d = b->toSyntaxTreeData();

	auto f = new Function(location, { d->getFunctionId(), d->expression->getTypeInfo() });

	StatementPtr asF = f;

	auto fc = dynamic_cast<FunctionCall*>(d->expression.get());

	int index = 0;

	for (auto& a : d->originalFunction.args)
	{
		if (a.typeInfo.isDynamic())
		{
			auto ti = d->args[index]->getTypeInfo();

			if (!ti.isDynamic())
				a.typeInfo = ti.withModifiers(a.isConst(), a.isReference());
		}

		index++;
	}

	

	f->data = d->originalFunction;

	if (f->data.returnType.isDynamic())
	{
		ReturnTypeInlineData rData(f->data);
		rData.object = fc;
		rData.object->currentCompiler = compiler;

		auto r = f->data.inliner->process(&rData);

		if (r.wasOk())
			f->data.returnType = rData.f.returnType;

		if (!r.wasOk())
			location.throwError(r.getErrorMessage());
	}

	f->data.inliner = nullptr;
	f->code = p;
	f->codeLength = length;
	f->isHardcodedFunction = true;
	
	if (d->object != nullptr)
		f->hardcodedObjectType = d->object->getTypeInfo().getComplexType();
	
	for (auto& a : originalArgs)
		f->parameters.add(a);

	if (d->object != nullptr)
	{
		f->hasObjectPtr = true;
	}

	try
	{
		if (d->expression->currentCompiler == nullptr)
			d->location.throwError("Internal compiler error");

		BaseCompiler::ScopedPassSwitcher sps1(d->expression->currentCompiler, BaseCompiler::FunctionTemplateParsing);
		f->process(d->expression->currentCompiler, d->expression->currentScope);

		BaseCompiler::ScopedPassSwitcher sps2(d->expression->currentCompiler, BaseCompiler::FunctionParsing);

		f->process(d->expression->currentCompiler, d->expression->currentScope);
	}
	catch (ParserHelpers::Error& e)
	{
		return Result::fail(e.toString());
		jassertfalse;
	}



	if (f->classData != nullptr)
	{
		auto r = f->classData->inliner->process(b);

		d->expression->currentCompiler->namespaceHandler.removeNamespace(f->classData->id);

 		return r;
	}

	

	return Result::fail("Can't find inliner");

#if 0
	auto d = b->toSyntaxTreeData();
	
	if (originalArgs.size() != d->args.size())
	{
		return Result::fail("arg amount mismatch");
	}

	auto& nh = compiler->namespaceHandler;
	Array<Symbol> args;

	auto thisId = d->path.getChildId("custom_syntax_inliner");
	//auto thisId = d->getFunctionId();

	NamespaceHandler::ScopedNamespaceSetter sns(nh, thisId);

	for (int i = 0; i < d->args.size(); i++)
	{
		args.add({ thisId.getChildId(originalArgs[i]), d->args[0]->getTypeInfo() });
		nh.addSymbol(args[i].id, args[i].typeInfo, NamespaceHandler::SymbolType::Variable);
	}

	// We don't use a ThisPointer statement because it can't be properly resolved
	// due to the missing function scope...
	if(d->object != nullptr)
		externalExpressions.set("this", new Operations::MemoryReference(d->location, d->object->clone(d->location), d->object->getTypeInfo().withModifiers(false, true), 0));

	using namespace Operations;

	currentScopeStatement = findParentStatementOfType<ScopeStatementBase>(d->expression);

	try
	{
		if (auto bl = parseStatementList())
		{
			auto sTree = as<SyntaxTree>(bl);
			sTree->setReturnType(d->expression->getTypeInfo());

			auto prevPass = d->expression->currentPass;

			ScopedPointer<FunctionScope> functionScope = new FunctionScope(d->expression->currentScope, NamespacedIdentifier("funkyBullshit"));

			for (auto& s : args)
				functionScope->parameters.add(s.id.getIdentifier());

			functionScope->parentFunction = nullptr;

			compiler->executePass(BaseCompiler::PreSymbolOptimization, functionScope, sTree);
			// add this when using stack...
			//compiler->executePass(BaseCompiler::DataSizeCalculation, functionScope, statements);
			compiler->executePass(BaseCompiler::DataAllocation, functionScope, sTree);
			compiler->executePass(BaseCompiler::DataInitialisation, functionScope, sTree);
			compiler->executePass(BaseCompiler::ResolvingSymbols, functionScope, sTree);
			compiler->executePass(BaseCompiler::TypeCheck, functionScope, sTree);
			compiler->executePass(BaseCompiler::PostSymbolOptimization, functionScope, sTree);

			compiler->setCurrentPass(prevPass);

			return d->makeInlinedStatementBlock(sTree, args);
		}
		else
		{
			return Result::fail("Can't parse inliner expression");
		}
	}
	catch (ParserHelpers::CodeLocation::Error& e)
	{
		return Result::fail(e.toString());
	}
#endif
}

void SyntaxTreeInlineParser::addExternalExpression(const String& id, ExprPtr e)
{
	externalExpressions.set(id, e->clone(b->toSyntaxTreeData()->location));
}

snex::jit::BlockParser::ExprPtr SyntaxTreeInlineParser::parseUnary()
{
	if (matchIf(JitTokens::syntax_tree_variable))
	{
		auto id = parseIdentifier();
		auto l = b->toSyntaxTreeData()->location;

		if (auto e = externalExpressions[id.toString()])
		{
			auto expr = parseDotOperator(e);

			if (matchIf(JitTokens::plusplus))
				expr = new Operations::Increment(location, expr, false, false);
			else if (matchIf(JitTokens::minusminus))
				expr = new Operations::Increment(location, expr, false, true);

			return expr;
		}
		
		l.throwError("Can't find external expression " + id.toString());
	}
	return BlockParser::parseUnary();
}


}
}

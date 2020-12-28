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
using namespace asmjit;


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

	compiler->namespaceHandler.setNamespacePosition(scopePath, startPos, location.getXYPosition(), ca.getInfo());

	match(JitTokens::closeBrace);

	return b.release();
}

BlockParser::StatementPtr CodeParser::parseStatement()
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
		return matchSemicolonAndReturn(parseReturnStatement());
	else if (matchIf(JitTokens::break_))
		return matchSemicolonAndReturn(new Operations::ControlFlowStatement(location, true));
	else if (matchIf(JitTokens::continue_))
		return matchSemicolonAndReturn(new Operations::ControlFlowStatement(location, false));
	else if (matchIf(JitTokens::while_))
		return parseWhileLoop();
	else if (matchIf(JitTokens::for_))
		return parseLoopStatement();

	if (matchIfType({}))
	{
		if (currentTypeInfo.isComplexType())
			return parseComplexTypeDefinition();
		else
			return matchSemicolonAndReturn(parseVariableDefinition());
	}
	else
		return matchSemicolonAndReturn(parseAssignment());
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

	if (s.typeInfo.isDynamic())
	{
		expr->tryToResolveType(compiler);
		bool isConst = s.isConst();
		bool isRef = s.isReference();

		s.typeInfo = expr->getTypeInfo().withModifiers(isConst, isRef);
	}

	
	compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Variable, ca.getInfo());

	if (s.typeInfo.isComplexType())
	{
		target = new Operations::ComplexTypeDefinition(location, { s.id }, s.typeInfo);
		target->addStatement(expr);
		return target;
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

	match(JitTokens::colon);
	
	ExprPtr loopBlock = parseExpression();

	match(JitTokens::closeParen);
	
	auto id = compiler->namespaceHandler.createNonExistentIdForLocation({}, location.getLine());

	Symbol iteratorSymbol(id.getChildId(iteratorId), iteratorType);

	NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, id);

	lastComment = {};
	CommentAttacher ca(*this);

	compiler->namespaceHandler.addSymbol(id.getChildId(iteratorId), {}, NamespaceHandler::Variable, ca.getInfo());
	StatementPtr body = parseStatementToBlock();
	
	return new Operations::Loop(location, iteratorSymbol, loopBlock.get(), body);
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
		lastStatement = bl->getLastStatement();

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
			if(dynamic_cast<Operations::ReturnStatement*>(p.get()))
				return true;

			return false;
		};

		if (!lastTrueStatement->forEachRecursive(hasReturn))
		{
			auto ptr = new Operations::StatementBlock(lastTrueStatement->location, tree->getPath());
			ptr->addStatement(lastTrueStatement->clone(lastTrueStatement->location));
			ptr->addStatement(new Operations::ReturnStatement(lastTrueStatement->location, nullptr));

			lastTrueStatement->replaceInParent(ptr);
		}
			

		if (lastFalseStatement != nullptr)
		{
			if (!lastFalseStatement->forEachRecursive(hasReturn))
			{
				lastFalseStatement->addStatement(new Operations::ReturnStatement(location, nullptr));
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





SyntaxTreeInlineParser::SyntaxTreeInlineParser(InlineData* b_, const StringArray& originalParameters, const String& code_):
	CodeParser(b_->toSyntaxTreeData()->expression->currentCompiler, ""),
	b(b_),
	code(code_),
	originalArgs(originalParameters),
	originalLocation(b_->toSyntaxTreeData()->location)
{
	location.location = code.getCharPointer();
	location.program = code.getCharPointer();
	p = code.getCharPointer();
	program = code;
	length = code.length();
	endPointer = p + length;
	skip();
}

juce::Result SyntaxTreeInlineParser::flush()
{
	auto d = b->toSyntaxTreeData();
	
	if (originalArgs.size() != d->args.size())
	{
		return Result::fail("arg amount mismatch");
	}

	auto& nh = compiler->namespaceHandler;
	Array<Symbol> args;

	auto thisId = d->path.getChildId("custom_syntax_inliner");

	NamespaceHandler::ScopedNamespaceSetter sns(nh, thisId);

	for (int i = 0; i < d->args.size(); i++)
	{
		args.add({ thisId.getChildId(originalArgs[i]), d->args[0]->getTypeInfo() });
		nh.addSymbol(args[i].id, args[i].typeInfo, NamespaceHandler::SymbolType::Variable);
	}

	if(d->object != nullptr)
		externalExpressions.set("this", new Operations::ThisPointer(d->location, d->object->getTypeInfo()));

	using namespace Operations;

	currentScopeStatement = findParentStatementOfType<ScopeStatementBase>(d->expression);

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
	else
	{
		return BlockParser::parseUnary();
	}
}


}
}
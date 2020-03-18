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


TypeInfo BlockParser::getDotParentType(ExprPtr e)
{
	if (auto dp = dynamic_cast<Operations::DotOperator*>(e.get()))
	{
		return dp->getDotParent()->getTypeInfo();
	}

	return {};
}

snex::jit::BlockParser::StatementPtr FunctionParser::parseStatementToBlock()
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

snex::jit::BlockParser::StatementPtr FunctionParser::parseStatementBlock()
{
	auto parentPath = getCurrentScopeStatement()->getPath();
	auto scopePath = compiler->namespaceHandler.getCurrentNamespaceIdentifier();

	if (parentPath == scopePath)
	{
		scopePath = location.createAnonymousScopeId(parentPath);
	}

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

	match(JitTokens::closeBrace);

	return b.release();
}

BlockParser::StatementPtr FunctionParser::parseStatement()
{
	if (matchIf(JitTokens::static_))
	{
		match(JitTokens::const_);

		if (!matchIfType())
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
	else if (matchIf(JitTokens::for_))
		return parseLoopStatement();

	if (matchIfType())
	{
		if (currentTypeInfo.isComplexType())
			return parseComplexTypeDefinition();
		else
			return matchSemicolonAndReturn(parseVariableDefinition());
	}
	else
		return matchSemicolonAndReturn(parseAssignment());
}

BlockParser::StatementPtr FunctionParser::parseReturnStatement()
{
	ExprPtr rt;

	if (currentType == JitTokens::semicolon)
		return new Operations::ReturnStatement(location, nullptr);
	else
		return new Operations::ReturnStatement(location, parseExpression());

}

snex::jit::BlockParser::StatementPtr FunctionParser::parseVariableDefinition()
{
	jassert(currentTypeInfo.isValid());

	SymbolParser sp(*this, compiler->namespaceHandler);

	auto s = sp.parseNewSymbol(NamespaceHandler::Unknown);
	auto target = new Operations::VariableReference(location, s);
	
	match(JitTokens::assign_);
	ExprPtr expr = parseExpression();
	
	compiler->namespaceHandler.addSymbol(s.id, s.typeInfo, NamespaceHandler::Variable);

	return new Operations::Assignment(location, target, JitTokens::assign_, expr, true);
}

snex::jit::BlockParser::StatementPtr FunctionParser::parseLoopStatement()
{
	match(JitTokens::openParen);

	matchType();

	auto iteratorType = currentTypeInfo;

	// has to be done manually because the normal symbol parser consumes the ':' token...
	auto iteratorId = parseIdentifier();

	match(JitTokens::colon);
	
	ExprPtr loopBlock = parseExpression();

	match(JitTokens::closeParen);
	
	auto id = location.createAnonymousScopeId(compiler->namespaceHandler.getCurrentNamespaceIdentifier());

	Symbol iteratorSymbol(id.getChildId(iteratorId), iteratorType);

	NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, id);

	compiler->namespaceHandler.addSymbol(id.getChildId(iteratorId), {}, NamespaceHandler::Variable);
	StatementPtr body = parseStatementToBlock();
	
	return new Operations::Loop(location, iteratorSymbol, loopBlock.get(), body);
}

snex::jit::BlockParser::StatementPtr FunctionParser::parseIfStatement()
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

void FunctionParser::finaliseSyntaxTree(SyntaxTree* tree)
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
			is->throwError("Not all paths return a value");

		auto lastTrueStatement = is->getTrueBranch();
		auto lastFalseStatement = is->getFalseBranch();

		auto hasReturn = [](Operations::Statement::Ptr p)
		{
			if(dynamic_cast<Operations::ReturnStatement*>(p.get()))
				return true;

			return false;
		};

		if (!lastTrueStatement->forEachRecursive(hasReturn))
			lastTrueStatement->addStatement(new Operations::ReturnStatement(location, nullptr));

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

BlockParser::ExprPtr BlockParser::createBinaryNode(ExprPtr l, ExprPtr r, TokenType op)
{
	return new Operations::BinaryOp(location, l, r, op);
}


snex::jit::BlockParser::StatementPtr FunctionParser::parseAssignment()
{
	ExprPtr left = parseExpression();

	if (matchIfAssignmentType())
	{
		auto t = currentAssignmentType;
		ExprPtr right = parseExpression();

		return new Operations::Assignment(location, left, t, right, false);
	}

	return left;
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
		if (matchIfType())
		{
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
	while(matchIf(JitTokens::dot))
	{
		auto e = parseReference(false);
		p = new Operations::DotOperator(location, p, e);
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
	bool found = false;

	Array<TemplateParameter> templateParameters;

    auto fSymbol = getCurrentSymbol();

	auto pType = getDotParentType(p);
	
	if (auto vt = pType.getTypedIfComplexType<VariadicTypeBase>())
	{
		if(currentType == JitTokens::lessThan)
			templateParameters = parseTemplateParameters();

		auto mId = vt->getVariadicId().getChildId(fSymbol.getName());

		FunctionClass::Ptr vfc = vt->getFunctionClass();
		Array<FunctionData> matches;
		vfc->addMatchingFunctions(matches, mId);
		
		auto f = matches.getFirst();

		if (auto il = f.inliner)
		{
			if (il->returnTypeFunction)
			{
				ReturnTypeInlineData rt(f);
				rt.object = dynamic_cast<Operations::DotOperator*>(p.get())->getDotParent();
				rt.templateParameters = templateParameters;

				il->process(&rt);
			}
		}

		fSymbol = Symbol(f.id, f.returnType);

		jassert(fSymbol.resolved);
	}

	while (matchIf(JitTokens::openParen))
	{
		if (auto dot = dynamic_cast<Operations::DotOperator*>(p.get()))
		{
			if (auto s = dynamic_cast<Operations::SymbolStatement*>(dot->getDotParent().get()))
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

		auto f = new Operations::FunctionCall(location, p, fSymbol, templateParameters);

		while (!isEOF() && currentType != JitTokens::closeParen)
		{
			f->addArgument(parseExpression());
			matchIf(JitTokens::comma);
		};

		match(JitTokens::closeParen);

		p = f;
		found = true;
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
	if (mustBeResolvedAtCompileTime)
		parseExistingSymbol();
	else
	{
		currentTypeInfo = {};
		currentSymbol = Symbol(parseIdentifier());
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
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

class FunctionParser : public BlockParser
{
public:

	FunctionParser(BaseCompiler* c, Operations::Function& f) :
		BlockParser(c, f.code, f.location.program, f.codeLength)
	{};
    
    virtual ~FunctionParser() {}

	StatementPtr parseStatementBlock();
	StatementPtr parseStatement();
	StatementPtr parseAssignment();
	StatementPtr parseReturnStatement();
	StatementPtr parseVariableDefinition(bool isConst);
	StatementPtr parseLoopStatement();
	StatementPtr parseIfStatement();
	

	void finaliseSyntaxTree(SyntaxTree* tree) override;

	ExprPtr createBinaryNode(ExprPtr l, ExprPtr r, TokenType op);

	ExprPtr parseExpression();
	ExprPtr parseTernaryOperator();
	ExprPtr parseBool();
	ExprPtr parseLogicOperation();
	ExprPtr parseComparation();
	ExprPtr parseSum();
	ExprPtr parseDifference();
	ExprPtr parseProduct();
	ExprPtr parseTerm();
	ExprPtr parseCast(Types::ID type);
	ExprPtr parseUnary();
	ExprPtr parseFactor();
	ExprPtr parseSymbolOrLiteral();
	ExprPtr parseReference();
	ExprPtr parseLiteral(bool isNegative=false);
	ExprPtr parseFunctionCall();
};


}
}

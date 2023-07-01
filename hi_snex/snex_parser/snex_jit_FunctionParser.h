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

class CodeParser : public BlockParser
{
public:

	CodeParser(BaseCompiler* c, const juce::String::CharPointerType& code, const juce::String::CharPointerType& wholeProgram, int length) :
		BlockParser(c, code, wholeProgram, length)
	{};

	CodeParser(BaseCompiler* c, const String& code) :
		BlockParser(c, code)
	{};

	virtual ~CodeParser() {};

	StatementPtr parseStatementToBlock();
	StatementPtr parseStatementBlock();
	StatementPtr parseStatement(bool mustHaveSemicolon=true);
	StatementPtr parseAssignment();
	StatementPtr parseReturnStatement();
	StatementPtr parseVariableDefinition();
	StatementPtr parseLoopStatement();
	StatementPtr parseWhileLoop();
	StatementPtr parseIfStatement();

	void finaliseSyntaxTree(SyntaxTree* tree) override;
};

class FunctionParser : public CodeParser
{
public:

	FunctionParser(BaseCompiler* c, Operations::Function& f) :
		CodeParser(c, f.code, f.location.program, f.codeLength)
	{};
    
    virtual ~FunctionParser() {}
};


struct SyntaxTreeInlineParser : private CodeParser
{
	SyntaxTreeInlineParser(InlineData* b, const StringArray& parameterNames, const cppgen::Base& code);
	Result flush();

	void addExternalExpression(const String& id, ExprPtr e);

	ExprPtr parseUnary() override;

	ParserHelpers::CodeLocation originalLocation;
	InlineData* b;

private:

	StringArray originalArgs;
	String code;

	HashMap<String, ExprPtr> externalExpressions;
};

}
}

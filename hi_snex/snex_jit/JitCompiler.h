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

class JITCompiler::Pimpl
{
public:

	Pimpl(const String& codeToCompile) :
		unprocessedCode(codeToCompile),
		preprocessor(codeToCompile),
		code(preprocessor.process()),
		compiledOK(false),
		useSafeFunctions(preprocessor.shouldUseSafeBufferFunctions())
	{

	};

	~Pimpl() {}

	

	JITScope* compileOneLineExpressionAndReturnScope(const Array<Types::ID>& typeList, GlobalScope* globalMemoryPool = nullptr)
	{
		String newCode;

		newCode << Types::Helpers::getCppTypeName(typeList[0]);

		newCode << " main(";

		for (int i = 1; i < typeList.size(); i++)
		{
			newCode << Types::Helpers::getCppTypeName(typeList[i]) << " ";
			newCode << Types::Helpers::getTypeChar(typeList[i]) << i;

			if (i != typeList.size() - 1)
				newCode << ", ";
		}

		newCode << "){\n return ";
		newCode << code;
		newCode << ";};";

		newCode.swapWith(code);

		return compileAndReturnScope(globalMemoryPool);
	}

	JITScope* compileAndReturnScope(GlobalScope* globalMemoryPool = nullptr)
	{
		ScopedPointer<JITScope> scope = new JITScope(globalMemoryPool);
		GlobalParser globalParser(code, scope, useSafeFunctions);

		try
		{
			globalParser.parseStatementList();
		}
		catch (ParserHelpers::CodeLocation::Error e)
		{
			compiledOK = false;
			return nullptr;
		}
		catch (String e)
		{
			errorMessage = e;
			compiledOK = false;
			return nullptr;
		}

		compiledOK = true;
		errorMessage = String();
		return scope.release();
	}

	bool wasCompiledOK() const { return compiledOK; };
	String getErrorMessage() const { return errorMessage; };

	String getCode(bool getPreprocessedCode) const
	{
		return getPreprocessedCode ? code : unprocessedCode;
	};

	void setGlobalVariable(const Identifier& id, int newValue)
	{
		externalVariables.add({ id, newValue });
	}

private:

	struct ExternalVariable
	{
		Identifier id;
		int value;
	};

	Array<ExternalVariable> externalVariables;

	int getLineNumberForError(int charactersFromStart)
	{
		int line = 1;

		for (int i = 0; i < jmin<int>(charactersFromStart, code.length()); i++)
		{
			if (code[i] == '\n')
			{
				line++;
			}
		}

		return line;
	}

	PreprocessorParser preprocessor;
	String code;
	bool compiledOK;
	String errorMessage;

	const String unprocessedCode;

	bool useSafeFunctions;
};

} // end namespace jit
} // end namespace snex


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

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;

void clearStringArrayLines(StringArray& sa, int startIndex, int endIndex)
{
	for (int i = startIndex; i < endIndex; i++)
	{
		sa.set(i, String());
	}
}


class PreprocessorParser
{
public:

	PreprocessorParser(const String& code_) :
		code(code_)
	{

	}

	String process()
	{
		StringArray lines = StringArray::fromLines(code);

		for (int i = 0; i < lines.size(); i++)
		{
			String line = lines[i];



			if (line.startsWith("#"))
			{
				const StringArray tokens = StringArray::fromTokens(line, " \t", "\"");

				const String op = tokens[0];

				if (op == "#define")
				{
					String name = tokens[1];
					String value = tokens.size() > 2 ? tokens[2] : "1";

					definitions.set(name, value);

					if (name == "SAFE")
					{
						useSafeBufferFunctions = false;
					}

					for (int j = i; j < lines.size(); j++)
					{
						lines.set(j, lines[j].replace(name, value));
					}

					lines.set(i, String());

					continue;
				}
				else if (op == "#if")
				{
					bool condition = tokens[1] == "1";

					const int ifStart = i;

					int elseIndex = -1;

					int endifIndex = -1;

					for (int j = i; j < lines.size(); j++)
					{
						if (lines[j].startsWith("#else"))
						{
							elseIndex = j;
						}
						if (lines[j].startsWith("#endif"))
						{
							endifIndex = j;
							break;
						}
					}

					if (elseIndex != -1 && endifIndex != -1)
					{
						lines.set(endifIndex, String());
						lines.set(elseIndex, String());

						if (condition)
						{
							clearStringArrayLines(lines, elseIndex, endifIndex);
							lines.set(i, String());
						}
						else
						{
							clearStringArrayLines(lines, i, elseIndex + 1);
						}

						continue;
					}
					else if (endifIndex != -1)
					{
						if (condition)
						{
							lines.set(endifIndex, String());
							lines.set(i, String());
						}
						else
						{
							clearStringArrayLines(lines, i, endifIndex + 1);
						}

						continue;
					}
				}
			}

		}

		String processedCode = lines.joinIntoString("\n");

		for (int i = 0; i < definitions.size(); i++)
		{
			String name = definitions.getName(i).toString();
			String value = definitions.getValueAt(i);

			processedCode = processedCode.replace(name, value);
		}

		return processedCode;
	}

	bool shouldUseSafeBufferFunctions() const
	{
		return useSafeBufferFunctions;
	}

private:

	bool useSafeBufferFunctions = true;

	const String& code;

	NamedValueSet definitions;
};


class GlobalParser : public ParserHelpers::TokenIterator,
					 public asmjit::ErrorHandler
{
public:

	GlobalParser(const String& code, JITScope* scope_, bool useSafeBufferFunctions_) :
		ParserHelpers::TokenIterator(code.getCharPointer(), code.getCharPointer(), code.length()),
		scope(scope_->pimpl),
		useSafeBufferFunctions(useSafeBufferFunctions_)
	{

	}

	bool handleError(asmjit::Error err, const char* message, asmjit::CodeEmitter* origin) override 
	{
		String error;
		error << "ASM Error: " << String(err) << ": " << message;

		location.throwError(error);

		return false;
	}

	void parseStatementList()
	{
		try
		{
			while (currentType != JitTokens::eof)
			{
				bool isConst = false;

				if (matchIf(JitTokens::const_)) { isConst = true; }

				if (JITTypeHelpers::matchesToken<float>(currentType)) parseStatement<float>(isConst);
				else if (JITTypeHelpers::matchesToken<double>(currentType)) parseStatement<double>(isConst);
				else if (JITTypeHelpers::matchesToken<int>(currentType)) parseStatement<int>(isConst);
				else if (JITTypeHelpers::matchesToken<BooleanType>(currentType)) parseStatement<BooleanType>(isConst);
				else if (currentType == JitTokens::void_) parseVoidFunction();

				else
				{
					location.throwError("Unexpected Token");
				}
			}

			for (int i = 0; i < functionsToParse.size(); i++)
			{
				auto& f = *functionsToParse[i];

				auto returnType = f.returnType;

				if (returnType == Types::ID::Float)			parseFunction<float>(f);
				else if (returnType == Types::ID::Double)	parseFunction<double>(f);
				else if (returnType == Types::ID::Integer)	parseFunction<int>(f);
				else if (returnType == Types::ID::Void)		parseFunction<void>(f);
				//else if (HiseJITTypeHelpers::matchesType<Buffer*>(f.lineType)) parseFunction<Buffer*>(f);
				else if (returnType == Types::ID::Integer) parseFunction<BooleanType>(f);
			}
		}
		catch (ParserHelpers::CodeLocation::Error e)
		{
			throw e;
		}
	}

	template <typename LineType> void parseStatement(bool isConst)
	{
		skip();

		const Identifier id = parseIdentifier();

		if (matchIf(JitTokens::assign_))
		{
			parseVariableDefinition<LineType>(id, isConst);
		}
		else if (matchIf(JitTokens::openParen))
		{
			parseFunctionDefinition<LineType>(id);
		}
		else if (matchIf(JitTokens::comma))
		{
			parseVariableDeclaration<LineType>(id);

			while (currentType != JitTokens::eof && currentType != JitTokens::semicolon)
			{
				parseVariableDeclaration<LineType>(parseIdentifier());
				matchIf(JitTokens::comma);
			}
		}
		else parseVariableDeclaration<LineType>(id);

		match(JitTokens::semicolon);
	}

	void parseVoidFunction()
	{
		skip();

		const Identifier id = parseIdentifier();

		match(JitTokens::openParen);

		parseFunctionDefinition<void>(id, true);

		match(JitTokens::semicolon);

	}

#if INCLUDE_GLOBALS
	template <typename LineType> void parseVariableDefinition(const Identifier& id, bool isConst)
	{
	}


	template <typename LineType> GlobalBase* parseVariableDeclaration(const Identifier& id)
	{
#if 0
		// We don't want to look for externals to allow overriding global variables
		if (scope->getGlobal(id, false) != nullptr) 
		{
			location.throwError("Identifier " + id.toString() + " already used.");
		}
#endif

		auto g = GlobalBase::create<LineType>(nullptr, id);

		scope->globals.add(g);

		return g;
	}
#endif

	template <typename LineType> LineType parseLiteral()
	{
		if (matchIf(JitTokens::true_))
		{
			if (!JITTypeHelpers::is<BooleanType, LineType>())
				location.throwError("Type mismatch: bool. Expected: " + JITTypeHelpers::getTypeName<LineType>());
			
			return (LineType)1;
		}

		if (matchIf(JitTokens::false_))
		{
			if (!JITTypeHelpers::is<BooleanType, LineType>())
				location.throwError("Type mismatch: bool. Expected: " + JITTypeHelpers::getTypeName<LineType>());

			return (LineType)0;
		}

		bool isMinus = matchIf(JitTokens::minus);
        
		if (!JITTypeHelpers::matchesType<LineType>(currentString))
		{
			location.throwError("Type mismatch: " + JITTypeHelpers::getTypeName(currentString) + ", Expected: " + JITTypeHelpers::getTypeName<LineType>());
		}

        LineType v = (LineType)(double)(isMinus ? ((double)currentValue * -1.0) : (double)(currentValue));

		match(JitTokens::literal);

		return v;
	}

	template <typename LineType> void parseFunctionDefinition(const Identifier &id, bool addVoidReturnStatement = false)
	{
		ScopedPointer<FunctionParserInfo> info = new FunctionParserInfo();

		info->id = id;
		info->program = location.program;
		info->addVoidReturnStatement = addVoidReturnStatement;
		info->returnType = Types::Helpers::getTypeFromTypeId<LineType>();

		while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
		{
			if (JITTypeHelpers::getTypeForToken(currentType) == typeid(void))
			{
				location.throwError("Type name expected");
			}

			info->args.add(Types::Helpers::getTypeFromTypeName(currentType));

			skip();

			info->parameterNames.add(parseIdentifier());

			matchIf(JitTokens::comma);
		}

		match(JitTokens::closeParen);

		match(JitTokens::openBrace);

		//HiseJIT::FunctionBuffer& fb = scope->createFunctionBuffer();

		auto start = location.location;
		info->offset = (int)(location.location - location.program);

		while (currentType != JitTokens::closeBrace && currentType != JitTokens::eof)
			skip();

		auto end = location.location;

		info->code = start;
		info->length = (int)(location.location - start);

		match(JitTokens::closeBrace);

		functionsToParse.add(info.release());

	}

	template <typename LineType> void parseFunction(FunctionParserInfo& info)
	{
		FunctionParser<LineType> f1(scope, info);

		ScopedPointer<asmjit::StringLogger> l = new asmjit::StringLogger();

		ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
		code->setLogger(l);
		code->init(scope->runtime->getCodeInfo());
		code->setErrorHandler(this);
		ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);

		FuncSignatureX sig;
		//AsmJitHelpers::fillSignature<LineType>(info, sig, false);
		compiler->addFunc(sig);

		f1.setCompiler(compiler);
		f1.parseFunctionBody();
		f1.addVoidReturnStatement();

		compiler->endFunc();
		compiler->finalize();
		compiler = nullptr;

		scope->runtime->add(&info.function, code);
		scope->assembly << "\n; Assembly code for " << info.id << "\n";
		scope->assembly << l->getString();
		code = nullptr;

		scope->addFunction(new FunctionData(info));
	}


#if 0
	template <typename ReturnType> void compileFunction1(const FunctionInfo& info)
	{
		FunctionParser<ReturnType> f1(scope, info);

		StringLogger l;

		ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
		code->init(scope->runtime->getCodeInfo());
		code->setErrorHandler(this);
		code->setLogger(&l);

		ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);

		FuncSignatureX sig;
		AsmJitHelpers::fillSignature<ReturnType>(info.data, sig, false);
		compiler->addFunc(sig);

		f1.setCompiler(compiler);
		f1.parseFunctionBody();
		f1.addVoidReturnStatement();

		compiler->endFunc();
		compiler->finalize();
		compiler = nullptr;

		scope->runtime->add(&info.data.function, code);
		scope->assembly << "\n; Assembly code for " << info.data.id << "\n";
		scope->assembly << l.getString();

		code = nullptr;

		scope->compiledFunctions.add(new FunkyFunctionData(info.data));
	}

	template <typename ReturnType, typename Param1Type, typename Param2Type> bool compileFunction2(const Identifier& id, const FunctionInfo& info)
	{
		if (true)
		{
			StringLogger l;

			ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
			code->init(scope->runtime->getCodeInfo());
			code->setErrorHandler(this);
			code->setLogger(&l);
			ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
			compiler->addFunc(FuncSignature2<ReturnType, Param1Type, Param2Type>());

			FunctionParser<ReturnType> f1(scope, info);
			f1.setCompiler(compiler);
			f1.parseFunctionBody();
			f1.addVoidReturnStatement();

			compiler->endFunc();
			compiler->finalize();
			compiler = nullptr;

			ReturnType(*fn)(Param1Type, Param2Type);

			scope->runtime->add(&fn, code);
			scope->assembly << "\n; Assembly code for " << id << "\n";
			scope->assembly << l.getString();

			code = nullptr;

			BaseFunction* b = new TypedFunction<ReturnType>(id, (void*)fn, info.data);

			scope->compiledFunctions.add(b);

			return true;
		}
		else
			return false;
	};
#endif

#if 0
	template <typename ReturnType, typename Param1Type, typename Param2Type, typename Param3Type>
	bool compileFunction3(const Identifier& id, const FunctionInfo& info)
	{
		if (JITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]) &&
			JITTypeHelpers::matchesToken<Param2Type>(info.parameterTypes[1]) &&
			JITTypeHelpers::matchesToken<Param3Type>(info.parameterTypes[2]))
		{
			StringLogger l;

			ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
			code->init(scope->runtime->getCodeInfo());
			code->setErrorHandler(this);
			code->setLogger(&l);
			ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
			compiler->addFunc(FuncSignature2<ReturnType, Param1Type, Param2Type, Param3Type>());

			FunctionParser<ReturnType, Param1Type, Param2Type, Param3Type> f1(scope, info);
			f1.setCompiler(compiler);
			f1.parseFunctionBody();
			f1.addVoidReturnStatement();

			compiler->endFunc();
			compiler->finalize();
			compiler = nullptr;

			ReturnType(*fn)(Param1Type, Param2Type, Param3Type);

			scope->runtime->add(&fn, code);
			scope->assembly << "\n; Assembly code for " << id << "\n";
			scope->assembly << l.getString();

			code = nullptr;

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type, Param2Type>(id, (void*)fn, (Param1Type)0, (Param2Type)0, (Param3Type)0);

			scope->compiledFunctions.add(b);

			return true;
		}
		else
			return false;
	};

	template <typename ReturnType, typename Param1Type, typename Param2Type, typename Param3Type, typename Param4Type, typename Param5Type>
	bool compileFunction4(const Identifier& id, const FunctionInfo& info)
	{
		if (JITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]) &&
			JITTypeHelpers::matchesToken<Param2Type>(info.parameterTypes[1]) &&
			JITTypeHelpers::matchesToken<Param3Type>(info.parameterTypes[2]) &&
			JITTypeHelpers::matchesToken<Param4Type>(info.parameterTypes[3]))
		{
			StringLogger l;

			ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
			code->init(scope->runtime->getCodeInfo());
			code->setErrorHandler(this);
			code->setLogger(&l);
			ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
			compiler->addFunc(FuncSignature2<ReturnType, Param1Type, Param2Type, Param3Type, Param4Type>());

			FunctionParser<ReturnType, Param1Type, Param2Type, Param3Type, Param4Type> f1(scope, info);
			f1.setCompiler(compiler);
			f1.parseFunctionBody();
			f1.addVoidReturnStatement();

			compiler->endFunc();
			compiler->finalize();
			compiler = nullptr;

			ReturnType(*fn)(Param1Type, Param2Type, Param3Type, Param4Type);

			scope->runtime->add(&fn, code);
			scope->assembly << "\n; Assembly code for " << id << "\n";
			scope->assembly << l.getString();

			code = nullptr;

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type, Param2Type, Param3Type, Param4Type>(id, (void*)fn, (Param1Type)0, (Param2Type)0, (Param3Type)0, (Param4Type)0);

			scope->compiledFunctions.add(b);

			return true;
		}
		else
			return false;
	};

	template <typename ReturnType, typename Param1Type, typename Param2Type, typename Param3Type, typename Param4Type, typename Param5Type> 
	bool compileFunction5(const Identifier& id, const FunctionInfo& info)
	{
		if (JITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]) &&
			JITTypeHelpers::matchesToken<Param2Type>(info.parameterTypes[1]) &&
			JITTypeHelpers::matchesToken<Param3Type>(info.parameterTypes[2]) &&
			JITTypeHelpers::matchesToken<Param4Type>(info.parameterTypes[3]) &&
			JITTypeHelpers::matchesToken<Param5Type>(info.parameterTypes[4]))
		{
			StringLogger l;

			ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
			code->init(scope->runtime->getCodeInfo());
			code->setErrorHandler(this);
			code->setLogger(&l);
			ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
			compiler->addFunc(FuncSignature2<ReturnType, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type>());

			FunctionParser<ReturnType, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type> f1(scope, info);
			f1.setCompiler(compiler);
			f1.parseFunctionBody();
			f1.addVoidReturnStatement();

			compiler->endFunc();
			compiler->finalize();
			compiler = nullptr;

			ReturnType(*fn)(Param1Type, Param2Type, Param3Type, Param4Type, Param5Type);

			scope->runtime->add(&fn, code);
			scope->assembly << "\n; Assembly code for " << id << "\n";
			scope->assembly << l.getString();

			code = nullptr;

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type, Param2Type, Param3Type, Param4Type, Param5Type>(id, (void*)fn, (Param1Type)0, (Param2Type)0, (Param3Type)0, (Param4Type)0, (Param5Type)0);

			scope->compiledFunctions.add(b);

			return true;
		}
		else
			return false;
	};
#endif

private:

	OwnedArray<FunctionParserInfo> functionsToParse;

	Identifier className;

	JITScope::Pimpl* scope;

	bool useSafeBufferFunctions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalParser)
};




} // end namespace jit
} // end namespace hnode


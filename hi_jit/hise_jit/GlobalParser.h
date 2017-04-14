/*
  ==============================================================================

    GlobalParser.h
    Created: 1 Mar 2017 11:42:46am
    Author:  Christoph

  ==============================================================================
*/

#ifndef GLOBALPARSER_H_INCLUDED
#define GLOBALPARSER_H_INCLUDED


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

	enum class PrivacyMode
	{
		public_,
		private_,
		numPrivacyModes
	};

	GlobalParser(const String& code, HiseJITScope* scope_, bool useSafeBufferFunctions_, bool useCppMode_=true) :
		ParserHelpers::TokenIterator(code.getCharPointer()),
		scope(scope_->pimpl),
		useSafeBufferFunctions(useSafeBufferFunctions_),
		useCppMode(useCppMode_)
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
			if (useCppMode)
			{
				match(HiseJitTokens::class_);

				className = parseIdentifier();

				match(HiseJitTokens::openBrace);
			}
			

			while (currentType != HiseJitTokens::eof)
			{
				if (useCppMode)
				{
					if (matchIf(HiseJitTokens::public_))
					{
						currentPrivacyMode = PrivacyMode::public_;
						match(HiseJitTokens::colon);
					}

					if (matchIf(HiseJitTokens::private_))
					{
						currentPrivacyMode = PrivacyMode::private_;
						match(HiseJitTokens::colon);
					}

					if (matchIf(HiseJitTokens::closeBrace))
					{
						match(HiseJitTokens::semicolon);
						match(HiseJitTokens::eof);
						break;
					}
				}
				

				bool isConst = false;

				if (matchIf(HiseJitTokens::const_)) { isConst = true; }

				if (HiseJITTypeHelpers::matchesToken<float>(currentType)) parseStatement<float>(isConst);
				else if (HiseJITTypeHelpers::matchesToken<double>(currentType)) parseStatement<double>(isConst);
				else if (HiseJITTypeHelpers::matchesToken<int>(currentType)) parseStatement<int>(isConst);
				else if (HiseJITTypeHelpers::matchesToken<BooleanType>(currentType)) parseStatement<BooleanType>(isConst);
#if INCLUDE_BUFFERS
				else if (HiseJITTypeHelpers::matchesToken<Buffer>(currentType)) parseBufferDefinition(isConst);
#endif
				else if (currentType == HiseJitTokens::void_) parseVoidFunction();

				else
				{
					location.throwError("Unexpected Token");
				}
			}

			for (int i = 0; i < functionsToParse.size(); i++)
			{
				auto& f = *functionsToParse[i];

				if (HiseJITTypeHelpers::matchesType<float>(f.lineType)) parseFunction<float>(f);
				else if (HiseJITTypeHelpers::matchesType<double>(f.lineType)) parseFunction<double>(f);
				else if (HiseJITTypeHelpers::matchesType<int>(f.lineType)) parseFunction<int>(f);
				else if (HiseJITTypeHelpers::matchesType<void>(f.lineType)) parseFunction<void>(f);
				//else if (HiseJITTypeHelpers::matchesType<Buffer*>(f.lineType)) parseFunction<Buffer*>(f);
				else if (HiseJITTypeHelpers::matchesType<BooleanType>(f.lineType)) parseFunction<BooleanType>(f);
			}
		}
		catch (ParserHelpers::CodeLocation::Error e)
		{
			int thisOffset = (int)(e.location - location.program);

			if (location.program != e.program)
			{
				e.offsetFromStart = thisOffset;
			}

			throw e;
		}
	}

	template <typename LineType> void parseStatement(bool isConst)
	{
		skip();

		const Identifier id = parseIdentifier();

		if (matchIf(HiseJitTokens::assign_))
		{
			parseVariableDefinition<LineType>(id, isConst);
		}
		else if (matchIf(HiseJitTokens::openParen))
		{
			parseFunctionDefinition<LineType>(id);
		}
		else if (matchIf(HiseJitTokens::comma))
		{
			parseVariableDeclaration<LineType>(id);

			while (currentType != HiseJitTokens::eof && currentType != HiseJitTokens::semicolon)
			{
				parseVariableDeclaration<LineType>(parseIdentifier());
				matchIf(HiseJitTokens::comma);
			}
		}
		else parseVariableDeclaration<LineType>(id);

		match(HiseJitTokens::semicolon);
	}

#if INCLUDE_BUFFERS
	void parseBufferDefinition(bool isConst)
	{
		skip();

		const Identifier id = parseIdentifier();

		ScopedPointer<GlobalBase> g = new GlobalBase(id, typeid(Buffer*));

		int size = 0;

		if (matchIf(HiseJitTokens::openParen))
		{
			size = (int)currentValue;
			match(HiseJitTokens::literal);
			match(HiseJitTokens::closeParen);
		}

		g->setBuffer(new VariantBuffer(size));
		g->isConst = isConst;

		scope->globals.add(g.release());

		match(HiseJitTokens::semicolon);
	}
#endif

	void parseVoidFunction()
	{
		skip();

		const Identifier id = parseIdentifier();

		match(HiseJitTokens::openParen);

		parseFunctionDefinition<void>(id, true);

		match(HiseJitTokens::semicolon);

	}

#if INCLUDE_GLOBALS
	template <typename LineType> void parseVariableDefinition(const Identifier& id, bool isConst)
	{
		GlobalBase* g = parseVariableDeclaration<LineType>(id);

		g->isConst = isConst;

		GlobalBase::store(g, parseLiteral<LineType>());

	}


	template <typename LineType> GlobalBase* parseVariableDeclaration(const Identifier& id)
	{
		if (scope->getGlobal(id) != nullptr)
		{
			location.throwError("Identifier " + id.toString() + " already used.");
		}

		auto g = GlobalBase::create<LineType>(id);

		scope->globals.add(g);

		return g;
	}
#endif

	template <typename LineType> LineType parseLiteral()
	{
		if (matchIf(HiseJitTokens::true_))
		{
			if (!HiseJITTypeHelpers::is<BooleanType, LineType>())
				location.throwError("Type mismatch: bool. Expected: " + HiseJITTypeHelpers::getTypeName<LineType>());
			
			return (LineType)1;
		}

		if (matchIf(HiseJitTokens::false_))
		{
			if (!HiseJITTypeHelpers::is<BooleanType, LineType>())
				location.throwError("Type mismatch: bool. Expected: " + HiseJITTypeHelpers::getTypeName<LineType>());

			return (LineType)0;
		}

		bool isMinus = matchIf(HiseJitTokens::minus);
        
		if (!HiseJITTypeHelpers::matchesType<LineType>(currentString))
		{
			location.throwError("Type mismatch: " + HiseJITTypeHelpers::getTypeName(currentString) + ", Expected: " + HiseJITTypeHelpers::getTypeName<LineType>());
		}

        LineType v = (LineType)(double)(isMinus ? ((double)currentValue * -1.0) : (double)(currentValue));

		match(HiseJitTokens::literal);

		return v;
	}

	template <typename LineType> void parseFunctionDefinition(const Identifier &id, bool addVoidReturnStatement = false)
	{
		ScopedPointer<FunctionInfo> info = new FunctionInfo();

		info->id = id;
		info->program = location.program;
		info->useSafeBufferFunctions = useSafeBufferFunctions;
		info->addVoidReturnStatement = addVoidReturnStatement;
		info->lineType = typeid(LineType);

		while (currentType != HiseJitTokens::closeParen && currentType != HiseJitTokens::eof)
		{
			if (HiseJITTypeHelpers::getTypeForToken(currentType) == typeid(void))
			{
				location.throwError("Type name expected");
			}

			info->parameterTypes.add(Identifier(currentType));

			skip();

			info->parameterNames.add(parseIdentifier());

			info->parameterAmount++;

			matchIf(HiseJitTokens::comma);
		}

		match(HiseJitTokens::closeParen);

		match(HiseJitTokens::openBrace);

		//HiseJIT::FunctionBuffer& fb = scope->createFunctionBuffer();

		auto start = location.location;
		info->offset = (int)(location.location - location.program);

		while (currentType != HiseJitTokens::closeBrace && currentType != HiseJitTokens::eof)
		{
			skip();
		}

		auto end = location.location;

		info->code = start;
		info->length = (int)(location.location - start);

		match(HiseJitTokens::closeBrace);

		functionsToParse.add(info.release());

	}

	template <typename LineType> void parseFunction(FunctionInfo& info)
	{
		switch (info.parameterAmount)
		{
		case 0:
		{
			compileFunction0<LineType>(info.id, info); return;
		}
		case 1:
		{
			if (compileFunction1<LineType, float>(info.id, info)) return;
			if (compileFunction1<LineType, int>(info.id, info)) return;
			if (compileFunction1<LineType, double>(info.id, info)) return;
			//if (compileFunction1<LineType, Buffer*>(info.id, info)) return;
			if (compileFunction1<LineType, BooleanType>(info.id, info)) return;
		}
		case 2:
		{
			if (compileFunction2<LineType, float, float>(info.id, info)) return;
			if (compileFunction2<LineType, float, double>(info.id, info)) return;
			if (compileFunction2<LineType, float, int>(info.id, info)) return;
			//if (compileFunction2<LineType, float, Buffer*>(info.id, info)) return;
			if (compileFunction2<LineType, float, BooleanType>(info.id, info)) return;

			if (compileFunction2<LineType, double, float>(info.id, info)) return;
			if (compileFunction2<LineType, double, double>(info.id, info)) return;
			if (compileFunction2<LineType, double, int>(info.id, info)) return;
			//if (compileFunction2<LineType, double, Buffer*>(info.id, info)) return;
			if (compileFunction2<LineType, double, BooleanType>(info.id, info)) return;

			if (compileFunction2<LineType, int, float>(info.id, info)) return;
			if (compileFunction2<LineType, int, double>(info.id, info)) return;
			if (compileFunction2<LineType, int, int>(info.id, info)) return;
			//if (compileFunction2<LineType, int, Buffer*>(info.id, info)) return;
			if (compileFunction2<LineType, int, BooleanType>(info.id, info)) return;
		}
		}
	}

	template <typename ReturnType> bool compileFunction0(const Identifier& id, const FunctionInfo& info)
	{	
		FunctionParser<ReturnType> f1(scope, info);

		

		ScopedPointer<asmjit::StringLogger> l = new asmjit::StringLogger();

		ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
		code->setLogger(l);
		code->init(scope->runtime->getCodeInfo());
		code->setErrorHandler(this);
		ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
		compiler->addFunc(FuncSignature0<ReturnType>());

		f1.setCompiler(compiler);
		f1.parseFunctionBody();
		f1.addVoidReturnStatement();
		

		compiler->endFunc();                           
		compiler->finalize();                          
		compiler = nullptr;

		ReturnType(*fn)();
		scope->runtime->add(&fn, code);

		//DBG(l->getString());

		code = nullptr;

		BaseFunction* b = new TypedFunction<ReturnType>(id, (void*)fn);

		scope->compiledFunctions.add(b);

		return true;
	}


	template <typename ReturnType, typename Param1Type> bool compileFunction1(const Identifier& id, const FunctionInfo& info)
	{

		if (HiseJITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]))
		{
			FunctionParser<ReturnType, Param1Type> f1(scope, info);

			StringLogger l;

			ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
			code->init(scope->runtime->getCodeInfo());
			code->setErrorHandler(this);
			code->setLogger(&l);
			ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
			compiler->addFunc(FuncSignature1<ReturnType, Param1Type>());

			f1.setCompiler(compiler);
			f1.parseFunctionBody();
			f1.addVoidReturnStatement();

			compiler->endFunc();
			compiler->finalize();
			compiler = nullptr;

			ReturnType(*fn)(Param1Type);
			scope->runtime->add(&fn, code);

			code = nullptr;


			DBG(l.getString());

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type>(id, (void*)fn, (Param1Type)0);

			scope->compiledFunctions.add(b);

			return true;
		}
		else
		{
			return false;
		}
	}

	template <typename ReturnType, typename Param1Type, typename Param2Type> bool compileFunction2(const Identifier& id, const FunctionInfo& info)
	{
		if (HiseJITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]) &&
			HiseJITTypeHelpers::matchesToken<Param2Type>(info.parameterTypes[1]))
		{
			FunctionParser<ReturnType, Param1Type, Param2Type> f1(scope, info);


			ScopedPointer<asmjit::CodeHolder> code = new asmjit::CodeHolder();
			code->init(scope->runtime->getCodeInfo());
			code->setErrorHandler(this);
			ScopedPointer<asmjit::X86Compiler> compiler = new asmjit::X86Compiler(code);
			compiler->addFunc(FuncSignature2<ReturnType, Param1Type, Param2Type>());

		

			f1.setCompiler(compiler);

			

			f1.parseFunctionBody();
			f1.addVoidReturnStatement();

			compiler->endFunc();
			compiler->finalize();
			compiler = nullptr;

			ReturnType(*fn)(Param1Type, Param2Type);
			scope->runtime->add(&fn, code);

			code = nullptr;

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type, Param2Type>(id, (void*)fn, (Param1Type)0, (Param2Type)0);

			scope->compiledFunctions.add(b);

			return true;
		}
		else
		{
			return false;
		}
	};

private:

	OwnedArray<FunctionInfo> functionsToParse;

	PrivacyMode currentPrivacyMode = PrivacyMode::public_;

	Identifier className;

	HiseJITScope::Pimpl* scope;

	bool useSafeBufferFunctions;
	bool useCppMode;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalParser)
};







#endif  // GLOBALPARSER_H_INCLUDED

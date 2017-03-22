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
    
    PreprocessorParser(const String& code_):
    code(code_)
    {
        
    }
    
    String process()
    {
        StringArray lines = StringArray::fromLines(code);
        
        for(int i = 0; i < lines.size(); i++)
        {
            String line = lines[i];
            
            
            
            if(line.startsWith("#"))
            {
                const StringArray tokens = StringArray::fromTokens(line, " \t", "\"");
                
                const String op = tokens[0];
                
                if(op == "#define")
                {
                    String name = tokens[1];
                    String value = tokens.size() > 2 ? tokens[2] : "1";
                    
                    definitions.set(name, value);
                    
					if (name == "DISABLE_SAFE_BUFFER_ACCESS")
					{
						useSafeBufferFunctions = false;
					}

                    for(int j = i; j < lines.size(); j++)
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
                    
                    for(int j = i; j < lines.size(); j++)
                    {
                        if(lines[j].startsWith("#else"))
                        {
                            elseIndex = j;
                        }
                        if(lines[j].startsWith("#endif"))
                        {
                            endifIndex = j;
                            break;
                        }
                    }
                    
                    if(elseIndex != -1 && endifIndex != -1)
                    {
                        lines.set(endifIndex, String());
						lines.set(elseIndex, String());

                        if(condition)
                        {
							clearStringArrayLines(lines, elseIndex, endifIndex);
                            lines.set(i, String());
                        }
                        else
                        {
							clearStringArrayLines(lines, i, elseIndex+1);
                        }
                        
                        continue;
                    }
                    else if(endifIndex != -1)
                    {
                        if(condition)
                        {
                            lines.set(endifIndex, String());
                            lines.set(i, String());
                        }
                        else
                        {
							clearStringArrayLines(lines, i, endifIndex+1);
                        }
                        
                        continue;
                    }
                }
            }
            
        }
        
        String processedCode = lines.joinIntoString("\n");
        
        for(int i = 0; i < definitions.size(); i++)
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


class GlobalParser : public ParserHelpers::TokenIterator
{
public:

	enum class PrivacyMode
	{
		public_,
		private_,
		numPrivacyModes
	};

	GlobalParser(const String& code, NativeJITScope* scope_, bool useSafeBufferFunctions_, bool useCppMode_=true) :
		ParserHelpers::TokenIterator(code.getCharPointer()),
		scope(scope_->pimpl),
		useSafeBufferFunctions(useSafeBufferFunctions_),
		useCppMode(useCppMode_)
	{

	}

	void parseStatementList()
	{
		try
		{
			if (useCppMode)
			{
				match(NativeJitTokens::class_);

				className = parseIdentifier();

				match(NativeJitTokens::openBrace);
			}
			

			while (currentType != NativeJitTokens::eof)
			{
				if (useCppMode)
				{
					if (matchIf(NativeJitTokens::public_))
					{
						currentPrivacyMode = PrivacyMode::public_;
						match(NativeJitTokens::colon);
					}

					if (matchIf(NativeJitTokens::private_))
					{
						currentPrivacyMode = PrivacyMode::private_;
						match(NativeJitTokens::colon);
					}

					if (matchIf(NativeJitTokens::closeBrace))
					{
						match(NativeJitTokens::semicolon);
						match(NativeJitTokens::eof);
						break;
					}
				}
				

				bool isConst = false;

				if (matchIf(NativeJitTokens::const_)) { isConst = true; }

				if (NativeJITTypeHelpers::matchesToken<float>(currentType)) parseStatement<float>(isConst);
				else if (NativeJITTypeHelpers::matchesToken<double>(currentType)) parseStatement<double>(isConst);
				else if (NativeJITTypeHelpers::matchesToken<int>(currentType)) parseStatement<int>(isConst);
				else if (NativeJITTypeHelpers::matchesToken<BooleanType>(currentType)) parseStatement<BooleanType>(isConst);
				else if (NativeJITTypeHelpers::matchesToken<Buffer>(currentType)) parseBufferDefinition(isConst);
				else if (currentType == NativeJitTokens::void_)
				{
					parseVoidFunction();
				}
				else
				{
					location.throwError("Unexpected Token");
				}
			}

			for (int i = 0; i < functionsToParse.size(); i++)
			{
				auto& f = *functionsToParse[i];

				if (NativeJITTypeHelpers::matchesType<float>(f.lineType)) parseFunction<float>(f);
				else if (NativeJITTypeHelpers::matchesType<double>(f.lineType)) parseFunction<double>(f);
				else if (NativeJITTypeHelpers::matchesType<int>(f.lineType)) parseFunction<int>(f);
				//else if (NativeJITTypeHelpers::matchesType<Buffer*>(f.lineType)) parseFunction<Buffer*>(f);
				else if (NativeJITTypeHelpers::matchesType<BooleanType>(f.lineType)) parseFunction<BooleanType>(f);
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

		if (matchIf(NativeJitTokens::assign_))
		{
			parseVariableDefinition<LineType>(id, isConst);
		}
		else if (matchIf(NativeJitTokens::openParen))
		{
			parseFunctionDefinition<LineType>(id);
		}
        else if(matchIf(NativeJitTokens::comma))
        {
            parseVariableDeclaration<LineType>(id);
            
            while(currentType != NativeJitTokens::eof && currentType != NativeJitTokens::semicolon)
            {
                parseVariableDeclaration<LineType>(parseIdentifier());
                matchIf(NativeJitTokens::comma);
            }
        }
		else parseVariableDeclaration<LineType>(id);

		match(NativeJitTokens::semicolon);
	}

	void parseBufferDefinition(bool isConst)
	{
		skip();

		const Identifier id = parseIdentifier();

		ScopedPointer<GlobalBase> g = new GlobalBase(id, typeid(Buffer*));

		int size = 0;

		if (matchIf(NativeJitTokens::openParen))
		{
			size = (int)currentValue;
			match(NativeJitTokens::literal);
			match(NativeJitTokens::closeParen);
		}

		g->setBuffer(new VariantBuffer(size));
		g->isConst = isConst;

		scope->globals.add(g.release());

		match(NativeJitTokens::semicolon);
	}

	void parseVoidFunction()
	{
		skip();

		const Identifier id = parseIdentifier();

		match(NativeJitTokens::openParen);

		parseFunctionDefinition<int>(id, true);

		match(NativeJitTokens::semicolon);

	}

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

	template <typename LineType> LineType parseLiteral()
	{
        bool isMinus = matchIf(NativeJitTokens::minus);
        
		if (!NativeJITTypeHelpers::matchesType<LineType>(currentString))
		{
			location.throwError("Type mismatch: " + NativeJITTypeHelpers::getTypeName(currentString) + ", Expected: " + NativeJITTypeHelpers::getTypeName<LineType>());
		}

        LineType v = (LineType)(double)(isMinus ? ((double)currentValue * -1.0) : (double)(currentValue));

		match(NativeJitTokens::literal);

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

		while (currentType != NativeJitTokens::closeParen && currentType != NativeJitTokens::eof)
		{
			if (NativeJITTypeHelpers::getTypeForToken(currentType) == typeid(void))
			{
				location.throwError("Type name expected");
			}

			info->parameterTypes.add(Identifier(currentType));

			skip();

			info->parameterNames.add(parseIdentifier());

			info->parameterAmount++;

			matchIf(NativeJitTokens::comma);
		}

		match(NativeJitTokens::closeParen);

		match(NativeJitTokens::openBrace);

		//NativeJIT::FunctionBuffer& fb = scope->createFunctionBuffer();

		auto start = location.location;
		info->offset = (int)(location.location - location.program);

		while (currentType != NativeJitTokens::closeBrace && currentType != NativeJitTokens::eof)
		{
			skip();
		}

		auto end = location.location;

		info->code = start;
		info->length = (int)(location.location - start);

		match(NativeJitTokens::closeBrace);

		functionsToParse.add(info.release());

	}

	template <typename LineType> void parseFunction(FunctionInfo& info)
	{
		switch (info.parameterAmount)
		{
		case 0:
		{
			compileFunction0<LineType>(info.id, info, info.addVoidReturnStatement); return;
		}
		case 1:
		{
			if (compileFunction1<LineType, float>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction1<LineType, int>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction1<LineType, double>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction1<LineType, Buffer*>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction1<LineType, BooleanType>(info.id, info, info.addVoidReturnStatement)) return;
		}
		case 2:
		{
			if (compileFunction2<LineType, float, float>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, float, double>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, float, int>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, float, Buffer*>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, float, BooleanType>(info.id, info, info.addVoidReturnStatement)) return;

			if (compileFunction2<LineType, double, float>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, double, double>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, double, int>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, double, Buffer*>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, double, BooleanType>(info.id, info, info.addVoidReturnStatement)) return;

			if (compileFunction2<LineType, int, float>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, int, double>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, int, int>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, int, Buffer*>(info.id, info, info.addVoidReturnStatement)) return;
			if (compileFunction2<LineType, int, BooleanType>(info.id, info, info.addVoidReturnStatement)) return;
		}
		}
	}

	template <typename ReturnType> bool compileFunction0(const Identifier& id, const FunctionInfo& info, bool addVoidReturnStatement)
	{	
		FunctionParser<ReturnType> f1(scope, info);
		f1.parseFunctionBody();
		
		if (addVoidReturnStatement)
		{
			f1.addVoidReturnStatement();
		}

		auto f = f1.compileFunction();

		BaseFunction* b = new TypedFunction<ReturnType>(id, (void*)f);

		scope->compiledFunctions.add(b);

		return true;
	}


	template <typename ReturnType, typename Param1Type> bool compileFunction1(const Identifier& id, const FunctionInfo& info, bool addVoidReturnStatement)
	{
		if (NativeJITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]))
		{
			FunctionParser<ReturnType, Param1Type> f1(scope, info);
			f1.parseFunctionBody();

			if (addVoidReturnStatement)
			{
				f1.addVoidReturnStatement();
			}

			auto f = f1.compileFunction();

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type>(id, (void*)f, Param1Type());

			scope->compiledFunctions.add(b);

			return true;
		}
		else
		{
			return false;
		}
	}

	template <typename ReturnType, typename Param1Type, typename Param2Type> bool compileFunction2(const Identifier& id, const FunctionInfo& info, bool addVoidReturnStatement)
	{
		

		if (NativeJITTypeHelpers::matchesToken<Param1Type>(info.parameterTypes[0]) &&
			NativeJITTypeHelpers::matchesToken<Param2Type>(info.parameterTypes[1]))
		{
			FunctionParser<ReturnType, Param1Type, Param2Type> f1(scope, info);
			f1.parseFunctionBody();

			if (addVoidReturnStatement)
			{
				f1.addVoidReturnStatement();
			}

			auto f = f1.compileFunction();

			BaseFunction* b = new TypedFunction<ReturnType, Param1Type, Param2Type>(id, (void*)f, Param1Type(), Param2Type());

			scope->compiledFunctions.add(b);

			return true;
		}
		else
		{
			return false;
		}
	}

private:

	OwnedArray<FunctionInfo> functionsToParse;

	PrivacyMode currentPrivacyMode = PrivacyMode::public_;

	Identifier className;

	NativeJITScope::Pimpl* scope;

	bool useSafeBufferFunctions;
	bool useCppMode;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalParser)
};







#endif  // GLOBALPARSER_H_INCLUDED

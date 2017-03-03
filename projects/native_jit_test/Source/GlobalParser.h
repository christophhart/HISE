/*
  ==============================================================================

    GlobalParser.h
    Created: 1 Mar 2017 11:42:46am
    Author:  Christoph

  ==============================================================================
*/

#ifndef GLOBALPARSER_H_INCLUDED
#define GLOBALPARSER_H_INCLUDED


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
                    
                    for(int j = i; j < lines.size(); j++)
                    {
                        lines.set(j, lines[j].replace(name, value));
                    }
                    
                    lines.remove(i--);
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
                        lines.remove(endifIndex);
                        
                        if(condition)
                        {
                            lines.removeRange(elseIndex, endifIndex-elseIndex);
                            lines.remove(i--);
                        }
                        else
                        {
                            lines.removeRange(i, elseIndex+1-i);
                        }
                        
                        continue;
                    }
                    else if(endifIndex != -1)
                    {
                        if(condition)
                        {
                            lines.remove(endifIndex);
                            lines.remove(i);
                            
                            i--;
                        }
                        else
                        {
                            lines.removeRange(i, endifIndex+1-i);
                            i--;
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
    
private:
    
    const String& code;
    
    NamedValueSet definitions;
};


class GlobalParser : public ParserHelpers::TokenIterator
{
public:

	GlobalParser(const String& code, GlobalScope* scope_) :
		ParserHelpers::TokenIterator(code),
		scope(scope_)
	{

	}

	void parseStatementList()
	{
		while (currentType != NativeJitTokens::eof)
		{
			if (NativeJITTypeHelpers::matchesToken<float>(currentType)) parseStatement<float>();
			else if (NativeJITTypeHelpers::matchesToken<double>(currentType)) parseStatement<double>();
			else if (NativeJITTypeHelpers::matchesToken<int>(currentType)) parseStatement<int>();
			else if (currentType == NativeJitTokens::void_)
			{
				parseVoidFunction();

			}
			else
			{
				location.throwError("Unexpected Token");
			}
		}
	}

	template <typename LineType> void parseStatement()
	{
		skip();

		const Identifier id = parseIdentifier();

		if (matchIf(NativeJitTokens::assign_))
		{
			parseVariableDefinition<LineType>(id);
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

	void parseVoidFunction()
	{
		skip();

		const Identifier id = parseIdentifier();

		match(NativeJitTokens::openParen);

		parseFunctionDefinition<int>(id, true);

		match(NativeJitTokens::semicolon);

	}

	template <typename LineType> void parseVariableDefinition(const Identifier& id)
	{
		GlobalBase* g = parseVariableDeclaration<LineType>(id);

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
		if (!NativeJITTypeHelpers::matchesType<LineType>(currentString))
		{
			location.throwError("Type mismatch: " + NativeJITTypeHelpers::getTypeName(currentString) + ", Expected: " + NativeJITTypeHelpers::getTypeName<LineType>());
		}

		LineType v = (LineType)currentValue;

		match(NativeJitTokens::literal);

		return v;
	}

	template <typename LineType> void parseFunctionDefinition(const Identifier &id, bool addVoidReturnStatement=false)
	{
		FunctionInfo info;
		
		info.program = location.program;

		if (addVoidReturnStatement)
		{
			//jassert(NativeJITTypeHelpers::is<int, LineType>());
		}

		while (currentType != NativeJitTokens::closeParen && currentType != NativeJitTokens::eof)
		{
			if (NativeJITTypeHelpers::getTypeForToken(currentType) == typeid(void))
			{
				location.throwError("Type name expected");
			}

			info.parameterTypes.add(Identifier(currentType));

			skip();

			info.parameterNames.add(parseIdentifier());
			
			info.parameterAmount++;

			matchIf(NativeJitTokens::comma);
		}

		match(NativeJitTokens::closeParen);

		match(NativeJitTokens::openBrace);

		//NativeJIT::FunctionBuffer& fb = scope->createFunctionBuffer();

		auto start = location.location;
		info.offset = (int)(location.location - location.program.getCharPointer());
		
		while (currentType != NativeJitTokens::closeBrace && currentType != NativeJitTokens::eof)
		{
			skip();
		}
		
		auto end = location.location;

		info.code = String(start, end);

		match(NativeJitTokens::closeBrace);

		
		

		switch (info.parameterAmount)
		{
		case 0:
		{
			compileFunction0<LineType>(id, info, addVoidReturnStatement); return;
		}
		case 1:
		{
			if (compileFunction1<LineType, float>(id, info, addVoidReturnStatement)) return;
			if (compileFunction1<LineType, int>(id, info, addVoidReturnStatement)) return;
			if (compileFunction1<LineType, double>(id, info, addVoidReturnStatement)) return;
		}
		case 2:
		{
			if (compileFunction2<LineType, float, float>(id, info, addVoidReturnStatement)) return;
			if (compileFunction2<LineType, float, double>(id, info, addVoidReturnStatement)) return;
			if (compileFunction2<LineType, float, int>(id, info, addVoidReturnStatement)) return;

			if (compileFunction2<LineType, double, float>(id, info, addVoidReturnStatement)) return;
			if (compileFunction2<LineType, double, double>(id, info, addVoidReturnStatement)) return;
			if (compileFunction2<LineType, double, int>(id, info, addVoidReturnStatement)) return;

			if (compileFunction2<LineType, int, float>(id, info, addVoidReturnStatement)) return;
			if (compileFunction2<LineType, int, double>(id, info, addVoidReturnStatement)) return;
			if (compileFunction2<LineType, int, int>(id, info, addVoidReturnStatement)) return;
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

	GlobalScope::Ptr scope;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalParser)
};




#endif  // GLOBALPARSER_H_INCLUDED

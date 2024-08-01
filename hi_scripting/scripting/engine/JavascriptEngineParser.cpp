namespace hise { using namespace juce;

//==============================================================================
struct HiseJavascriptEngine::RootObject::TokenIterator
{
	TokenIterator(const String& code, const String &externalFile) : location(code, externalFile), p(code.getCharPointer()) { skip(); }

	DebugableObject::Location createDebugLocation()
	{
		DebugableObject::Location loc;

		loc.fileName = location.externalFile;
		loc.charNumber = (int)(location.location - location.program.getCharPointer());

		return loc;
	}

	void skip()
	{
		skipWhitespaceAndComments();
		location.location = p;
		currentType = matchNextToken();
	}

	void skipBlock()
	{
		match(TokenTypes::openBrace);

		int braceCount = 1;

		while (currentType != TokenTypes::eof && braceCount > 0)
		{
			if (currentType == TokenTypes::openBrace) braceCount++;
			else if (currentType == TokenTypes::closeBrace) braceCount--;

			skip();
		}
	}

	void match(TokenType expected)
	{
		if (currentType != expected)
			location.throwError("Found " + getTokenName(currentType) + " when expecting " + getTokenName(expected));

		skip();
	}

	bool matchIf(TokenType expected)                                 { if (currentType == expected)  { skip(); return true; } return false; }
	bool matchesAny(TokenType t1, TokenType t2) const                { return currentType == t1 || currentType == t2; }
	bool matchesAny(TokenType t1, TokenType t2, TokenType t3) const  { return matchesAny(t1, t2) || currentType == t3; }

	CodeLocation location;
	TokenType currentType;
	var currentValue;

	void clearLastComment()
	{
		lastComment = String();
	}

	String lastComment;

    Identifier parseIdentifier()
    {
        Identifier i;
        if (currentType == TokenTypes::identifier)
            i = currentValue.toString();

        match(TokenTypes::identifier);
        return i;
    }
    
    VarTypeChecker::VarTypes matchVarType()
    {
        if(matchIf(TokenTypes::colon))
        {
            auto id = parseIdentifier();
            
            VarTypeChecker::VarTypes t = VarTypeChecker::Undefined;
            
#if ENABLE_SCRIPTING_SAFE_CHECKS
            t = VarTypeChecker::getTypeFromString(id);
            
            if(t == VarTypeChecker::Undefined)
                location.throwError("Unknown type " + id.toString());
#endif
            
            return t;
        }
        
        return VarTypeChecker::Undefined;
    }
    
	void skipWhitespaceAndComments()
	{
		for (;;)
		{
			p = p.findEndOfWhitespace();

			if (*p == '/')
			{
				const juce_wchar c2 = p[1];

				if (c2 == '/') { p = CharacterFunctions::find(p, (juce_wchar) '\n'); continue; }

				if (c2 == '*')
				{
					location.location = p;

					lastComment = String(p).upToFirstOccurrenceOf("*/", false, false).fromFirstOccurrenceOf("/**", false, false).trim();

					p = CharacterFunctions::find(p + 2, CharPointer_ASCII("*/"));


					if (p.isEmpty()) location.throwError("Unterminated '/*' comment");
					p += 2; continue;
				}
			}

			break;
		}
	}

private:
	String::CharPointerType p;

	static bool isIdentifierStart(const juce_wchar c) noexcept{ return CharacterFunctions::isLetter(c) || c == '_'; }
	static bool isIdentifierBody(const juce_wchar c) noexcept{ return CharacterFunctions::isLetterOrDigit(c) || c == '_'; }

		TokenType matchNextToken()
	{
		if (isIdentifierStart(*p))
		{
			String::CharPointerType end(p);
			while (isIdentifierBody(*++end)) {}

			const size_t len = (size_t)(end - p);
#define JUCE_JS_COMPARE_KEYWORD(name, str) if (len == sizeof (str) - 1 && matchToken (TokenTypes::name, len)) return TokenTypes::name;
			JUCE_JS_KEYWORDS(JUCE_JS_COMPARE_KEYWORD)

				currentValue = String(p, end); p = end;
			return TokenTypes::identifier;
		}

		if (p.isDigit())
		{
			if (parseHexLiteral() || parseFloatLiteral() || parseOctalLiteral() || parseDecimalLiteral())
				return TokenTypes::literal;

			location.throwError("Syntax error in numeric constant");
		}

		if (parseStringLiteral(*p) || (*p == '.' && parseFloatLiteral()))
			return TokenTypes::literal;

#define JUCE_JS_COMPARE_OPERATOR(name, str) if (matchToken (TokenTypes::name, sizeof (str) - 1)) return TokenTypes::name;
		JUCE_JS_OPERATORS(JUCE_JS_COMPARE_OPERATOR)

			if (!p.isEmpty())
				location.throwError("Unexpected character '" + String::charToString(*p) + "' in source");

		return TokenTypes::eof;
	}

	bool matchToken(TokenType name, const size_t len) noexcept
	{
		if (p.compareUpTo(CharPointer_ASCII(name), (int)len) != 0) return false;
		p += (int)len;  return true;
	}


	

	bool parseStringLiteral(juce_wchar quoteType)
	{
		if (quoteType != '"' && quoteType != '\'')
			return false;

		Result r(JSON::parseQuotedString(p, currentValue));
		if (r.failed()) location.throwError(r.getErrorMessage());
		return true;
	}

	bool parseHexLiteral()
	{
		if (*p != '0' || (p[1] != 'x' && p[1] != 'X')) return false;

		String::CharPointerType t(++p);
		int64 v = CharacterFunctions::getHexDigitValue(*++t);
		if (v < 0) return false;

		for (;;)
		{
			const int digit = CharacterFunctions::getHexDigitValue(*++t);
			if (digit < 0) break;
			v = v * 16 + digit;
		}

		currentValue = v; p = t;
		return true;
	}

	bool parseFloatLiteral()
	{
		int numDigits = 0;
		String::CharPointerType t(p);
		while (t.isDigit())  { ++t; ++numDigits; }

		const bool hasPoint = (*t == '.');

		if (hasPoint)
			while ((++t).isDigit())  ++numDigits;

		if (numDigits == 0)
			return false;

		juce_wchar c = *t;
		const bool hasExponent = (c == 'e' || c == 'E');

		if (hasExponent)
		{
			c = *++t;
			if (c == '+' || c == '-')  ++t;
			if (!t.isDigit()) return false;
			while ((++t).isDigit()) {}
		}

		if (!(hasExponent || hasPoint)) return false;

		currentValue = CharacterFunctions::getDoubleValue(p);  p = t;
		return true;
	}

	bool parseOctalLiteral()
	{
		String::CharPointerType t(p);
		int64 v = *t - '0';
		if (v != 0) return false;  // first digit of octal must be 0

		for (;;)
		{
			const int digit = (int)(*++t - '0');
			if (isPositiveAndBelow(digit, 8))        v = v * 8 + digit;
			else if (isPositiveAndBelow(digit, 10))  location.throwError("Decimal digit in octal constant");
			else break;
		}

		currentValue = v;  p = t;
		return true;
	}

	bool parseDecimalLiteral()
	{
		int64 v = 0;

		for (;; ++p)
		{
			const int digit = (int)(*p - '0');
			if (isPositiveAndBelow(digit, 10))  v = v * 10 + digit;
			else break;
		}

		currentValue = v;
		return true;
	}
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::ExpressionTreeBuilder : private TokenIterator
{
	ExpressionTreeBuilder(const String code, const String externalFile, HiseJavascriptPreprocessor::Ptr preprocessor_) :
		TokenIterator(code, externalFile),
		preprocessor(preprocessor_)
	{
#if ENABLE_SCRIPTING_BREAKPOINTS
		if (externalFile.isNotEmpty())
		{
			fileId = Identifier("File_" + File(externalFile).getFileNameWithoutExtension());
		}
#endif
	}

    HiseJavascriptPreprocessor::Ptr preprocessor;

	void setupApiData(HiseSpecialData &data, const String& codeToPreprocess)
	{
		hiseSpecialData = &data;
		currentNamespace = hiseSpecialData;
		
#if 0 //ENABLE_SCRIPTING_BREAKPOINTS

		Identifier localId = hiseSpecialData->getBreakpointLocalIdentifier();

		if (localId.isValid())
		{
			if (hiseSpecialData->getCallback(localId) != nullptr)
			{
				currentlyParsedCallback = localId;
			}
			else if (auto obj = hiseSpecialData->getInlineFunction(localId))
			{
				currentInlineFunction = obj;
				currentlyParsingInlineFunction = true;
			}

		}
#endif

		if(codeToPreprocess.isNotEmpty())
			preprocessCode(codeToPreprocess);
	}

	void preprocessCode(const String& codeToPreprocess, const String& externalFileName="");

	BlockStatement* parseStatementList()
	{
		ScopedPointer<BlockStatement> b(new BlockStatement(location));

		bool allowScopedBlockStatements = true;

		while (currentType != TokenTypes::closeBrace && currentType != TokenTypes::eof)
		{
#if ENABLE_SCRIPTING_BREAKPOINTS
			const int64 charactersBeforeToken = (location.location - location.program.getCharPointer());
			ScopedPointer<Statement> s = parseStatement();
			const int64 charactersAfterToken = (location.location - location.program.getCharPointer());
			Range<int64> r(charactersBeforeToken, charactersAfterToken);

			for (int i = 0; i < breakpoints.size(); i++)
			{
				const bool isOtherFile = !fileId.isNull() && breakpoints[i].snippetId != fileId;
				const bool isFileButOnInit = fileId.isNull() && breakpoints[i].snippetId.toString().startsWith("File");

				if (isOtherFile || isFileButOnInit)
					continue;

				if (breakpoints[i].found)
					continue;

				if (r.contains(breakpoints[i].charIndex))
				{
					if (currentInlineFunction != nullptr)
					{
						if (getCurrentNamespace() != hiseSpecialData)
						{
							const String combination = currentNamespace->id.toString() + "." + dynamic_cast<InlineFunction::Object*>(currentInlineFunction)->name.toString();

							s->breakpointReference.localScopeId = Identifier(combination);

						}
						else
						{
							s->breakpointReference.localScopeId = dynamic_cast<InlineFunction::Object*>(currentInlineFunction)->name;
						}
					}
						
					else if (currentlyParsedCallback.isValid())
					{
						s->breakpointReference.localScopeId = currentlyParsedCallback;
					}
					
					s->breakpointReference.index = breakpoints[i].index;
					breakpoints.getReference(i).found = true;
					break;
				}
			}
#else
			ScopedPointer<Statement> s = parseStatement();
#endif

			if (auto sbs = dynamic_cast<ScopedBlockStatement*>(s.get()))
			{
				if(!allowScopedBlockStatements)
				{
					location.throwError("Scoped block statements must be added at the scope start.");
				}

				if(auto shouldBeUsed = (USE_BACKEND || !sbs->isDebugStatement()))
				{
					b->scopedBlockStatements.add(sbs);
					s.release();
				}
				else
				{
					s = nullptr;
				}
			}
			else
			{
				b->statements.add(s.release());
				allowScopedBlockStatements = false;
			}
		}

		return b.release();
	}


	String removeUnneededNamespaces(int& counter);

	String uglify();

	void parseFunctionParamsAndBody(FunctionObject& fo)
	{
		if (matchIf(TokenTypes::openBracket))
		{
			while (currentType != TokenTypes::closeBracket)
			{
				auto paramName = currentValue.toString();

				fo.capturedLocals.add(parseExpression());

				if (currentType != TokenTypes::closeBracket)
					match(TokenTypes::comma);
			}

			for (auto e : fo.capturedLocals)
			{
				if (e->getVariableName().isNull())
				{
					location.throwError("Can't capture anonymous expressions");
				}
			}

			match(TokenTypes::closeBracket);
		}

		match(TokenTypes::openParen);

		while (currentType != TokenTypes::closeParen)
		{
			fo.parameters.add(currentValue.toString());
			match(TokenTypes::identifier);

			if (currentType != TokenTypes::closeParen)
				match(TokenTypes::comma);
		}

		struct ScopedFunctionSetter
		{
			ScopedFunctionSetter(ExpressionTreeBuilder& p, FunctionObject* o):
				parent(p)
			{
				lastObject = parent.currentFunctionObject;
				parent.currentFunctionObject = o;
			}

			~ScopedFunctionSetter()
			{
				parent.currentFunctionObject = lastObject;
			}

			ExpressionTreeBuilder& parent;
			DynamicObject* lastObject;
		};

		match(TokenTypes::closeParen);

		ScopedFunctionSetter svs(*this, &fo);

		// We need to temporarily set the currentInlineFunction to nullptr to avoid
		// local references inside the nested function body which will fail when
		// ENABLE_SCRIPTING_BREAKPOINTS is disabled
		ScopedValueSetter<DynamicObject*> svs1(outerInlineFunction, currentInlineFunction);
		ScopedValueSetter<DynamicObject*> svs2(currentInlineFunction, nullptr);

		fo.body = parseBlock();
	}


	Expression* parseExpression()
	{
		Identifier id = Identifier::isValidIdentifier(currentValue.toString()) ? Identifier(currentValue.toString()) : Identifier::null;

		bool skipConsoleCalls = false;

#if !ENABLE_SCRIPTING_SAFE_CHECKS
		static const Identifier c("Console");

		if (id == c)
		{
			skipConsoleCalls = true;
		}
#endif

		ExpPtr lhs = parseLogicOperator();

		if (matchIf(TokenTypes::in))
		{
			ExpPtr rhs(parseExpression());

			IteratorData d;

			d.id = id;
			currentIterators.add(d);

			return rhs.release();
		}

		if (matchIf(TokenTypes::arrow))				return parseArrowFunction(lhs);
		if (matchIf(TokenTypes::question))          return parseTerneryOperator(lhs);
		if (matchIf(TokenTypes::assign))            { ExpPtr rhs(parseExpression()); return new Assignment(location, lhs, rhs); }
		if (matchIf(TokenTypes::plusEquals))        return parseInPlaceOpExpression<AdditionOp>(lhs);
		if (matchIf(TokenTypes::minusEquals))       return parseInPlaceOpExpression<SubtractionOp>(lhs);
        if (matchIf(TokenTypes::timesEquals))       return parseInPlaceOpExpression<MultiplyOp>(lhs);
        if (matchIf(TokenTypes::divideEquals))      return parseInPlaceOpExpression<DivideOp>(lhs);
        if (matchIf(TokenTypes::moduloEquals))      return parseInPlaceOpExpression<ModuloOp>(lhs);
		if (matchIf(TokenTypes::leftShiftEquals))   return parseInPlaceOpExpression<LeftShiftOp>(lhs);
        if (matchIf(TokenTypes::andEquals))         return parseInPlaceOpExpression<BitwiseAndOp>(lhs);
        if (matchIf(TokenTypes::orEquals))          return parseInPlaceOpExpression<BitwiseOrOp>(lhs);
        if (matchIf(TokenTypes::xorEquals))         return parseInPlaceOpExpression<BitwiseXorOp>(lhs);
		if (matchIf(TokenTypes::rightShiftEquals))  return parseInPlaceOpExpression<RightShiftOp>(lhs);

		if (skipConsoleCalls)
		{
			return new Expression(location);
		}

		return lhs.release();
	}

	Array<Breakpoint> breakpoints;

private:

	HiseSpecialData *hiseSpecialData;

	bool currentlyParsingInlineFunction = false;
	Identifier currentlyParsedCallback = Identifier::null;

	Identifier fileId;

	DynamicObject* currentFunctionObject = nullptr;
	DynamicObject* outerInlineFunction = nullptr;
	DynamicObject* currentInlineFunction = nullptr;

	JavascriptNamespace* currentNamespace = nullptr;

	JavascriptNamespace* getCurrentNamespace()
	{
		jassert(currentNamespace != nullptr);

		return currentNamespace;
	}

	void throwError(const String& err) const  { location.throwError(err); }

	template <typename OpType>
	Expression* parseInPlaceOpExpression(ExpPtr& lhs)
	{
		ExpPtr rhs(parseExpression());
		Expression* bareLHS = lhs; // careful - bare pointer is deliberately alised
		return new SelfAssignment(location, bareLHS, new OpType(location, lhs, rhs));
	}

	BlockStatement* parseBlock()
	{
		match(TokenTypes::openBrace);
		ScopedPointer<BlockStatement> b(parseStatementList());
		match(TokenTypes::closeBrace);
		return b.release();
	}

	

	Statement* parseStatement()
	{
		if (matchIf(TokenTypes::dot))			   return parseScopedBlockStatement();
		if (matchIf(TokenTypes::include_))		   return parseExternalFile();
		if (matchIf(TokenTypes::inline_))		   return parseInlineFunction(getCurrentNamespace());

		if (currentType == TokenTypes::openBrace)   return parseBlock();

		if (matchIf(TokenTypes::const_))		   return parseConstVar(getCurrentNamespace());
		if (matchIf(TokenTypes::var))              return parseVar();
		if (matchIf(TokenTypes::register_var))	   return parseRegisterVar(getCurrentNamespace());
		if (matchIf(TokenTypes::global_))		   return parseGlobalAssignment();
		if (matchIf(TokenTypes::local_))		   return parseLocalAssignment();
		if (matchIf(TokenTypes::namespace_))	   return parseNamespace();
		if (matchIf(TokenTypes::if_))              return parseIf();
		if (matchIf(TokenTypes::while_))           return parseDoOrWhileLoop(false);
		if (matchIf(TokenTypes::do_))              return parseDoOrWhileLoop(true);
		if (matchIf(TokenTypes::for_))             return parseForLoop();
		if (matchIf(TokenTypes::return_))          return parseReturn();
		if (matchIf(TokenTypes::switch_))		   return parseSwitchBlock();
		if (matchIf(TokenTypes::break_))           return new BreakStatement(location);
		if (matchIf(TokenTypes::continue_))        return new ContinueStatement(location);
		if (matchIf(TokenTypes::function))         return parseFunction();
		if (matchIf(TokenTypes::semicolon))        return new Statement(location);
		if (matchIf(TokenTypes::plusplus))         return parsePreIncDec<AdditionOp>();
		if (matchIf(TokenTypes::minusminus))       return parsePreIncDec<SubtractionOp>();

		if (matchesAny(TokenTypes::openParen, TokenTypes::openBracket))
			return matchEndOfStatement(parseFactor());

		if (matchesAny(TokenTypes::identifier, TokenTypes::literal, TokenTypes::minus))
		{
			ExpPtr ex = parseExpression();
			return matchEndOfStatement(ex.release());
		}

		throwError("Found " + getTokenName(currentType) + " when expecting a statement");
		return nullptr;
	}

	Statement* parseScopedBlockStatement()
	{
		ExpPtr condition;

		if(matchIf(TokenTypes::if_))
		{
			match(TokenTypes::openParen);
			condition = parseExpression();
			match(TokenTypes::closeParen);
			match(TokenTypes::colon);
		}

		auto typeId = parseIdentifier();
		
		if(typeId == ScopedSetter::getStaticId())
		{
			ScopedPointer<ScopedSetter> n = new ScopedSetter(location, condition);

			match(TokenTypes::openParen);
			n->lhs = parseExpression();
			match(TokenTypes::comma);
			n->rhs = parseExpression();
			match(TokenTypes::closeParen);

			return n.release();
		}
		else if(typeId == ScopedSuspender::getStaticId())
		{
			match(TokenTypes::openParen);
			auto name = Identifier(currentValue.toString());
			dispatch::HashedCharPtr p(name);

			try
			{
				auto path = dispatch::HashedPath(p);
				match(TokenTypes::literal);
				match(TokenTypes::closeParen);
				return new ScopedSuspender(location, condition, path);
			}
			catch(Result& r)
			{
				location.throwError(r.getErrorMessage());
			}
		}
		else if(typeId == ScopedBypasser::getStaticId())
		{
			match(TokenTypes::openParen);
			auto b = parseExpression();
			match(TokenTypes::closeParen);

			return new ScopedBypasser(location, condition, b);
		}
		else if(typeId == ScopedTracer::getStaticId())
		{
			match(TokenTypes::openParen);
			auto name = currentValue.toString();
			match(TokenTypes::literal);
			match(TokenTypes::closeParen);

			return new ScopedTracer(location, condition, name);
		}
		else if(typeId == ScopedProfiler::getStaticId())
		{
			match(TokenTypes::openParen);
			auto name = currentValue.toString();
			match(TokenTypes::literal);
			match(TokenTypes::closeParen);

			return new ScopedProfiler(location, condition, name);
		}
		else if(typeId == ScopedCounter::getStaticId())
		{
			match(TokenTypes::openParen);
			auto name = currentValue.toString();
			match(TokenTypes::literal);
			match(TokenTypes::closeParen);

			return new ScopedCounter(location, condition, name);
		}
		else if(typeId == ScopedDumper::getStaticId())
		{
			match(TokenTypes::openParen);

			OwnedArray<Expression> expressions;

			while(true)
			{
				if(matchIf(TokenTypes::closeParen))
					break;
				if(matchIf(TokenTypes::eof))
					break;

				expressions.add(parseExpression());
				matchIf(TokenTypes::comma);
			}

			if(expressions.isEmpty())
				location.throwError("expected expressions");

			auto n = new ScopedDumper(location, condition);
			n->dumpObjects.swapWith(expressions);
			return n;
		}
		else if (typeId == ScopedNoop::getStaticId())
		{
			match(TokenTypes::openParen);

			while(true)
			{
				if(matchIf(TokenTypes::closeParen))
					break;
				if(matchIf(TokenTypes::eof))
					break;

				ExpPtr unused = parseExpression();
				matchIf(TokenTypes::comma);
			}

			matchIf(TokenTypes::closeParen);

			return new ScopedNoop(location, condition);
		}
		else if(typeId == ScopedPrinter::getStaticId())
		{
			match(TokenTypes::openParen);
			auto name = currentValue.toString();
			match(TokenTypes::literal);
			match(TokenTypes::closeParen);

			return new ScopedPrinter(location, condition, name);
		}
		else if(typeId == ScopedLocker::getStaticId())
		{
			match(TokenTypes::openParen);
			auto l = (int)parseExpression()->getResult(Scope(nullptr, nullptr, nullptr));
			match(TokenTypes::closeParen);

			return new ScopedLocker(location, condition, (LockHelpers::Type)l);
		}
		else if(typeId == ScopedBefore::getStaticId())
		{
			ScopedPointer<ScopedBefore> n = new ScopedBefore(location, condition);

			match(TokenTypes::openParen);
			n->expected = parseExpression();
			match(TokenTypes::comma);
			n->actual = parseExpression();
			match(TokenTypes::closeParen);

			return n.release();
		}
		else if(typeId == ScopedAfter::getStaticId())
		{
			ScopedPointer<ScopedAfter> n = new ScopedAfter(location, condition);

			match(TokenTypes::openParen);
			n->expected = parseExpression();
			match(TokenTypes::comma);
			n->actual = parseExpression();
			match(TokenTypes::closeParen);

			return n.release();
		}

		location.throwError("unknown scope statement type " + typeId.toString());
		RETURN_IF_NO_THROW(nullptr);
	}

	String getFileContent(const String &fileNameInScript, String &refFileName, bool allowMultipleIncludes = false)
	{
		String cleanedFileName = fileNameInScript.removeCharacters("\"\'");

		if (cleanedFileName.contains("{DEVICE}"))
		{
			cleanedFileName = cleanedFileName.replace("{DEVICE}", HiseDeviceSimulator::getDeviceName());
		}

#if USE_BACKEND

		if (File::isAbsolutePath(cleanedFileName)) 
			refFileName = cleanedFileName;
		else if (cleanedFileName.contains("{GLOBAL_SCRIPT_FOLDER}"))
		{
			File globalScriptFolder = PresetHandler::getGlobalScriptFolder(dynamic_cast<Processor*>(hiseSpecialData->processor));

			const String f1 = cleanedFileName.fromFirstOccurrenceOf("{GLOBAL_SCRIPT_FOLDER}", false, false);

			refFileName = globalScriptFolder.getChildFile(f1).getFullPathName();
		}
		else
		{
			const String fileName = "{PROJECT_FOLDER}" + cleanedFileName;
			refFileName = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(hiseSpecialData->processor)).getFilePath(fileName, ProjectHandler::SubDirectories::Scripts);
		}

		File f(refFileName);
		const String shortFileName = f.getFileName();

		auto mc = dynamic_cast<Processor*>(hiseSpecialData->processor)->getMainController();
		auto ef = mc->getExternalScriptFile(f, false);

		String code;

		if(ef != nullptr)
			code = ef->getFileDocument().getAllContent();
		else if (f.existsAsFile())
			code = f.loadFileAsString();
		else
			throwError("File " + refFileName + " not found");
		
		if (!allowMultipleIncludes)
		{
			for (int i = 0; i < hiseSpecialData->includedFiles.size(); i++)
			{
				if (hiseSpecialData->includedFiles[i]->f == f)
				{
					debugToConsole(dynamic_cast<Processor*>(hiseSpecialData->processor), "File " + shortFileName + " was included multiple times");
					return String();
				}
			}
		}

		return code;

#else

		refFileName = cleanedFileName;

		if (File::isAbsolutePath(refFileName))
		{
			File f(refFileName);

			if (!allowMultipleIncludes)
			{
				for (int i = 0; i < hiseSpecialData->includedFiles.size(); i++)
				{
					if (hiseSpecialData->includedFiles[i]->f == f)
					{
						DBG("File " + refFileName + " was included multiple times");
						return String();
					}

				}
			}

			return f.loadFileAsString();
		}
		else
		{
			if (!allowMultipleIncludes)
			{
				for (int i = 0; i < hiseSpecialData->includedFiles.size(); i++)
				{
					if (hiseSpecialData->includedFiles[i]->scriptName == refFileName)
					{
						DBG("Script " + refFileName + " was included multiple times");
						return String();
					}
				}
			}

			return dynamic_cast<Processor*>(hiseSpecialData->processor)->getMainController()->getExternalScriptFromCollection(fileNameInScript);
		}
#endif
	};

	

	/** Call this to include and register the file, then use getFileContent with the reference String to obtain the actual content. */
	String addExternalFile()
	{
		if (getCurrentNamespace() != hiseSpecialData)
		{
			location.throwError("Including files inside namespaces is not supported");
		}

		match(TokenTypes::openParen);

		String refFileName;
		String fileContent = getFileContent(currentValue.toString(), refFileName);

		if (fileContent.isEmpty())
			return {};

#if USE_BACKEND
		File f(refFileName);
		hiseSpecialData->includedFiles.add(new ExternalFileData(ExternalFileData::Type::RelativeFile, f, String()));
#else

		if (File::isAbsolutePath(refFileName))
			hiseSpecialData->includedFiles.add(new ExternalFileData(ExternalFileData::Type::AbsoluteFile, File(refFileName), String()));
		else
			hiseSpecialData->includedFiles.add(new ExternalFileData(ExternalFileData::Type::AbsoluteFile, File(), refFileName));

#endif

		return refFileName;
	}

	Statement* parseExternalFile()
	{
		auto refFileName = addExternalFile();

		if (refFileName.isEmpty())
		{
			match(TokenTypes::literal);
			match(TokenTypes::closeParen);
			match(TokenTypes::semicolon);

			return new Statement(location);
		}
		else
		{
			try
			{
				String fileContent = getFileContent(currentValue.toString(), refFileName, true);

                auto ok = preprocessor->process(fileContent, refFileName);
                
                if (!ok.wasOk())
                {
                    CodeLocation loc(fileContent, refFileName);
                    loc.location = loc.program.getCharPointer() + (ok.getErrorMessage().getIntValue()-1);
                    loc.throwError(ok.getErrorMessage().fromFirstOccurrenceOf(":", false, false));
                }
                
				ExpressionTreeBuilder ftb(fileContent, refFileName, preprocessor);

#if ENABLE_SCRIPTING_BREAKPOINTS
				ftb.breakpoints.addArray(breakpoints);
#endif

				ftb.hiseSpecialData = hiseSpecialData;
				ftb.currentNamespace = hiseSpecialData;

				//ftb.setupApiData(*hiseSpecialData, fileContent);

                
                
				ScopedPointer<BlockStatement> s = ftb.parseStatementList();

				match(TokenTypes::literal);
				match(TokenTypes::closeParen);
				match(TokenTypes::semicolon);

				return s.release();
			}
			catch (String &errorMessage)
			{
				hiseSpecialData->includedFiles.getLast()->setErrorMessage(errorMessage);

				throw errorMessage;
			}
		}
	}

	Expression* matchEndOfStatement(Expression* ex)  { ExpPtr e(ex); if (currentType != TokenTypes::eof) match(TokenTypes::semicolon); return e.release(); }
	Expression* matchCloseParen(Expression* ex)      { ExpPtr e(ex); match(TokenTypes::closeParen); return e.release(); }

	Statement* parseIf()
	{
		ScopedPointer<IfStatement> s(new IfStatement(location));
		match(TokenTypes::openParen);
		s->condition = parseExpression();
		match(TokenTypes::closeParen);
		s->trueBranch = parseStatement();
		s->falseBranch = matchIf(TokenTypes::else_) ? parseStatement() : new Statement(location);
		return s.release();
	}

	Statement *parseRegisterAssignment(const Identifier &id)
	{
		match(TokenTypes::identifier);
		match(TokenTypes::assign);

		//const int index = registerIdentifiers.indexOf(id);

		const int index = hiseSpecialData->varRegister.getRegisterIndex(id);

		RegisterAssignment *r = new RegisterAssignment(location, index, parseExpression());

		match(TokenTypes::semicolon);
		return r;
	}

	Statement* parseReturn()
	{
		if (matchIf(TokenTypes::semicolon))
			return new ReturnStatement(location, new Expression(location));

		ReturnStatement* r = new ReturnStatement(location, parseExpression());
		matchIf(TokenTypes::semicolon);
		return r;
	}

	Statement* parseVar()
	{
#if 0
		if (getCurrentNamespace() != hiseSpecialData)
		{
			location.throwError("No var definitions inside namespaces (use reg or const var instead)");
		}
#endif

		if (currentInlineFunction != nullptr)
			location.throwError("Can't declare var statement in inline function");

		ScopedPointer<VarStatement> s(new VarStatement(location));
		s->name = parseIdentifier();

		hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::RootScope, s->name, location);

		s->initialiser = matchIf(TokenTypes::assign) ? parseExpression() : new Expression(location);

		if (matchIf(TokenTypes::comma))
		{
			ScopedPointer<BlockStatement> block(new BlockStatement(location));
			block->statements.add(s.release());
			block->statements.add(parseVar());
			return block.release();
		}

		match(TokenTypes::semicolon);
		return s.release();
	}

	Statement* parseConstVar(JavascriptNamespace* ns)
	{
		matchIf(TokenTypes::var);

		if(currentlyParsingInlineFunction || 
		   currentFunctionObject != nullptr ||
		   currentInlineFunction != nullptr ||
		   outerInlineFunction != nullptr)
		{
			location.throwError("Can't declare const var statement inside function body");
		}

		ScopedPointer<ConstVarStatement> s(new ConstVarStatement(location));

		s->name = parseIdentifier();

		hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::ConstVariables, s->name, location);

		s->initialiser = matchIf(TokenTypes::assign) ? parseExpression() : new Expression(location);

		if (matchIf(TokenTypes::comma))
		{
			ScopedPointer<BlockStatement> block(new BlockStatement(location));
			block->statements.add(s.release());
			block->statements.add(parseVar());
			return block.release();
		}

		jassert(ns->constObjects.contains(s->name));

		static const var uninitialised("uninitialised");
		ns->constObjects.set(s->name, uninitialised); // Will be initialied at runtime
		s->ns = ns;

		ns->comments.set(s->name, lastComment);

		clearLastComment();

		return s.release();
	}

	Statement *parseRegisterVar(JavascriptNamespace* ns, TokenIterator* preparser=nullptr)
	{
		if (preparser)
		{
            auto varType = preparser->matchVarType();
            
			Identifier name = preparser->currentValue.toString();

			ns->varRegister.addRegister(name, var::undefined(), varType);
            ns->registerLocations.add(preparser->createDebugLocation());

			ns->comments.set(name, preparser->lastComment);
			preparser->clearLastComment();

			if (ns->registerLocations.size() != ns->varRegister.getNumUsedRegisters())
			{
				String s;

				if (!ns->id.isNull())
					s << ns->id.toString() << ".";

				s << name << ": error at definition";

				preparser->location.throwError(s);
			}

			return nullptr;
		}
		else
		{
			ScopedPointer<RegisterVarStatement> s(new RegisterVarStatement(location));

            matchVarType();
            
			s->name = parseIdentifier();
			hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::Register, s->name, location);
			s->varRegister = &ns->varRegister;
			s->initialiser = matchIf(TokenTypes::assign) ? parseExpression() : new Expression(location);

			if (matchIf(TokenTypes::comma))
			{
				ScopedPointer<BlockStatement> block(new BlockStatement(location));
				block->statements.add(s.release());
				block->statements.add(parseVar());
				return block.release();
			}

			match(TokenTypes::semicolon);
			return s.release();
		}
	}
	
	Statement* parseGlobalAssignment()
	{
		ScopedPointer<GlobalVarStatement> s(new GlobalVarStatement(location));
		s->name = parseIdentifier();
		
		if (!hiseSpecialData->globals->hasProperty(s->name))
		{
			hiseSpecialData->globals->setProperty(s->name, var::undefined());
		}
		
		s->initialiser = matchIf(TokenTypes::assign) ? parseExpression() : new Expression(location);
		
		if (matchIf(TokenTypes::comma))
		{
			ScopedPointer<BlockStatement> block(new BlockStatement(location));
			block->statements.add(s.release());
			block->statements.add(parseVar());
			return block.release();
		}

		match(TokenTypes::semicolon);
		return s.release();
	}

	Statement* parseLocalAssignment()
	{
		if (InlineFunction::Object::Ptr ifo = dynamic_cast<InlineFunction::Object*>(getCurrentInlineFunction()))
		{
			ScopedPointer<LocalVarStatement> s(new LocalVarStatement(location, ifo.get()));
			s->name = parseIdentifier();
			
			hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::LocalScope, s->name, location);

			ifo->localProperties->set(s->name, {});

			s->initialiser = matchIf(TokenTypes::assign) ? parseExpression() : new Expression(location);

			if (matchIf(TokenTypes::comma))
			{
				ScopedPointer<BlockStatement> block(new BlockStatement(location));
				block->statements.add(s.release());
				block->statements.add(parseVar());
				return block.release();
			}

			match(TokenTypes::semicolon);
			return s.release();
		}
		else if (!currentlyParsedCallback.isNull())
		{
			Callback* callback = hiseSpecialData->getCallback(currentlyParsedCallback);

			ScopedPointer<CallbackLocalStatement> s(new CallbackLocalStatement(location, callback));
			s->name = parseIdentifier();
			
			hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::LocalScope, s->name, location);

			callback->localProperties.set(s->name, var());

			s->initialiser = matchIf(TokenTypes::assign) ? parseExpression() : new Expression(location);

			if (matchIf(TokenTypes::comma))
			{
				ScopedPointer<BlockStatement> block(new BlockStatement(location));
				block->statements.add(s.release());
				block->statements.add(parseVar());
				return block.release();
			}

			match(TokenTypes::semicolon);
			return s.release();
		}

		throwError("Cannot define local variables outside of inline functions or callbacks.");
		RETURN_IF_NO_THROW(nullptr)
	}

	Statement* parseCallback()
	{
		Identifier name = parseIdentifier();

		Callback *c = hiseSpecialData->getCallback(name);

		jassert(c != nullptr);

		match(TokenTypes::openParen);

		for (int i = 0; i < c->getNumArgs(); i++)
		{
			c->parameters[i] = parseIdentifier();
			c->parameterValues[i] = var::undefined();
			
			if (i != c->getNumArgs() - 1) match(TokenTypes::comma);
		}

		match(TokenTypes::closeParen);

		ScopedValueSetter<Identifier> cParser(currentlyParsedCallback, name, Identifier::null);

		ScopedPointer<BlockStatement> s = parseBlock();

		

		c->setStatements(s.release());

		return new Statement(location);
	}

	Statement* parseNamespace()
	{
        auto prevLoc = location;
		Identifier namespaceId = parseIdentifier();

		dispatch::StringBuilder b;
		b << "parse namespace " << namespaceId;
		TRACE_SCRIPTING(DYNAMIC_STRING_BUILDER(b));

        static const Array<Identifier> illegalIds =
        {
            Identifier("Settings"),
            Identifier("Engine"),
            Identifier("Message"),
            Identifier("Server"),
            Identifier("FileSystem"),
            Identifier("Synth"),
            Identifier("Sampler"),
            Identifier("Console")
        };
        
        if(illegalIds.contains(namespaceId))
            prevLoc.throwError("Illegal namespace ID");
        
		currentNamespace = hiseSpecialData->getNamespace(namespaceId);

		if (currentNamespace == nullptr)
		{
            prevLoc.throwError("Error at parsing namespace");
		}

		ScopedPointer<BlockStatement> block = parseBlock();
		
		currentNamespace = hiseSpecialData;

		return block.release();
	}

	Statement* parseFunction()
	{
		Identifier name;

		if (hiseSpecialData->getCallback(currentValue.toString()))
		{
			return parseCallback();
		}

		var fn = parseFunctionDefinition(name);

		

		if (name.isNull())
			throwError("Functions defined at statement-level must have a name");

		ExpPtr nm(new UnqualifiedName(location, name, true)), value(new LiteralValue(location, fn));
		return new Assignment(location, nm, value);
	}

	InlineFunction::Object *getInlineFunction(Identifier &id, JavascriptNamespace* ns=nullptr)
	{
		if (ns == nullptr)
		{
			for (int i = 0; i < hiseSpecialData->inlineFunctions.size(); i++)
			{
				DynamicObject *o = hiseSpecialData->inlineFunctions.getUnchecked(i).get();

				InlineFunction::Object *obj = dynamic_cast<InlineFunction::Object*>(o);

				jassert(obj != nullptr);

				if (obj->name == id) return obj;
			}
		}
		else
		{
			for (int i = 0; i < ns->inlineFunctions.size(); i++)
			{
				DynamicObject *o = ns->inlineFunctions.getUnchecked(i).get();

				InlineFunction::Object *obj = dynamic_cast<InlineFunction::Object*>(o);

				jassert(obj != nullptr);

				if (obj->name == id) return obj;
			}
		}

		return nullptr;
	}

	JavascriptNamespace* getNamespaceForStorageType(JavascriptNamespace::StorageType storageType, JavascriptNamespace* nameSpaceToLook, const Identifier &id)
	{
		switch (storageType)
		{
		case HiseJavascriptEngine::RootObject::JavascriptNamespace::StorageType::Register:
			if (nameSpaceToLook != nullptr && nameSpaceToLook->varRegister.getRegisterIndex(id) != -1) return nameSpaceToLook;
			if (hiseSpecialData->varRegister.getRegisterIndex(id) != -1) return hiseSpecialData;
			break;

		case HiseJavascriptEngine::RootObject::JavascriptNamespace::StorageType::ConstVariable:
			if (nameSpaceToLook != nullptr && nameSpaceToLook->constObjects.contains(id)) return nameSpaceToLook;
			if (hiseSpecialData->constObjects.contains(id)) return hiseSpecialData;
			break;

		case HiseJavascriptEngine::RootObject::JavascriptNamespace::StorageType::InlineFunction:
		{
			if (nameSpaceToLook != nullptr)
			{
				for (int i = 0; i < nameSpaceToLook->inlineFunctions.size(); i++)
				{
					if (dynamic_cast<InlineFunction::Object*>(nameSpaceToLook->inlineFunctions[i].get())->name == id)
						return nameSpaceToLook;
				}
			}

			for (int i = 0; i < hiseSpecialData->inlineFunctions.size(); i++)
			{
				if (dynamic_cast<InlineFunction::Object*>(hiseSpecialData->inlineFunctions[i].get())->name == id)
					return hiseSpecialData;
			}
			break;

		}
			
		case HiseJavascriptEngine::RootObject::JavascriptNamespace::StorageType::numStorageTypes:
			break;
		default:
			break;
		}

		return nullptr;
	}

	
	int getRegisterIndex(const Identifier& id, JavascriptNamespace* ns = nullptr)
	{
		if (ns == nullptr)
		{
			return hiseSpecialData->varRegister.getRegisterIndex(id);
		}
		else
		{
			return ns->varRegister.getRegisterIndex(id);
		}
	}

	var* getRegisterData(int index, JavascriptNamespace* ns = nullptr)
	{
		if (ns == nullptr)
		{
			return hiseSpecialData->varRegister.getVarPointer(index);
		}
		else
		{
			return ns->varRegister.getVarPointer(index);
		}
	}

	int getConstIndex(const Identifier& id, JavascriptNamespace* ns = nullptr)
	{
		if (ns == nullptr)
		{
			return hiseSpecialData->constObjects.indexOf(id);
		}
		else
		{
			return ns->constObjects.indexOf(id);
		}
	}
	
	var* getConstData(int index, JavascriptNamespace* ns = nullptr)
	{
		if (ns == nullptr)
		{
			return hiseSpecialData->constObjects.getVarPointerAt(index);
		}
		else
		{
			return ns->constObjects.getVarPointerAt(index);
		}
	}

	DynamicObject* getCurrentInlineFunction()
	{
		return currentInlineFunction;
	}

	Expression* parseInlineFunctionCall(InlineFunction::Object *obj)
	{
		ScopedPointer<InlineFunction::FunctionCall> f = new InlineFunction::FunctionCall(location, obj);

		parseIdentifier();

		if (currentType == TokenTypes::openParen)
		{
			match(TokenTypes::openParen);

			while (currentType != TokenTypes::closeParen)
			{
				f->addParameter(parseExpression());
				if (currentType != TokenTypes::closeParen)
					match(TokenTypes::comma);
			}

			if (f->numArgs != f->parameterExpressions.size())
			{

				throwError("Inline function call " + obj->name + ": parameter amount mismatch: " + String(f->parameterExpressions.size()) + " (Expected: " + String(f->numArgs) + ")");
			}

			return matchCloseParen(f.release());
		}
		else
		{
			return new LiteralValue(location, var(obj));
		}
	}

	Statement *parseInlineFunction(JavascriptNamespace* ns, TokenIterator *preparser=nullptr)
	{
		if (preparser != nullptr)
		{
			DebugableObject::Location loc = preparser->createDebugLocation();

			preparser->match(TokenTypes::function);
			
            auto returnType = preparser->matchVarType();
            
			Identifier name = preparser->currentValue.toString();
			preparser->match(TokenTypes::identifier);
			preparser->match(TokenTypes::openParen);

			InlineFunction::Object::ArgumentList inlineArguments;

			while (preparser->currentType != TokenTypes::closeParen)
			{
                Identifier pid =preparser->currentValue.toString();
				preparser->match(TokenTypes::identifier);
                
                auto argType = preparser->matchVarType();
                
                inlineArguments.add({argType, pid});
                
				if (preparser->currentType != TokenTypes::closeParen)
					preparser->match(TokenTypes::comma);
			}

			preparser->match(TokenTypes::closeParen);

			ScopedPointer<InlineFunction::Object> o = new InlineFunction::Object(name, inlineArguments);

			o->location = loc;
            o->returnType = returnType;

			ns->inlineFunctions.add(o.release());
			preparser->matchIf(TokenTypes::semicolon);

			return nullptr;
		}
		else
		{
			if (getCurrentInlineFunction() != nullptr) throwError("No nested inline functions allowed.");

			match(TokenTypes::function);

            matchVarType();
            
			Identifier name = parseIdentifier();

			match(TokenTypes::openParen);

			while (currentType != TokenTypes::closeParen) skip();

			match(TokenTypes::closeParen);

			InlineFunction::Object::Ptr o;

			for (int i = 0; i < ns->inlineFunctions.size(); i++)
			{
				if ((o = dynamic_cast<InlineFunction::Object*>(ns->inlineFunctions[i].get())))
				{
					if (o->name == name)
					{
						break;
					}
				}
			}

			currentInlineFunction = o.get();

			if (o != nullptr)
			{
				o->commentDoc = lastComment;
				clearLastComment();

				ScopedPointer<BlockStatement> body = parseBlock();

				o->body = body.release();

				currentInlineFunction = nullptr;

				matchIf(TokenTypes::semicolon);

				return new Statement(location);
			}
			else
			{
				currentInlineFunction = nullptr;

				location.throwError("Error at inline function parsing");

				return nullptr;
			}
		}
	}

	Statement* parseJITModule()
	{

		match(TokenTypes::openParen);


		String refFileName;
		String fileContent = getFileContent(currentValue.toString(), refFileName);

		match(TokenTypes::literal);
		match(TokenTypes::closeParen);
		match(TokenTypes::semicolon);

		return new Statement(location);
	}


	Statement* parseCaseStatement()
	{
		const bool isNotDefaultCase = currentType == TokenTypes::case_;

		ScopedPointer<CaseStatement> s(new CaseStatement(location, isNotDefaultCase));

		skip();

		if (isNotDefaultCase) s->conditions.add(parseExpression());

		match(TokenTypes::colon);

		if (currentType == TokenTypes::openBrace)
		{
			s->body = parseBlock();
		}
		else if (currentType == TokenTypes::case_ || currentType == TokenTypes::default_ || currentType == TokenTypes::closeBrace)
		{
			// Empty statement (the condition will be added to the next case.
			s->body = nullptr;
		}
		else
		{
			s->body = new BlockStatement(location);

			while (currentType != TokenTypes::case_ && currentType != TokenTypes::closeBrace && currentType != TokenTypes::default_)
			{
				s->body->statements.add(parseStatement());
			}
		}

		return s.release();
	}

	Statement* parseSwitchBlock()
	{
		ScopedPointer<SwitchStatement> s(new SwitchStatement(location));

		match(TokenTypes::openParen);
		s->condition = parseExpression();
		match(TokenTypes::closeParen);
		match(TokenTypes::openBrace);

		OwnedArray<Expression> emptyCaseConditions;

		while (currentType == TokenTypes::case_ || currentType == TokenTypes::default_)
		{
			ScopedPointer<CaseStatement> caseStatement = dynamic_cast<CaseStatement*>(parseCaseStatement());

			if (caseStatement != nullptr)
			{
				if (caseStatement->body == nullptr)
				{
					for (auto& c : caseStatement->conditions)
					{
						emptyCaseConditions.add(c.release());
					}

					caseStatement->conditions.clear();

					continue;
				}
				else
				{
					while (!emptyCaseConditions.isEmpty())
					{
						auto c = emptyCaseConditions.removeAndReturn(0);
						caseStatement->conditions.add(c);
					}
				}

				if (caseStatement->isNotDefault)
				{
					s->cases.add(caseStatement.release());
				}
				else
				{
					s->defaultCase = caseStatement.release();
				}
			}
		}

		match(TokenTypes::closeBrace);

		return s.release();
	}

	Statement* parseForLoop()
	{
		match(TokenTypes::openParen);

		const bool isVarInitialiser = matchIf(TokenTypes::var);
		
        if(currentInlineFunction && isVarInitialiser)
        {
            location.throwError("Can't use var initialiser inside inline function");
        }
        
		Expression *iter = parseExpression();

		// Allow unqualified names in for loop initialisation for convenience
		if (auto assignment = dynamic_cast<Assignment*>(iter))
		{
			if (auto un = dynamic_cast<UnqualifiedName*>(assignment->target.get()))
            {
				un->allowUnqualifiedDefinition = true;
                
                ScopedPointer<Expression> newExpression;
                
                auto id = un->getVariableName();
                
                // replace the anonymous initialiser with a local / var assignment
                // in order to prevent global leakage
                
                if(auto fo = dynamic_cast<FunctionObject*>(currentFunctionObject))
                {
                    auto s = new VarStatement(location);
                    s->name = id;

                    hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::RootScope, id, location);

                    s->initialiser.swapWith(assignment->newValue);
                    
                    newExpression = assignment;
                    iter = s;
                }
                else if(auto ifo = dynamic_cast<InlineFunction::Object*>(currentInlineFunction))
                {
                    auto lv = new LocalVarStatement(location, ifo);
                    lv->name = id;
                    
                    hiseSpecialData->checkIfExistsInOtherStorage(HiseSpecialData::VariableStorageType::LocalScope, id, location);
                    
                    ifo->localProperties->set(lv->name, {});
                    lv->initialiser.swapWith(assignment->newValue);
                    
                    newExpression = assignment;
                    iter = lv;
                }
            }
		}

		if (!isVarInitialiser && currentType == TokenTypes::closeParen)
		{
			ScopedPointer<LoopStatement> s(new LoopStatement(location, false, true));

			for (auto& it : currentIterators)
			{
				if (it.loop == nullptr)
				{
					it.loop = s;
					break;
				}
			}

			s->currentIterator = iter;

			s->iterator = nullptr;
			s->initialiser = nullptr;
			s->condition = new LiteralValue(location, true);

			match(TokenTypes::closeParen);

			s->body = parseStatement();

			for (const auto& it: currentIterators)
			{
				if (it.loop == s)
				{
					currentIterators.remove(currentIterators.indexOf(it));
					break;
				}
			}

			return s.release();
		}
		else
		{
			ScopedPointer<LoopStatement> s(new LoopStatement(location, false));

			s->initialiser = matchEndOfStatement(iter);

			if (matchIf(TokenTypes::semicolon))
				s->condition = new LiteralValue(location, true);
			else
			{
				s->condition = parseExpression();
				match(TokenTypes::semicolon);
			}

			if (matchIf(TokenTypes::closeParen))
				s->iterator = new Statement(location);
			else
			{
				s->iterator = parseExpression();
				match(TokenTypes::closeParen);
			}

			s->body = parseStatement();
			return s.release();
		}



	}

	Statement* parseDoOrWhileLoop(bool isDoLoop)
	{
		ScopedPointer<LoopStatement> s(new LoopStatement(location, isDoLoop));
		s->initialiser = new Statement(location);
		s->iterator = new Statement(location);

		if (isDoLoop)
		{
			s->body = parseBlock();
			match(TokenTypes::while_);
		}

		match(TokenTypes::openParen);
		s->condition = parseExpression();
		match(TokenTypes::closeParen);

		if (!isDoLoop)
			s->body = parseStatement();

		return s.release();
	}

	Expression* parseArrowFunction(ExpPtr lhs)
	{
		ScopedPointer<FunctionObject> fo = new FunctionObject();

		fo->location.fileName = location.getCallbackName(true);
		fo->location.charNumber = location.getCharIndex();

		if(auto el = dynamic_cast<ExpressionList*>(lhs.get()))
		{
			for(auto c: el->children)
			{
				if(auto n = dynamic_cast<UnqualifiedName*>(c))
				{
					fo->parameters.add(n->name);
				}
			}
		}
		if(auto n = dynamic_cast<UnqualifiedName*>(lhs.get()))
		{
			fo->parameters.add(n->name);
		}

		if(matchIf(TokenTypes::openBrace))
		{
			fo->body = parseStatementList();
			match(TokenTypes::closeBrace);
		}
		else
		{
			auto returnValue = parseExpression();
			fo->body = new ReturnStatement(location, returnValue);
		}
		
		ExpPtr nm(new UnqualifiedName(location, "unusedArrow", true)), value(new LiteralValue(location, var(fo.release())));
		return new Assignment(location, nm, value);
	}

	var parseFunctionDefinition(Identifier& functionName)
	{
		
		const String::CharPointerType functionStart(location.location);

		if (currentType == TokenTypes::identifier)
			functionName = parseIdentifier();

		

		ScopedPointer<FunctionObject> fo(new FunctionObject());

		fo->location.fileName = location.getCallbackName(true);
		fo->location.charNumber = location.getCharIndex();

		parseFunctionParamsAndBody(*fo);
		fo->functionCode = String(functionStart, location.location);
        fo->createFunctionDefinition(functionName);
		fo->commentDoc = lastComment;
		clearLastComment();

		return var(fo.release());
	}

	Expression* parseFunctionCall(FunctionCall* call, ExpPtr& function)
	{
		ScopedPointer<FunctionCall> s(call);
		s->object = function;
		match(TokenTypes::openParen);

		while (currentType != TokenTypes::closeParen)
		{
			s->arguments.add(parseExpression());
			if (currentType != TokenTypes::closeParen)
				match(TokenTypes::comma);
		}

		return matchCloseParen(s.release());
	}

	Expression* parseApiExpression()
	{
		const Identifier apiId = parseIdentifier();
		const int apiIndex = hiseSpecialData->apiIds.indexOf(apiId);
		ApiClass *apiClass = hiseSpecialData->apiClasses.getUnchecked(apiIndex).get();

		match(TokenTypes::dot);

		const Identifier memberName = parseIdentifier();

		int constantIndex = apiClass->getConstantIndex(memberName);

		if (constantIndex != -1)
		{
			return parseApiConstant(apiClass, memberName);
		}
		else
		{
			return parseApiCall(apiClass, memberName);
		}
	}

	Expression* parseApiConstant(ApiClass *apiClass, const Identifier &constantName)
	{
		const int index = apiClass->getConstantIndex(constantName);

		const var value = apiClass->getConstantValue(index);

		ScopedPointer<ApiConstant> s = new ApiConstant(location);
		s->value = value;

		return s.release();
	}

	Expression* parseApiCall(ApiClass *apiClass, const Identifier &functionName)
	{
		int functionIndex = 0;
		int numArgs = 0;
		apiClass->getIndexAndNumArgsForFunction(functionName, functionIndex, numArgs);

		const String prettyName = apiClass->getObjectName() + "." + functionName.toString();

		if (functionIndex < 0) throwError("Function / constant not found: " + prettyName); // Handle also missing constants here
        
        auto pt = apiClass->getForcedParameterTypes(functionName);
        
		ScopedPointer<ApiCall> s = new ApiCall(location, apiClass, numArgs, functionIndex, pt);

		match(TokenTypes::openParen);

		int numActualArguments = 0;

		while (currentType != TokenTypes::closeParen)
		{
			if (numActualArguments < numArgs)
			{
				s->argumentList[numActualArguments++] = parseExpression();

				if (currentType != TokenTypes::closeParen)
					match(TokenTypes::comma);
			}
			else throwError("Too many arguments in API call " + prettyName + "(). Expected: " + String(numArgs));
		}

		if (numArgs != numActualArguments) throwError("Call to " + prettyName + "(): argument number mismatch : " + String(numActualArguments) + " (Expected : " + String(numArgs) + ")");

		return matchCloseParen(s.release());
	}

	Expression* parseConstExpression(JavascriptNamespace* ns=nullptr)
	{
		const Identifier constId = parseIdentifier();
		const int index = getConstIndex(constId, ns);

#if 0
		if (currentType == TokenTypes::dot)
		{
			match(TokenTypes::dot);
			const Identifier memberName = parseIdentifier();

			return parseConstObjectApiCall(constId, memberName, ns);
		}
#endif

		ns = (ns != nullptr) ? ns : hiseSpecialData;

		return new ConstReference(location, ns, index);
	}

	Expression* parseConstObjectApiCall(const Identifier& objectName, const Identifier& functionName, JavascriptNamespace* ns=nullptr)
	{
		const String prettyName = objectName.toString() + "." + functionName.toString();

		const int index = getConstIndex(objectName, ns);
		var *v = getConstData(index, ns);

		ScopedPointer<ConstObjectApiCall> s = new ConstObjectApiCall(location, v, functionName);

		match(TokenTypes::openParen);

		int numActualArguments = 0;

		while (currentType != TokenTypes::closeParen)
		{
			s->argumentList[numActualArguments++] = parseExpression();

			if (currentType != TokenTypes::closeParen)
				match(TokenTypes::comma);
		}

		return matchCloseParen(s.release());
	}

	Expression* parseSuffixes(Expression* e)
	{
		ExpPtr input(e);

		if (matchIf(TokenTypes::dot))
			return parseSuffixes(new DotOperator(location, input, parseIdentifier()));

		if (currentType == TokenTypes::openParen)
			return parseSuffixes(parseFunctionCall(new FunctionCall(location), input));

		if (matchIf(TokenTypes::openBracket))
		{
			ScopedPointer<ArraySubscript> s(new ArraySubscript(location));
			s->object = input;
			s->index = parseExpression();
			match(TokenTypes::closeBracket);
			return parseSuffixes(s.release());
		}

		if (matchIf(TokenTypes::plusplus))   return parsePostIncDec<AdditionOp>(input);
		if (matchIf(TokenTypes::minusminus)) return parsePostIncDec<SubtractionOp>(input);
		
		return input.release();
	}

	Expression* parseCloseParen(Expression* ex)
	{
		ExpPtr e(ex);

		ExpressionList* ne = nullptr;

		while(!matchIf(TokenTypes::closeParen))
		{
			if(e == ex)
			{
				if(ne == nullptr)
					ne = new ExpressionList(location);

				ne->children.add(e.release());

				e = ne;
			}

			match(TokenTypes::comma);

			ne->children.add(parseExpression());
		}

		return e.release();
	}

	Expression* parseFactor(JavascriptNamespace* ns=nullptr)
	{
		if (currentType == TokenTypes::identifier)
		{
			Identifier id = Identifier(currentValue.toString());

			// Allow direct referencing of namespaced variables within the namespace
			if (getCurrentNamespace() != hiseSpecialData && ns == nullptr)
			{
				ns = getCurrentNamespace();

				// Allow usage of namespace prefix within namespace
				if (ns->id == id)
				{
					match(TokenTypes::identifier);
					match(TokenTypes::dot);
					id = currentValue.toString();
				}
			}

			LoopStatement* iteratorLoop = nullptr;

			for (const auto& it : currentIterators)
			{
				if (it.id == id)
				{
					iteratorLoop = it.loop;
					break;
				}
			}

			if (iteratorLoop != nullptr)
			{
				return parseSuffixes(new LoopStatement::IteratorName(location, iteratorLoop, parseIdentifier()));
			}
			else if (auto ob = dynamic_cast<InlineFunction::Object*>(outerInlineFunction))
			{
				const int inlineParameterIndex = ob->parameterNames.indexOf(id);
				const int localParameterIndex = ob->localProperties->indexOf(id);

				int captureIndex = -1;

				if (auto fo = dynamic_cast<FunctionObject*>(currentFunctionObject))
				{
					captureIndex = fo->getCaptureIndex(id);
				}

				if (captureIndex == -1)
				{
					if (inlineParameterIndex != -1)
						location.throwError("Can't reference inline function parameters in nested function body");

					if (localParameterIndex != -1)
						location.throwError("Can't reference local variables in nested function body");
				}
			}
			else if (auto ob = dynamic_cast<InlineFunction::Object*>(currentInlineFunction))
			{
				

				const int inlineParameterIndex = ob->parameterNames.indexOf(id);
				const int localParameterIndex = ob->localProperties->indexOf(id);

				if (inlineParameterIndex >= 0)
				{
					parseIdentifier();
					return parseSuffixes(new InlineFunction::ParameterReference(location, ob, inlineParameterIndex));
				}
				if (localParameterIndex >= 0)
				{
					parseIdentifier();
					return parseSuffixes(new LocalReference(location, ob, id));
				}
			}

			// Only resolve one level of namespaces
			JavascriptNamespace* namespaceForId = hiseSpecialData->getNamespace(id);

			if (namespaceForId != nullptr)
			{
				match(TokenTypes::identifier);
				match(TokenTypes::dot);
				
				return parseFactor(namespaceForId);
			}
			else
			{
                if(auto fo = dynamic_cast<FunctionObject*>(currentFunctionObject))
                {
                    for(auto cl : fo->capturedLocals)
                    {
                        if(cl->getVariableName() == id)
                        {
                            return parseSuffixes(new UnqualifiedName(location, parseIdentifier(), false));
                        }
                    }
                }
                
                
				if (JavascriptNamespace* inlineNamespace = getNamespaceForStorageType(JavascriptNamespace::StorageType::InlineFunction, ns, id))
				{
					InlineFunction::Object *obj = getInlineFunction(id, inlineNamespace);
					return parseSuffixes(parseInlineFunctionCall(obj));
				}
				else if (JavascriptNamespace* constNamespace = getNamespaceForStorageType(JavascriptNamespace::StorageType::ConstVariable, ns, id))
				{
					return parseSuffixes(parseConstExpression(constNamespace));
				}
				else if (JavascriptNamespace* regNamespace = getNamespaceForStorageType(JavascriptNamespace::StorageType::Register, ns, id))
				{
					VarRegister* rootRegister = &regNamespace->varRegister;

					const int registerIndex = rootRegister->getRegisterIndex(id);
                    
                    auto type = rootRegister->getRegisterVarType(registerIndex);

					return parseSuffixes(new RegisterName(location, parseIdentifier(), rootRegister, registerIndex, getRegisterData(registerIndex, regNamespace), type));
				}

				const int apiClassIndex = hiseSpecialData->apiIds.indexOf(id);
				const int globalIndex = hiseSpecialData->globals != nullptr ? hiseSpecialData->globals->getProperties().indexOf(id) : -1;
				
				if (apiClassIndex != -1)
				{
					return parseSuffixes(parseApiExpression());
				}
				else if (globalIndex != -1)
				{
					return parseSuffixes(new GlobalReference(location, hiseSpecialData->globals.get(), parseIdentifier()));
				}
				else
				{
					if (!currentlyParsedCallback.isNull())
					{
						Callback *c = hiseSpecialData->getCallback(currentlyParsedCallback);

						if (c != nullptr)
						{
							var* callbackParameter = c->getVarPointer(id);

							if (callbackParameter != nullptr)
							{
								parseIdentifier();
								return parseSuffixes(new CallbackParameterReference(location, callbackParameter));
							}

							var* localParameter = c->localProperties.getVarPointer(id);

							if (localParameter != nullptr)
							{
								auto name = parseIdentifier();

								return parseSuffixes(new CallbackLocalReference(location, c, name));
							}
						}
						else
						{
							jassertfalse;
						}
					}

					return parseSuffixes(new UnqualifiedName(location, parseIdentifier(), false));
				}
			}
		}

		auto prevLocation = location;

		if (matchIf(TokenTypes::openParen))        return parseSuffixes(parseCloseParen(parseExpression()));
		if (matchIf(TokenTypes::true_))            return parseSuffixes(new LiteralValue(prevLocation, (int)1));
		if (matchIf(TokenTypes::false_))           return parseSuffixes(new LiteralValue(prevLocation, (int)0));
		if (matchIf(TokenTypes::null_))            return parseSuffixes(new LiteralValue(prevLocation, var()));
		if (matchIf(TokenTypes::undefined))        return parseSuffixes(new Expression(prevLocation));

		if (currentType == TokenTypes::literal)
		{
			var v(currentValue); skip();
			return parseSuffixes(new LiteralValue(prevLocation, v));
		}

		if (matchIf(TokenTypes::openBrace))
		{
			ScopedPointer<ObjectDeclaration> e(new ObjectDeclaration(location));

			while (currentType != TokenTypes::closeBrace)
			{
				e->names.add(currentValue.toString());
				match((currentType == TokenTypes::literal && currentValue.isString())
					? TokenTypes::literal : TokenTypes::identifier);
				match(TokenTypes::colon);
				e->initialisers.add(parseExpression());

				if (currentType != TokenTypes::closeBrace)
					match(TokenTypes::comma);
			}

			match(TokenTypes::closeBrace);
			return parseSuffixes(e.release());
		}

		if (matchIf(TokenTypes::openBracket))
		{
			ScopedPointer<ArrayDeclaration> e(new ArrayDeclaration(location));

			while (currentType != TokenTypes::closeBracket)
			{
				e->values.add(parseExpression());

				if (currentType != TokenTypes::closeBracket)
					match(TokenTypes::comma);
			}

			match(TokenTypes::closeBracket);
			return parseSuffixes(e.release());
		}

		if (matchIf(TokenTypes::function))
		{
			Identifier name;
			var fn = parseFunctionDefinition(name);

			if (name.isValid())
				throwError("Inline functions definitions cannot have a name");

			if (auto fo = dynamic_cast<FunctionObject*>(fn.getDynamicObject()))
			{
				if (!fo->capturedLocals.isEmpty())
					return new AnonymousFunctionWithCapture(location, fn);
			}

			return new LiteralValue(location, fn);
		}

		if (matchIf(TokenTypes::new_))
		{
			return parseNewOperator();
		}

		if (matchIf(TokenTypes::isDefined_))
		{
			return parseIsDefined();
		}

		throwError("Found " + getTokenName(currentType) + " when expecting an expression");
		RETURN_IF_NO_THROW(nullptr);
	}

	template <typename OpType>
	Expression* parsePreIncDec()
	{
		Expression* e = parseFactor(); // careful - bare pointer is deliberately alised
		ExpPtr lhs(e), one(new LiteralValue(location, (int)1));
		return new SelfAssignment(location, e, new OpType(location, lhs, one));
	}

	template <typename OpType>
	Expression* parsePostIncDec(ExpPtr& lhs)
	{
		Expression* e = lhs.release(); // careful - bare pointer is deliberately alised
		ExpPtr lhs2(e), one(new LiteralValue(location, (int)1));
		return new PostAssignment(location, e, new OpType(location, lhs2, one));
	}

	Expression* parseTypeof()
	{
		ScopedPointer<FunctionCall> f(new FunctionCall(location));
		f->object = new UnqualifiedName(location, "typeof", true);
		f->arguments.add(parseUnary());
		return f.release();
	}

	Expression* parseUnary()
	{
		if (matchIf(TokenTypes::minus))       { ExpPtr a(new LiteralValue(location, (int)0)), b(parseUnary()); return new SubtractionOp(location, a, b); }
		if (matchIf(TokenTypes::logicalNot))  { ExpPtr a(new LiteralValue(location, (int)0)), b(parseUnary()); return new EqualsOp(location, a, b); }
		if (matchIf(TokenTypes::plusplus))    return parsePreIncDec<AdditionOp>();
		if (matchIf(TokenTypes::minusminus))  return parsePreIncDec<SubtractionOp>();
		if (matchIf(TokenTypes::typeof_))     return parseTypeof();

		return parseFactor();
	}

	Expression* parseNewOperator()
	{
		location.throwError("new is not supported anymore");
		RETURN_IF_NO_THROW(nullptr);
	}

	Expression* parseIsDefined()
	{
		match(TokenTypes::openParen);

		ExpPtr a(parseExpression());

		match(TokenTypes::closeParen);

		return new IsDefinedTest(location, a.release());
	}

	Expression* parseMultiplyDivide()
	{
		ExpPtr a(parseUnary());

		for (;;)
		{
			if (matchIf(TokenTypes::times))        { ExpPtr b(parseUnary()); a = new MultiplyOp(location, a, b); }
			else if (matchIf(TokenTypes::divide))  { ExpPtr b(parseUnary()); a = new DivideOp(location, a, b); }
			else if (matchIf(TokenTypes::modulo))  { ExpPtr b(parseUnary()); a = new ModuloOp(location, a, b); }
			else break;
		}

		return a.release();
	}

	Expression* parseAdditionSubtraction()
	{
		ExpPtr a(parseMultiplyDivide());

		for (;;)
		{
			if (matchIf(TokenTypes::plus))            { ExpPtr b(parseMultiplyDivide()); a = new AdditionOp(location, a, b); }
			else if (matchIf(TokenTypes::minus))      { ExpPtr b(parseMultiplyDivide()); a = new SubtractionOp(location, a, b); }
			else break;
		}

		return a.release();
	}

	Expression* parseShiftOperator()
	{
		ExpPtr a(parseAdditionSubtraction());

		for (;;)
		{
			if (matchIf(TokenTypes::leftShift))                { ExpPtr b(parseExpression()); a = new LeftShiftOp(location, a, b); }
			else if (matchIf(TokenTypes::rightShift))          { ExpPtr b(parseExpression()); a = new RightShiftOp(location, a, b); }
			else if (matchIf(TokenTypes::rightShiftUnsigned))  { ExpPtr b(parseExpression()); a = new RightShiftUnsignedOp(location, a, b); }
			else break;
		}

		return a.release();
	}

	Expression* parseComparator()
	{
		ExpPtr a(parseShiftOperator());

		for (;;)
		{
			if (matchIf(TokenTypes::equals))                  { ExpPtr b(parseShiftOperator()); a = new EqualsOp(location, a, b); }
			else if (matchIf(TokenTypes::notEquals))          { ExpPtr b(parseShiftOperator()); a = new NotEqualsOp(location, a, b); }
			else if (matchIf(TokenTypes::typeEquals))         { ExpPtr b(parseShiftOperator()); a = new TypeEqualsOp(location, a, b); }
			else if (matchIf(TokenTypes::typeNotEquals))      { ExpPtr b(parseShiftOperator()); a = new TypeNotEqualsOp(location, a, b); }
			else if (matchIf(TokenTypes::lessThan))           { ExpPtr b(parseShiftOperator()); a = new LessThanOp(location, a, b); }
			else if (matchIf(TokenTypes::lessThanOrEqual))    { ExpPtr b(parseShiftOperator()); a = new LessThanOrEqualOp(location, a, b); }
			else if (matchIf(TokenTypes::greaterThan))        { ExpPtr b(parseShiftOperator()); a = new GreaterThanOp(location, a, b); }
			else if (matchIf(TokenTypes::greaterThanOrEqual)) { ExpPtr b(parseShiftOperator()); a = new GreaterThanOrEqualOp(location, a, b); }
			else break;
		}

		return a.release();
	}

	Expression* parseLogicOperator()
	{
		ExpPtr a(parseComparator());

		for (;;)
		{
			if (matchIf(TokenTypes::logicalAnd))       { ExpPtr b(parseComparator()); a = new LogicalAndOp(location, a, b); }
			else if (matchIf(TokenTypes::logicalOr))   { ExpPtr b(parseComparator()); a = new LogicalOrOp(location, a, b); }
			else if (matchIf(TokenTypes::bitwiseAnd))  { ExpPtr b(parseComparator()); a = new BitwiseAndOp(location, a, b); }
			else if (matchIf(TokenTypes::bitwiseOr))   { ExpPtr b(parseComparator()); a = new BitwiseOrOp(location, a, b); }
			else if (matchIf(TokenTypes::bitwiseXor))  { ExpPtr b(parseComparator()); a = new BitwiseXorOp(location, a, b); }
			else break;
		}

		return a.release();
	}

	Expression* parseTerneryOperator(ExpPtr& condition)
	{
		ScopedPointer<ConditionalOp> e(new ConditionalOp(location));
		e->condition = condition;
		e->trueBranch = parseExpression();
		match(TokenTypes::colon);
		e->falseBranch = parseExpression();
		return e.release();
	}

	


	


	Array<Identifier> registerIdentifiers;

	struct IteratorData
	{
		bool operator==(const IteratorData& other) const
		{
			return loop == other.loop;
		}

		LoopStatement* loop = nullptr;
		Identifier id;
	};

	Array<IteratorData> currentIterators;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpressionTreeBuilder)
};


void HiseJavascriptEngine::RootObject::ExpressionTreeBuilder::preprocessCode(const String& codeToPreprocess, const String& externalFileName)
{
	if (codeToPreprocess.isEmpty()) return;

	dispatch::StringBuilder b;
	b << "preprocess " << externalFileName;
	TRACE_SCRIPTING(DYNAMIC_STRING_BUILDER(b));

	static const var undeclared("undeclared");

	JavascriptNamespace* rootNamespace = hiseSpecialData;
	JavascriptNamespace* cns = rootNamespace;
	TokenIterator it(codeToPreprocess, externalFileName);

	int braceLevel = 0;

	while (it.currentType != TokenTypes::eof)
	{
		if (it.currentType == TokenTypes::namespace_)
		{
			if (cns != rootNamespace)
			{
				it.location.throwError("Nesting of namespaces is not allowed");
			}

			it.match(TokenTypes::namespace_);
			Identifier namespaceId = Identifier(it.currentValue);

			if (hiseSpecialData->getNamespace(namespaceId) == nullptr)
			{
				ScopedPointer<JavascriptNamespace> newNamespace = new JavascriptNamespace(namespaceId);
				newNamespace->namespaceLocation = it.createDebugLocation();
				cns = newNamespace;
				hiseSpecialData->namespaces.add(newNamespace.release());
				continue;
			}
			else
			{
				it.location.throwError("Duplicate namespace " + namespaceId.toString());
			}
		}

		// Skip extern "C" functions
		if (it.currentType == TokenTypes::extern_)
		{
			while (!(it.currentType == TokenTypes::closeBrace && braceLevel == 1) && 
				   !(it.currentType == TokenTypes::eof))
			{
				if (it.currentType == TokenTypes::openBrace) braceLevel++;
				else if (it.currentType == TokenTypes::closeBrace) braceLevel--;

				it.skip();
			}
		}

		// Search in included files
		if (it.currentType == TokenTypes::include_)
		{
			it.match(TokenTypes::include_);
			it.match(TokenTypes::openParen);
			String fileName = it.currentValue.toString();
			String externalCode = getFileContent(it.currentValue.toString(), fileName);
			
            
            
			preprocessCode(externalCode, fileName);

			continue;
		}

		// Handle the brace level
		if (it.matchIf(TokenTypes::openBrace))
		{
			braceLevel++;
			continue;
		}
		else if (it.matchIf(TokenTypes::closeBrace))
		{
			braceLevel--;
			if (braceLevel == 0 && (rootNamespace != cns))
			{
				cns = rootNamespace;
			}
			
			continue;
		}

		if (it.matchIf(TokenTypes::inline_))
		{	
			parseInlineFunction(cns, &it);
			continue;
		}

		if (it.matchIf(TokenTypes::register_var))
		{
			parseRegisterVar(cns, &it);
			continue;
		}

		// Handle the keyword
		if (it.currentType == TokenTypes::const_)
		{
			it.match(TokenTypes::const_);
			it.matchIf(TokenTypes::var);

			const Identifier newId(it.currentValue);

			if ((rootNamespace == cns) && braceLevel != 0) it.location.throwError("const var declaration must be on global level");
			if (newId.isNull())					  it.location.throwError("Expected identifier for const var declaration");
			if (cns->constObjects.contains(newId))			  it.location.throwError("Duplicate const var declaration.");

			cns->constObjects.set(newId, undeclared);
			cns->constLocations.add(it.createDebugLocation());

			continue;
		}
		else
		{
			it.skip();
		}
	}

	if (rootNamespace != cns)
	{
		it.location.throwError("Parsing error (open namespace)");
	}

	if (cns->constObjects.size() != cns->constLocations.size())
	{
		jassertfalse;
	}
}

String HiseJavascriptEngine::RootObject::ExpressionTreeBuilder::removeUnneededNamespaces(int& counter)
{
	StringArray namespaces;
	
	while (currentType != TokenTypes::eof)
	{
		if (currentType != TokenTypes::namespace_) skip();
		else
		{
			auto start = location.location;

			match(TokenTypes::namespace_);
			match(TokenTypes::identifier);
			skipBlock();
			matchIf(TokenTypes::semicolon);

			auto end = location.location;
			
			namespaces.add(String(start, end));
		}
	}

	String returnCode = location.program;

	for (int i = namespaces.size() - 1; i >= 0; i--)
	{
		const String namespaceId = RegexFunctions::getFirstMatch("namespace\\s+(\\w+)", namespaces[i])[1];
		const String remainingCode = returnCode.fromFirstOccurrenceOf(namespaces[i], false, false);
		TokenIterator it(remainingCode, "");
		bool found = false;

		while (it.currentType != TokenTypes::eof)
		{
			if (it.currentType == TokenTypes::identifier && it.currentValue == namespaceId)
			{
				found = true;
				break;
			}

			it.skip();
		}
		
		if (!found)
		{
			returnCode = returnCode.replace(namespaces[i], "");
			counter++;
		}
	}

	return returnCode;
}

String HiseJavascriptEngine::RootObject::ExpressionTreeBuilder::uglify()
{
	String uglyCode;

	int tokenCounter = 0;

	while (currentType != TokenTypes::eof)
	{
		if (currentType == TokenTypes::in) uglyCode << ' '; // the only keyword that needs a leading space...

		if (currentType == TokenTypes::identifier)
		{
			uglyCode << currentValue.toString();
		}
		else if (currentType == TokenTypes::literal)
		{
			if (currentValue.isString())
			{
				uglyCode << "\"" << currentValue.toString().replace("\n", "\\n") << "\"";
			}
			else
			{
				uglyCode << currentValue.toString();
			}
		}
		else
		{
			uglyCode << currentType;
		}

		if (currentType == TokenTypes::namespace_ ||
			currentType == TokenTypes::function ||
			currentType == TokenTypes::const_ || 
			currentType == TokenTypes::extern_ ||
			currentType == TokenTypes::case_ ||
			currentType == TokenTypes::const_ ||
			currentType == TokenTypes::local_ ||
			currentType == TokenTypes::var ||
			currentType == TokenTypes::in ||
			currentType == TokenTypes::inline_ ||
			currentType == TokenTypes::return_ ||
			currentType == TokenTypes::typeof_ ||
			currentType == TokenTypes::register_var ||
			currentType == TokenTypes::new_ ||
			currentType == TokenTypes::else_ ||
			currentType == TokenTypes::global_)
		{
			uglyCode << ' ';
		}

		tokenCounter++;

		if (tokenCounter % 256 == 0) uglyCode << NewLine::getDefault(); // one single line is too slow for the code editor...

		skip();
	}

	return uglyCode;
}


var HiseJavascriptEngine::RootObject::evaluate(const String& code)
{
	ExpressionTreeBuilder tb(code, String(), preprocessor);
	tb.setupApiData(hiseSpecialData, code);
    
	auto& cp = currentLocalScopeCreator.get();

    DynamicObject::Ptr localScope = cp != nullptr ? cp->createScope(this) : nullptr;
    
    if(localScope == nullptr)
        localScope = this;
    else
    {
        for(const auto& x: getProperties())
            localScope->setProperty(x.name, x.value);
    }
    
	return ExpPtr(tb.parseExpression())->getResult(Scope(nullptr, this, localScope.get()));
}

bool HiseJavascriptEngine::RootObject::areTypeEqual(const var& a, const var& b)
{
	return a.hasSameTypeAs(b) && isFunction(a) == isFunction(b)
		&& (((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid())) || a == b);
}

void HiseJavascriptEngine::RootObject::execute(const String& code, bool allowConstDeclarations)
{
	ExpressionTreeBuilder tb(code, String(), preprocessor);

#if ENABLE_SCRIPTING_BREAKPOINTS
	tb.breakpoints.swapWith(breakpoints);
#endif

	tb.setupApiData(hiseSpecialData, allowConstDeclarations ? code : String());

	ScopedPointer<BlockStatement> sl;

	{
		TRACE_SCRIPTING("parse script");
		sl = tb.parseStatementList();
	}
	
	
	if(shouldUseCycleCheck)
		prepareCycleReferenceCheck();

	{
		TRACE_SCRIPTING("run onInit callback");
		sl->perform(Scope(nullptr, this, this), nullptr);
	}

	

	Array<OptimizationPass::OptimizationResult> results;

	auto before = Time::getMillisecondCounter();

	for (auto o : hiseSpecialData.optimizations)
	{
		if(auto or_ = hiseSpecialData.runOptimisation(o))
			results.add(or_);
	}
	
	auto after = Time::getMillisecondCounter();
	
	auto optimisationTimeMs = after - before;
	
	if (!results.isEmpty())
	{
		String s;

		for (auto r : results)
			s << r.passName << ": " << String(r.numOptimizedStatements) << "\n";

		s << "Optimization Duration: " << String(optimisationTimeMs) << "ms";

		hiseSpecialData.processor->setOptimisationReport(s);
	}
}

HiseJavascriptEngine::RootObject::FunctionObject::FunctionObject(const FunctionObject& other) : DynamicObject(), functionCode(other.functionCode)
{
	ExpressionTreeBuilder tb(functionCode, String(), nullptr);

	tb.parseFunctionParamsAndBody(*this);
}

} // namespace hise

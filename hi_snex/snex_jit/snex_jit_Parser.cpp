namespace snex {
namespace jit {
using namespace juce;

int Compiler::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	auto c = source.nextChar();

	if (c == 'P')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::PassMessage;
	}
	if (c == 'W')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Warning;
	}
	if (c == 'E')
	{
		source.skipToEndOfLine(); return BaseCompiler::MessageType::Error;
	}
	if (c == '-')
	{
		c = source.nextChar();

		source.skipToEndOfLine();

		if (c == '-')
			return BaseCompiler::MessageType::VerboseProcessMessage;
		else
			return BaseCompiler::MessageType::ProcessMessage;
	}

	return BaseCompiler::MessageType::ProcessMessage;
}


class ClassScope : public FunctionClass,
				   public BaseScope
{
public:

	

	ClassScope(GlobalScope& parent) :
		FunctionClass({}),
		BaseScope({}, &parent)
	{
		parent.setCurrentClassScope(this);

		parent.getBufferHandler().reset();

		jassert(scopeType == BaseScope::Class);

		runtime = new asmjit::JitRuntime();

		addFunctionClass(new MessageFunctions());
		addFunctionClass(new MathFunctions());
		addFunctionClass(new BlockFunctions());
	};

	ScopedPointer<asmjit::JitRuntime> runtime;

	struct FunctionDebugInfo : public DebugInformationBase
	{
		FunctionDebugInfo(jit::FunctionData* d):
			type(d->returnType),
			full(d->getSignature()),
			name(d->id.toString())
		{

		}

		int getType() const override
		{
			return ApiHelpers::DebugObjectTypes::LocalFunction;
		}

		juce::String getCategory() const override
		{
			return "Local function";
		}

		Types::ID type;
		juce::String name;
		juce::String full;

		virtual juce::String getTextForName() const
		{
			return full;
		}

		virtual juce::String getTextForType() const { return "function"; }

		virtual juce::String getTextForDataType() const
		{
			return Types::Helpers::getTypeName(type);
		}

		virtual juce::String getTextForValue() const
		{
			return "";
		}

		virtual juce::String getCodeToInsert() const
		{
			return full.fromFirstOccurrenceOf(" ", false, false);
		}

	};

	struct LocalVariableInfo
	{
		bool isParameter;
		Identifier id;
		Types::ID type;
	};

	struct LocalVariableDebugObject : public DebugableObjectBase
	{
		LocalVariableDebugObject(LocalVariableInfo info_, GlobalScope* s_):
			s(s_),
			info(info_)
		{}

		GlobalScope* s = nullptr;
		LocalVariableInfo info;

		Identifier getObjectName() const override 
		{
			return info.id;
		}

		int getTypeNumber() const override
		{
			return info.type;
		}

		juce::String getDebugDataType() const override
		{
			return Types::Helpers::getTypeName(info.type);
		}

		juce::String getDebugValue() const override
		{
			if (s != nullptr)
			{
				auto entry = s->getBreakpointHandler().getEntry(getObjectName());

				if (entry.isUsed)
					return Types::Helpers::getCppValueString(entry.currentValue);
				else
					return "unknown";
			}
		}
		juce::String getCategory() const override { return info.isParameter ? "Parameter" : "Local variable"; };
	};

	struct BufferDummy : public ManualDebugObject
	{

		BufferDummy():
			ManualDebugObject()
		{
			childNames.add("create");
			childNames.add("getAudioFile");
			childNames.add("getTableData");
			category = "API call";
			objectName = "Buffer";
		}

		int getTypeNumber() const override { return ApiHelpers::DebugObjectTypes::ApiCall; }
		
		void fillInfo(SettableDebugInfo* childInfo, const juce::String& id) override
		{
			if (id == "create")
			{
				childInfo->category = "API call";
				childInfo->codeToInsert = "Buffer.create(int size)";
				childInfo->name = "Buffer.create(int size)";
				childInfo->typeValue = Types::ID::Block;
				childInfo->description.append("\nCreates a empty buffer with the given size", GLOBAL_BOLD_FONT());
			}
			if (id == "getAudioFile")
			{
				childInfo->category = "API call";
				childInfo->codeToInsert = "Buffer.getAudioFile(int fileIndex)";
				childInfo->name = "Buffer.getAudioFile(int fileIndex)";
				childInfo->typeValue = Types::ID::Block;
				childInfo->description.append("\nreturns a wrapped block for the given audio file", GLOBAL_BOLD_FONT());
			}
			if (id == "getTableData")
			{
				childInfo->category = "API call";
				childInfo->codeToInsert = "Buffer.getTableData(int tableIndex)";
				childInfo->name = "Buffer.getTableData(int tableIndex)";
				childInfo->typeValue = Types::ID::Block;
				childInfo->description.append("\nreturns a wrapped block for the given look up table", GLOBAL_BOLD_FONT());
			}
		}
	};

	

	void addLocalVariableInfo(const LocalVariableInfo& l)
	{
		localVariableInfo.add(l);
	}

	juce::Array<LocalVariableInfo> localVariableInfo;

	void addRegisteredFunctionClasses(OwnedArray<DebugInformationBase>& list, int typeNumber= ApiHelpers::DebugObjectTypes::ApiCall)
	{
		for (auto f : registeredClasses)
			list.add(new ObjectDebugInformation(f, typeNumber));
	}

	void createDebugInfo(OwnedArray<DebugInformationBase>& list)
	{
		list.clear();

		auto addType = [&list](const juce::String& name, const juce::String& description, Types::ID type)
		{
			auto wblockInfo = new SettableDebugInfo();
			wblockInfo->codeToInsert = name;
			wblockInfo->name = name;
			wblockInfo->description.append("\n" + description, GLOBAL_BOLD_FONT(), Colours::black);
			wblockInfo->dataType = name;
			wblockInfo->value = "";
			wblockInfo->typeValue = type;
			wblockInfo->category = "Basic Type";

			list.add(wblockInfo);
		};

		addType("int", "A 32bit integer type", Types::ID::Integer);
		addType("bool", "A boolean type (true | false)", Types::ID::Integer);
		addType("float", "A 32bit floating point type", Types::ID::Float);
		addType("sfloat", "A smoothed floating point value ", Types::ID::Float);
		addType("double", "A 64bit floating point type", Types::ID::Double);
		addType("sdouble", "A smoothed 64bit floating point value", Types::ID::Double);
		addType("block", "A wrapper around a array of float numbers", Types::ID::Block);
		addType("wblock", "A buffer object with index wrapping", Types::ID::Block);
		addType("zblock", "A buffer object with zero wrapping", Types::ID::Block);
		addType("event", "The HISE event", Types::ID::Event);
		
		auto forInfo = new SettableDebugInfo();
		forInfo->codeToInsert = "for (auto& s : channel)\n\t{\n\t\t\n\t}\n\t";;
		forInfo->name = "for-loop";
		forInfo->description.append("\nA range based for loop over a block", GLOBAL_BOLD_FONT(), Colours::black);
		forInfo->dataType = "float";
		forInfo->value = "";
		forInfo->category = "Template";
		forInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		auto channelInfo = new SettableDebugInfo();
		channelInfo->codeToInsert = "void processChannel(block channel, int channelIndex)\n{\n\t\n}\n";;
		channelInfo->name = "processChannel";
		channelInfo->description.append("\nBlock based channel processing", GLOBAL_BOLD_FONT(), Colours::black);
		channelInfo->value = "";
		channelInfo->category = "Template";
		channelInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(channelInfo);

		auto sampleInfo = new SettableDebugInfo();
		sampleInfo->codeToInsert = "float processSample(float input)\n{\n\treturn input;\n}\n";;
		sampleInfo->name = "processSample";
		sampleInfo->description.append("\nMono (or stateless) sample processing", GLOBAL_BOLD_FONT(), Colours::black);
		sampleInfo->value = "";
		sampleInfo->category = "Template";
		sampleInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(sampleInfo);

		auto setInfo = new SettableDebugInfo();
		setInfo->codeToInsert = "void setParameterX(double value)\n{\n\t\n}\n";;
		setInfo->name = "setParameter";
		setInfo->description.append("\nParameter callback template", GLOBAL_BOLD_FONT(), Colours::black);
		setInfo->value = "";
		setInfo->category = "Template";
		setInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(setInfo);

		auto frameInfo = new SettableDebugInfo();
		frameInfo->codeToInsert = "void processFrame(block frame)\n{\n\t\n}\n";;
		frameInfo->name = "processFrame";
		frameInfo->description.append("\nInterleaved frame processing callback", GLOBAL_BOLD_FONT(), Colours::black);
		frameInfo->value = "";
		frameInfo->category = "Template";
		frameInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(frameInfo);

		list.add(forInfo);

		for (auto l : localVariableInfo)
		{
			if (hasVariable(l.id))
				continue;

			auto lInfo = new LocalVariableDebugObject(l, dynamic_cast<GlobalScope*>(getParent()));
			list.add(new ObjectDebugInformation(lInfo));
		}

		for (auto f : functions)
		{
			list.add(new FunctionDebugInfo(f));
		}

		if (auto gs = dynamic_cast<GlobalScope*>(getParent()))
		{
			list.add(new ObjectDebugInformation(gs->getGlobalFunctionClass("Console"), ApiHelpers::DebugObjectTypes::ApiCall));
		}

		
		addRegisteredFunctionClasses(list);
		

		for (auto v : referencedVariables)
		{
			list.add(new ObjectDebugInformation(v));
		}

		list.add(ManualDebugObject::create<BufferDummy>());
	}

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassScope);
};

class ClassCompiler final: public BaseCompiler
{
public:

	ClassCompiler(GlobalScope& memoryPool_) :
		BaseCompiler(),
		memoryPool(memoryPool_),
		lastResult(Result::ok())
	{
		const auto& optList = memoryPool.getOptimizationPassList();

		if (!optList.isEmpty())
		{
			OptimizationFactory f;

			for (const auto& id : optList)
				addOptimization(f.createOptimization(id));
		}

		newScope = new JitCompiledFunctionClass(memoryPool);
	};


	void setFunctionCompiler(asmjit::X86Compiler* cc)
	{
		asmCompiler = cc;
	}

	asmjit::Runtime* getRuntime()
	{
		return newScope->pimpl->runtime;
	}

	JitCompiledFunctionClass* compileAndGetScope(const juce::String& code)
	{
		NewClassParser parser(this, code);

		if(newScope == nullptr)
			newScope = new JitCompiledFunctionClass(memoryPool);

		try
		{
			setCurrentPass(BaseCompiler::Parsing);
			syntaxTree = parser.parseStatementList();
			executePass(PreSymbolOptimization, newScope->pimpl, syntaxTree);
			executePass(ResolvingSymbols, newScope->pimpl, syntaxTree);
			executePass(TypeCheck, newScope->pimpl, syntaxTree);
			executePass(PostSymbolOptimization, newScope->pimpl, syntaxTree);
			executePass(FunctionParsing, newScope->pimpl, syntaxTree);

			// Optimize now

			executePass(FunctionCompilation, newScope->pimpl, syntaxTree);

			lastResult = Result::ok();
		}
		catch (ParserHelpers::CodeLocation::Error& e)
		{
			
			syntaxTree = nullptr;

			logMessage(BaseCompiler::Error, e.toString());
			lastResult = Result::fail(e.toString());
		}

		return newScope.release();
	}

	Result getLastResult() { return lastResult; }

	ScopedPointer<JitCompiledFunctionClass> newScope;

	asmjit::X86Compiler* asmCompiler;

	juce::String assembly;

	Result lastResult;

	GlobalScope& memoryPool;

	ScopedPointer<SyntaxTree> syntaxTree;
};


SyntaxTree* BlockParser::parseStatementList()
{
	matchIf(JitTokens::openBrace);

	compiler->logMessage(BaseCompiler::ProcessMessage, "Parsing statements");

	ScopedPointer<SyntaxTree> list = new SyntaxTree(location);

	while (currentType != JitTokens::eof && currentType != JitTokens::closeBrace)
	{
		auto s = parseStatement();
		list->addStatement(s);
	}

	matchIf(JitTokens::closeBrace);

	finaliseSyntaxTree(list);

	return list.release();
}

BlockParser::StatementPtr NewClassParser::parseStatement()
{
	bool isConst = matchIf(JitTokens::const_);

	bool isSmoothedType = isSmoothedVariableType();
	bool isWrappedBlType = isWrappedBlockType();

	currentHnodeType = matchType();
	
	matchIfSymbol();

	StatementPtr s;

	if (matchIf(JitTokens::openParen))
	{
		compiler->logMessage(BaseCompiler::ProcessMessage, "Adding function " + currentSymbol.toString());
		s = parseFunction();
		matchIf(JitTokens::semicolon);
	}
	else
	{
		if (isSmoothedType)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding smoothed variable " + currentSymbol.toString());
			s = parseSmoothedVariableDefinition();
			match(JitTokens::semicolon);
		}
		else if (isWrappedBlType)
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding wrapped block " + currentSymbol.toString());
			s = parseWrappedBlockDefinition();
			match(JitTokens::semicolon);
		}
		else
		{
			compiler->logMessage(BaseCompiler::ProcessMessage, "Adding variable " + currentSymbol.toString());
			s = parseVariableDefinition(isConst);
			match(JitTokens::semicolon);
		}
	}

	return s;
}

snex::jit::BlockParser::StatementPtr NewClassParser::parseSmoothedVariableDefinition()
{
	StatementPtr s;

	match(JitTokens::assign_);
	auto value = parseVariableStorageLiteral();

	return new Operations::SmoothedVariableDefinition(location, currentSymbol, currentHnodeType, value);
}

snex::jit::BlockParser::StatementPtr NewClassParser::parseWrappedBlockDefinition()
{
	StatementPtr s;
	match(JitTokens::assign_);

	auto value = parseBufferInitialiser();

	return new Operations::WrappedBlockDefinition(location, currentSymbol, value);
}

snex::jit::BlockParser::ExprPtr NewClassParser::parseBufferInitialiser()
{
	if (auto cc = dynamic_cast<ClassCompiler*>(compiler.get()))
	{
		auto& handler = cc->memoryPool.getBufferHandler();
		auto id = parseIdentifier();

		if (id == Identifier("Buffer"))
		{
			match(JitTokens::dot);

			auto function = parseIdentifier();
			match(JitTokens::openParen);

			auto value = parseVariableStorageLiteral().toInt();
			match(JitTokens::closeParen);

			try
			{
				if (function.toString() == "create")
				{
					return new Operations::Immediate(location, handler.create(currentSymbol.id, value));
				}
				else if (function.toString() == "getAudioFile")
				{
					return new Operations::Immediate(location, handler.getAudioFile(value, currentSymbol.id));
				}
				else if (function.toString() == "getTable")
				{
					return new Operations::Immediate(location, handler.getTable(value, currentSymbol.id));
				}
				else
				{
					throw juce::String("Invalid buffer function");
				}
			}
			catch (juce::String& s)
			{
				location.throwError(s);
			}
		}
		else
			location.throwError("Expected Buffer function");
	}
}

BlockParser::StatementPtr NewClassParser::parseVariableDefinition(bool /*isConst*/)
{
	auto type = currentHnodeType;
	auto target = new Operations::VariableReference(location, { {}, currentSymbol.id, type });
	target->isLocalToScope = true;

	match(JitTokens::assign_);


	ExprPtr expr;
	
	if (type == Types::ID::Block)
	{
		expr = parseBufferInitialiser();
	}
	else
	{
		expr = new Operations::Immediate(location, parseVariableStorageLiteral());
	}
	
	return new Operations::Assignment(location, target, JitTokens::assign_, expr);
}

BlockParser::StatementPtr NewClassParser::parseFunction()
{
	auto func = new Operations::Function(location);

	StatementPtr newStatement = func;

	auto& fData = func->data;

	fData.id = currentSymbol.id;
	fData.returnType = currentHnodeType;
	fData.object = nullptr;

	while (currentType != JitTokens::closeParen && currentType != JitTokens::eof)
	{
		auto t = matchType();

		bool isAlias = matchIf(JitTokens::bitwiseAnd);

		fData.args.add({ t, isAlias });
		func->parameters.add(parseIdentifier());

		matchIf(JitTokens::comma);
	}

	match(JitTokens::closeParen);

	func->code = location.location;

	match(JitTokens::openBrace);
	int numOpenBraces = 1;

	while (currentType != JitTokens::eof && numOpenBraces > 0)
	{
		if (currentType == JitTokens::openBrace) numOpenBraces++;
		if (currentType == JitTokens::closeBrace) numOpenBraces--;
		skip();
	}

	
	func->codeLength = static_cast<int>(location.location - func->code);
	
	return newStatement;
}

snex::VariableStorage BlockParser::parseVariableStorageLiteral()
{
	bool isMinus = matchIf(JitTokens::minus);

	auto type = Types::Helpers::getTypeFromStringValue(currentString);

	juce::String stringValue = currentString;
	match(JitTokens::literal);

	if (type == Types::ID::Integer)
		return VariableStorage(stringValue.getIntValue() * (!isMinus ? 1 : -1));
	else if (type == Types::ID::Float)
		return VariableStorage(stringValue.getFloatValue() * (!isMinus ? 1.0f : -1.0f));
	else if (type == Types::ID::Double)
		return VariableStorage(stringValue.getDoubleValue() * (!isMinus ? 1.0 : -1.0));
	else if (type == Types::ID::Event)
    {
        HiseEvent e;
		return VariableStorage(e);
    }
	else if (type == Types::ID::Block)
		return block();
	else
		return {};
}



}
}


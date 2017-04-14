/*
  ==============================================================================

    FunctionParser.h
    Created: 7 Mar 2017 10:45:07pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef FUNCTIONPARSER_H_INCLUDED
#define FUNCTIONPARSER_H_INCLUDED



template <typename R, typename ... Parameters> class FunctionParser : public FunctionParserBase
{
public:

	
	FunctionParser(HiseJITScope::Pimpl* scope_, const FunctionInfo& info_) :
		FunctionParserBase(scope_, info_)
	{
		
	}

	~FunctionParser()
	{
		
	}

	void addVoidReturnStatement() override
	{
		if (!HiseJITTypeHelpers::is<R, void>() || voidReturnWasFound)
			return;

		storeGlobalsBeforeReturn();

		asmCompiler->ret();
	}

	void finalizeReturnStatement()
	{
		int x = 5;


#if 0
		returnStatement = &expr.Add(*returnStatement, expr.template Immediate<R>(R()));

		checkAllLinesReferenced();

		for (int i = 0; i < getNumGlobalNodes(); i++)
		{
			auto& zero = getGlobalNode(i)->template getAssignmentNodeForReturnStatement<R>(&expr);

			returnStatement = &expr.Add(*returnStatement, zero);
		}

		for (int i = 0; i < getNumAnonymousLines(); i++)
		{
			returnStatement = &expr.Add(*returnStatement, getAnonymousLine<R>(i));
		}

		for (int i = 0; i < bufferAssignmentNodes.size(); i++)
		{
			returnStatement = &expr.Add(*returnStatement, *bufferAssignmentNodes[i]);
		}
#endif
	}

	void storeGlobalsBeforeReturn()
	{
		for (int i = 0; i < globalNodes.size(); i++)
		{
			if (globalNodes[i]->isChangedGlobal())
			{
				void* data = &(scope->getGlobal(globalNodes[i]->getId())->data);

				TypeInfo thisType = globalNodes[i]->getType();

				if (HiseJITTypeHelpers::matchesType<float>(thisType)) AsmJitHelpers::StoreGlobal<float>(*asmCompiler, data, globalNodes[i]);
				if (HiseJITTypeHelpers::matchesType<double>(thisType)) AsmJitHelpers::StoreGlobal<double>(*asmCompiler, data, globalNodes[i]);
				if (HiseJITTypeHelpers::matchesType<int>(thisType)) AsmJitHelpers::StoreGlobal<int>(*asmCompiler, data, globalNodes[i]);
				if (HiseJITTypeHelpers::matchesType<BooleanType>(thisType)) AsmJitHelpers::StoreGlobal<BooleanType>(*asmCompiler, data, globalNodes[i]);
			}
		}
	}

	void parseReturn() override
	{
		
		ScopedBaseNodePointer rt;

		if (matchIf(HiseJitTokens::semicolon))
		{
			addVoidReturnStatement();
			voidReturnWasFound = true;
		}
		else
		{
			rt = parseTypedExpression<R>();
			match(HiseJitTokens::semicolon);

			storeGlobalsBeforeReturn();

			AsmJitHelpers::Return(*asmCompiler, getTypedNode<R>(rt));
		}
	}

	BaseNodePtr parseParameterReferenceTyped(const Identifier& id) override
	{
		const int pIndex = getParameterIndex(id);

		

		const String pTypeName = info.parameterTypes[pIndex].toString();

		const TypeInfo pType = HiseJITTypeHelpers::getTypeForToken(pTypeName.getCharPointer());

		ScopedBaseNodePointer pNode;

		if (pIndex == 0)
			pNode = AsmJitHelpers::Parameter(*asmCompiler, 0, pType);
		
		else if (pIndex == 1)
		{
			pNode = AsmJitHelpers::Parameter(*asmCompiler, 1, pType);
		}
		else
		{
			return nullptr;
		}

		pNode->setId(id.toString());

		return pNode.release();
	}

	void parseBufferLine(const Identifier &id) override
	{
		parseIdentifier();

		if (matchIf(HiseJitTokens::openBracket))
		{
			parseBufferAssignment(id);
			
		}

#if 0
		if (matchIf(HiseJitTokens::dot))
		{
			auto r = parseBufferFunction<R>(id);
			//bufferAssignmentNodes.add(dynamic_cast<HiseJIT::Node<R>*>(r));
		}
#endif

		match(HiseJitTokens::semicolon);
	}

	typedef R(*FuncPointer)(Parameters...);

	FuncPointer compileFunction()
	{
#if 0
		if (returnStatement == nullptr)
		{
			location.throwError("No return value specified");
			return nullptr;
		}

		finalizeReturnStatement();

		return expr.Compile(*returnStatement);
#endif

		return nullptr;
	}

private:

	
};



#endif  // FUNCTIONPARSER_H_INCLUDED

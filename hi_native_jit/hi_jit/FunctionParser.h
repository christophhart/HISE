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

	using TypedNativeJITFunction = NativeJIT::Function<R, Parameters...>;

	Array<NativeJIT::Node<R>*> bufferAssignmentNodes;

	TypedNativeJITFunction expr;

	FunctionParser(NativeJITScope::Pimpl* scope_, const FunctionInfo& info_) :
		FunctionParserBase(scope_, info_),
		expr(scope_->allocator, *scope_->createFunctionBuffer())
	{
		setExpressionNodeFactory(&expr);
	}

	void addVoidReturnStatement() override
	{
		returnStatement = &expr.template Immediate<R>(R());
	}

	void finalizeReturnStatement()
	{
		returnStatement = &expr.Add(*returnStatement, expr.template Immediate<R>(R()));

		checkAllLinesReferenced();

		for (int i = 0; i < getNumGlobalNodes(); i++)
		{
			auto& zero = getGlobalNode(i)->getAssignmentNodeForReturnStatement<R>(&expr);

			returnStatement = &expr.Add(*returnStatement, zero);
		}

		for (int i = 0; i < bufferAssignmentNodes.size(); i++)
		{
			returnStatement = &expr.Add(*returnStatement, *bufferAssignmentNodes[i]);
		}
	}


	void parseReturn() override
	{
		returnStatement = &parseExpression<R>();
		match(NativeJitTokens::semicolon);
	}

	NativeJIT::NodeBase* parseParameterReferenceTyped(const Identifier& id) override
	{
		const int pIndex = getParameterIndex(id);

		if (pIndex == 0)
			return &expr.GetP1();
		
		else if (pIndex == 1)
		{
			return &expr.GetP2();
		}
		else
		{
			return nullptr;
		}
	}


	void parseBufferLine(const Identifier &id) override
	{
		if (matchIf(NativeJitTokens::openBracket))
		{
			auto& r = parseBufferAssignment<R>(id);
			bufferAssignmentNodes.add(&r);
		}

		if (matchIf(NativeJitTokens::dot))
		{
			auto& r = parseBufferFunction<R>(id);
			bufferAssignmentNodes.add(&r);
		}

		match(NativeJitTokens::semicolon);
	}

	typedef R(*FuncPointer)(Parameters...);

	FuncPointer compileFunction()
	{
		if (returnStatement == nullptr)
		{
			location.throwError("No return value specified");
			return nullptr;
		}

		finalizeReturnStatement();

		return expr.Compile(*returnStatement);
	}


private:



	NativeJIT::Node<R>* returnStatement = nullptr;


};



#endif  // FUNCTIONPARSER_H_INCLUDED

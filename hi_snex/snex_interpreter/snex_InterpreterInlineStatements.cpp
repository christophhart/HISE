namespace snex {
namespace interpreter {
namespace nodes {

/** A InlinedFunction is a syntax tree that should be opaque to its parent.

    - creates a new scope with ScopeType::InlinedFunction
    - allocates for each InlinedArgument
    - assigns the incoming values to the scope
    - InlinedParameters read the value from the scope
*/
struct InlinedFunction: public Expression,
                        public DefinitionBase
{
    SNEX_INTERPRETER_NODE(InlinedFunction, WantsNode::No);

    InlinedFunction(Factory& f, const ValueTree& v) :
      Expression(f, v),
      scopeId(Helpers::getNamespacedIdentifier(v, AstIds::ScopeId)),
      returnType(Helpers::getType(f, v, AstIds::ReturnType)),
      blockData(f, *this)
    {}

    TypeInfo getTypeInfo() const override
    {
        return returnType;
    }

    VariableStorage getResult(Scope* s) override
    {
        VariableStorage rv;
        blockData.perform(statementScope, &rv);
        return rv;
    }

    Scope::Ptr createScope(Scope* scopeToUse) override
    {
        return new Scope(Scope::ScopeType::InlinedFunction, scopeToUse, scopeToUse->engine, scopeId);
    }

    Result allocateInScope(Scope* scopeToUse) override
    {
        setAllocated();
        return Result::ok();
    }

    NamespacedIdentifier scopeId;
    TypeInfo returnType;
    Statement::BlockExecutor blockData;
};

struct InlinedArgument : public Statement,
                         public DefinitionBase
{
    SNEX_INTERPRETER_NODE(InlinedArgument, WantsNode::No);

    InlinedArgument(Factory& f, const ValueTree& v) :
        Statement(f, v),
        argument(Helpers::getSymbol(f, v, AstIds::ParameterName)),
        initialiser(Helpers::getChildExpression(this, 0)),
        df(OpFunctions::Data::getFunctionPtr(initialiser->getTypeInfo()))
    {}

    Result allocateInScope(Scope* scopeToUse) override
    {
        setAllocated();
        return scopeToUse->addSymbol(argument, argument.typeInfo.getRequiredByteSize(), argument.typeInfo.getRequiredAlignment());
    }

    void resolveDataPointers(Scope* s) override
    {
        if (auto thisScope = s->getScope(argument.id.getParent()))
        {
            if (auto e = thisScope->find(argument.id.id))
            {
                target = { thisScope->getDataPointer(), e->offset };
                argument.resolved = true;
                return;
            }
        }

        // happens at inline function with references?
        jassertfalse; 
        auto idx = s->getLValueReferenceIndex(argument.id.id);
    }

    ResultCode perform(Scope* s, VariableStorage* returnValue)
    {
        auto v = initialiser->getResult(s);
        df.second(target, v);
        return ResultCode::ok;
    }

    Scope::LValue target;
    Symbol argument;
    Expression::ExprPtr initialiser;
    OpFunctions::Data::Functions df;
};

struct InlinedParameter : public VariableReference
{
    SNEX_INTERPRETER_NODE(InlinedParameter, WantsNode::No);

    InlinedParameter(Factory& f, const ValueTree& v) :
        VariableReference(f, v)
    {}

};

struct InlinedReturnValue : public ReturnStatement
{
    SNEX_INTERPRETER_NODE(InlinedReturnValue, WantsNode::No);

    InlinedReturnValue(Factory& f, const ValueTree& v) :
        ReturnStatement(f, v)
    {}
};



} // namespace nodes
} // namespace interpreter
} // namespace snex
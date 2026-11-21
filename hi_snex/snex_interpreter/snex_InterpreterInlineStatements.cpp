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

    static Symbol resolveSymbol(Factory& f, const ValueTree& v, const Identifier symbolId)
    {
        auto symbolString = v[symbolId].toString();

        if (symbolString.startsWith("unresolved"))
        {
            if (symbolString == "unresolved this")
            {
                auto parentFunc = valuetree::Helpers::findParentWithType(v, Identifier("InlinedFunction"));
                jassert(parentFunc.isValid());

                Symbol thisPointer;

                thisPointer.id = Helpers::getNamespacedIdentifier(parentFunc, AstIds::ScopeId);
                thisPointer.id = thisPointer.id.getChildId("this");

                for (auto c : parentFunc)
                {
                    if (c.getType() == Identifier("InlinedArgument") &&
                        c[AstIds::ParameterName].toString() == symbolString)
                    {
                        auto childType = Helpers::getSymbol(f, c.getChild(0));
                        thisPointer.typeInfo = childType.typeInfo.withModifiers(false, true, false);
                        return thisPointer;
                    }
                }
            }
            

            jassertfalse;
        }

        return Helpers::getSymbol(f, v, symbolId);
    }

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
        argument(InlinedFunction::resolveSymbol(f, v, AstIds::ParameterName)),
        initialiser(Helpers::getChildExpression(this, 0)),
        df(OpFunctions::Data::getFunctionPtr(initialiser->getTypeInfo()))
    {
        
    }

    Result allocateInScope(Scope* scopeToUse) override
    {
        setAllocated();
        auto inlineScope = scopeToUse->getParentScopeWithType(Scope::ScopeType::InlinedFunction);
        jassert(inlineScope->id == argument.id.getParent());

        return inlineScope->addSymbol(argument);
    }

    void resolveDataPointers(Scope* s) override
    {
        auto inlineScope = s->getParentScopeWithType(Scope::ScopeType::InlinedFunction);
        inlineScope->resolveDataPointer(argument.id);
    }

    ResultCode perform(Scope* s, VariableStorage* returnValue)
    {
        auto v = initialiser->getResult(s);
        target.write(v);
        return ResultCode::ok;
    }

    Scope::ResolvedReference target;
    Symbol argument;
    Expression::ExprPtr initialiser;
    OpFunctions::Data::Functions df;
};

struct InlinedParameter : public VariableReference
{
    SNEX_INTERPRETER_NODE(InlinedParameter, WantsNode::No);

    InlinedParameter(Factory& f, const ValueTree& v) :
        VariableReference(f, v)
    {
        this->symbol = InlinedFunction::resolveSymbol(f, v, AstIds::Symbol);
    }

    void resolveDataPointers(Scope* s) override
    {
        auto inlineScope = s->getParentScopeWithType(Scope::ScopeType::InlinedFunction);
        jassert(inlineScope->id == this->symbol.id.getParent());
        this->ref = inlineScope->resolveDataPointer(this->symbol.id);
        this->symbol.resolved = (bool)this->ref;
    }

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
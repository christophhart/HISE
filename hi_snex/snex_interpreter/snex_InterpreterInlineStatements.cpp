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
      blockData(f, *this),
      isReference(returnType.isRef()),
      readReturnedValue(isReference && returnType.getType() != Types::ID::Pointer),
      returnFunctions(isReference ? OpFunctions::Data::getFunctionPtr(returnType) : OpFunctions::Data::Functions{})
    {}

    static Symbol resolveSymbol(Factory& f, const ValueTree& v, const Identifier symbolId)
    {
        auto symbolString = v[symbolId].toString();

        if (symbolString.startsWith("unresolved"))
        {
            auto parentFunc = valuetree::Helpers::findParentWithType(v, Identifier("InlinedFunction"));
            jassert(parentFunc.isValid());

            auto scopeId = Helpers::getNamespacedIdentifier(parentFunc, AstIds::ScopeId);

            for (auto c : parentFunc)
            {
                if (c.getType() != Identifier("InlinedArgument") ||
                    c[AstIds::ParameterName].toString() != symbolString ||
                    c.getNumChildren() == 0)
                    continue;

                auto childSymbol = Helpers::getSymbol(f, c.getChild(0));
                Symbol resolved;
                resolved.typeInfo = childSymbol.typeInfo;

                if (symbolString == "unresolved this")
                {
                    resolved.id = scopeId.getChildId("this");
                    resolved.typeInfo = resolved.typeInfo.withModifiers(false, true, false);
                }
                else
                {
                    auto parameterName = symbolString.fromLastOccurrenceOf("::", false, false);
                    if (parameterName.isEmpty())
                        break;
                    resolved.id = scopeId.getChildId(parameterName);
                    resolved.typeInfo = resolved.typeInfo.withModifiers(false, true, false);
                }

                return resolved;
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

        if (readReturnedValue)
            return returnFunctions.first({ rv.getDataPointer(), 0 });

        return rv;
    }

    Scope::LValue& getLValue(Scope* s) override
    {
        if (!isReference)
            return Expression::getLValue(s);

        VariableStorage rv;
        blockData.perform(statementScope, &rv);
        returnAddress = { rv.getDataPointer(), 0 };
        return returnAddress;
    }

    void assign(Scope* s, const VariableStorage& value) override
    {
        if (!isReference)
            return Expression::assign(s, value);

        returnFunctions.second(getLValue(s), value);
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
    const bool isReference;
    const bool readReturnedValue;
    const OpFunctions::Data::Functions returnFunctions;
    Scope::LValue returnAddress;
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
        target = inlineScope->resolveDataPointer(argument.id);
    }

    ResultCode perform(Scope* s, VariableStorage* returnValue)
    {
        if (argument.typeInfo.isRef())
            target.assign(initialiser->getLValue(s));
        else
            target.write(initialiser->getResult(s));

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
        this->ref = s->resolveDataPointer(this->symbol.id);
        this->symbol.resolved = (bool)this->ref;
    }

};

struct InlinedReturnValue : public ReturnStatement
{
    SNEX_INTERPRETER_NODE(InlinedReturnValue, WantsNode::No);

    InlinedReturnValue(Factory& f, const ValueTree& v) :
        ReturnStatement(f, v),
        returnsReference(Helpers::getType(f,
            valuetree::Helpers::findParentWithType(v, Identifier("InlinedFunction")), AstIds::ReturnType).isRef())
    {}

    ResultCode perform(Scope* s, VariableStorage* returnValue) override
    {
        if (returnsReference && returnValue != nullptr && value != nullptr)
        {
            *returnValue = value->getLValue(s).readAsPointer();
            performCleanup(s);
            return ResultCode::returnWasHit;
        }

        return ReturnStatement::perform(s, returnValue);
    }

    const bool returnsReference;
};



} // namespace nodes
} // namespace interpreter
} // namespace snex
namespace snex {
namespace interpreter {

namespace shortcut {

struct base : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<base>;
    using ResultCode = Statement::ResultCode;

    virtual ResultCode perform(Scope* s, VariableStorage* returnValue) = 0;

    base(Statement::Factory& f, Statement& parent_);;

    struct ShortcutFactory
    {
        using CreateFunction = std::function<Ptr(Statement::Factory&, Statement& parent)>;

        ShortcutFactory();

        template <typename T> void registerType()
        {
            NamespacedIdentifier id = T::getScopeId();
            CreateFunction f = T::create;
            functions.push_back({ id, f });
        }

        Ptr create(Statement::Factory& f, Statement& parent) const
        {
            auto scopeId = parent.astNode[AstIds::ScopeId].toString();
            auto nid = NamespacedIdentifier::fromString(scopeId);

            for (const auto& sf : functions)
            {
                if (sf.first.isParentOf(nid))
                    return sf.second(f, parent);
            }

            return nullptr;
        }

    private:

        std::vector<std::pair<NamespacedIdentifier, CreateFunction>> functions;
    };

    Statement& parent;
    NamespacedIdentifier id;

    std::map<int, Expression::ExprPtr> arguments;
};

namespace index {


struct executor: public base
{
    enum class FunctionType
    {
        Unknown,
        Constructor,
        GetFrom,
        Assign,
        Inc,
        Dec,
        GetIndex,
        GetAlpha,
        GetInterpolated,
        TypeCast
    };

    enum class IndexType
    {
        unsafe,
        wrapped,
        clamped,
        unscaled,
        normalised,
        lerp,
        hermite
    };

    static bool isFloatType(IndexType t)
    {
        return t >= IndexType::unscaled;
    }

    static bool isInterpolator(IndexType t)
    {
        return t >= IndexType::lerp;
    }

    static int getUpperBounds(const TypeInfo& intIndexType)
    {
        auto st = intIndexType.getTypedComplexType<StructType>();
        return st->getTemplateInstanceParameters()[0].constant;
    }

    static IndexType getIndexType(const TypeInfo& indexType)
    {
        auto st = indexType.getTypedComplexType<StructType>();
        auto id = st->id.id;

        if (id == Identifier("unsafe"))
            return IndexType::unsafe;
        if (id == Identifier("clamped"))
            return IndexType::clamped;
        if (id == Identifier("wrapped"))
            return IndexType::wrapped;
        if (id == Identifier("unscaled"))
            return IndexType::unscaled;
        if (id == Identifier("normalised"))
            return IndexType::normalised;
        if (id == Identifier("lerp"))
            return IndexType::lerp;
        if (id == Identifier("hermite"))
            return IndexType::hermite;

        jassertfalse;
        return IndexType::unsafe;
    }

    void setFloatingPointType(const TypeInfo& ft)
    {
        if (auto st = ft.getTypedComplexType<StructType>())
        {
            floatIndexType = getIndexType(ft);
            auto list = st->getTemplateInstanceParameters();
            floatTargetType = list[0].type.getType();
            dataFunctions = Statement::OpFunctions::Data::getFunctionPtr(TypeInfo(floatTargetType));

            setIntegerType(list[1].type);
        }
    }

    void setIntegerType(const TypeInfo& it)
    {
        integerIndexType = getIndexType(it);
        upperBounds = getUpperBounds(it);
    }

    executor(Statement::Factory& f, Statement& p, IndexType t, const Identifier& id) :
      base(f, p),
      type(t),
      dataFunctions(Statement::OpFunctions::Data::getFunctionPtr(TypeInfo(Types::ID::Integer)))
    {
        if (id == Identifier("type_cast"))
            functionType = FunctionType::TypeCast;
        else if (id == Identifier("assignInternal"))
            functionType = FunctionType::Assign;
        else if (id == Identifier("operator="))
            functionType = FunctionType::Assign;
        else if (id == Identifier("getFrom"))
            functionType = FunctionType::GetFrom;
        else if (id == Identifier("++operator"))
            functionType = FunctionType::Inc;
        else if (id == Identifier("--operator"))
            functionType = FunctionType::Dec;
        else if (id == this->id.getParent().id)
            functionType = FunctionType::Constructor;
        else if (id == Identifier("getIndex"))
            functionType = FunctionType::GetIndex;
        else if (id == Identifier("getAlpha"))
            functionType = FunctionType::GetAlpha;
        else if (id == Identifier("getInterpolated"))
            functionType = FunctionType::GetInterpolated;
        else
        {
            jassertfalse;
        }

        auto ti = arguments[-1]->getTypeInfo();

        if (isFloatType(type))
        {
            if (isInterpolator(type))
            {
                argContainer.allocateReturnType(*parent.getTyped<Expression>());

                if (auto st = ti.getTypedComplexType<StructType>())
                {
                    auto ft = st->getTemplateInstanceParameters()[0].type;
                    setFloatingPointType(ft);
                }
            }
            else
            {
                setFloatingPointType(ti);
            }
        }
        else
        {
            integerIndexType = type;
            upperBounds = getUpperBounds(ti);
        }

        if (functionType == FunctionType::GetFrom)
        {
            auto container = arguments[0]->getTypeInfo();

            if (auto sp = container.getTypedIfComplexType<SpanType>())
            {
                upperBounds = sp->getNumElements();
                elementSize = sp->getElementSize();
                isDynAccess = false;
            }
            if (auto dyn = container.getTypedIfComplexType<DynType>())
            {
                elementSize = dyn->getElementType().getRequiredByteSizeNonZero();
                isDynAccess = true;
            }
        }
    };

    virtual ~executor() {};

    Scope::LValue targetPointer;
    Statement::OpFunctions::Data::Functions dataFunctions;

    VariableStorage performIndexOp(Scope* s)
    {
        auto thisPtr = arguments[-1]->getResult(s);

        targetPointer = { thisPtr.getDataPointer(), 0 };

        if (isFloatType(type))
        {
            if (floatTargetType == Types::ID::Float)
                return performWithFloatType<float>(s);
            else
                return performWithFloatType<double>(s);
        }
        else
        {
            return switchIntIndexOp(s, type);
        }
    }

    template <typename FloatType> VariableStorage performWithFloatType(Scope* s)
    {
        auto floatValue = (FloatType)dataFunctions.first(targetPointer);

        switch (integerIndexType)
        {
        case IndexType::unsafe:
        {
            snex::Types::index::unsafe<0, false> obj(0);
            return performFloatIndexOp(s, obj, floatValue, floatIndexType);
        }
        case IndexType::wrapped:
        {
            snex::Types::index::wrapped<0, false> obj(0);
            return performFloatIndexOp(s, obj, floatValue, floatIndexType);
        }
        case IndexType::clamped:
        {
            snex::Types::index::clamped<0, false> obj(0);
            return performFloatIndexOp(s, obj, floatValue, floatIndexType);
        }
        }
    }

    template <typename IntType, typename F> VariableStorage performFloatIndexOp(Scope* s, IntType& obj, F floatValue, IndexType ft)
    {
        switch (ft)
        {
        case IndexType::unscaled:
        {
            snex::Types::index::unscaled<F, IntType> fobj(floatValue);
            return performIndexOp(s, fobj);
        }
        case IndexType::normalised:
        {
            snex::Types::index::normalised<F, IntType> fobj(floatValue);
            return performIndexOp(s, fobj);
        }
        }
    }

    VariableStorage switchIntIndexOp(Scope* s, IndexType typeToUse)
    {
        auto intValue = dataFunctions.first(targetPointer).toInt();

        switch (typeToUse)
        {
        case IndexType::unsafe:
        {
            snex::Types::index::unsafe<0, false> obj(0);
            return performIndexOp(s, obj);
        }
        case IndexType::wrapped:
        {
            snex::Types::index::wrapped<0, false> obj(intValue);
            return performIndexOp(s, obj);
        }
        case IndexType::clamped:
        {
            snex::Types::index::clamped<0, false> obj(intValue);
            return performIndexOp(s, obj);
        }
        }
    }

    template <typename I> VariableStorage performIndexOp(Scope* s, I& obj)
    {
        using IType = typename I::Type;
        static constexpr auto ITypeId = Types::Helpers::getTypeFromTypeId<IType>();
        static constexpr bool IsFloatType = std::is_floating_point<IType>();
        static constexpr bool IsInterpolator = I::isInterpolator();

        if constexpr (!IsInterpolator && IsFloatType)
        {
            if (isInterpolator(type))
            {
                switch (type)
                {
                case IndexType::lerp:
                {
                    snex::Types::index::lerp<I> obj2(obj.getRawValue());
                    return performIndexOp(s, obj2);
                }
                case IndexType::hermite:
                {
                    snex::Types::index::hermite<I> obj2(obj.getRawValue());
                    return performIndexOp(s, obj2);
                }
                }

                jassertfalse;
            }
        }

        //using TestIndexType = snex::Types::index::unsafe<0, false>;
        //using TestFloatType = snex::Types::index::normalised<float, TestIndexType>;
        //using InterpolatorType = snex::Types::index::lerp<TestFloatType>;

        auto value = dataFunctions.first(targetPointer).toType<ITypeId>();

        switch (functionType)
        {
        case FunctionType::Constructor:
        {
            auto initValue = arguments[0]->getResult(s);
            dataFunctions.second(targetPointer, initValue);
            return VariableStorage();
        }
        case FunctionType::TypeCast:
        {
            return obj.getLogicType().getWithDynamicLimit(value, (IType)upperBounds);
        }
        case FunctionType::Dec:
        {
            value -= IType(1.0);
            dataFunctions.second(targetPointer, value);
            return VariableStorage(targetPointer.first);
        }
        case FunctionType::Inc:
        {
            value += IType(1.0);
            dataFunctions.second(targetPointer, value);
            return VariableStorage(targetPointer.first);
        }
        case FunctionType::Assign:
        {
            auto newValue = arguments[0]->getResult(s).toType<ITypeId>();
            obj = newValue;
            dataFunctions.second(targetPointer, obj.getRawValue());
            return VariableStorage();
        }
        case FunctionType::GetIndex:
        {
            auto delta = arguments[1]->getResult(s).toInt();
            auto limit = arguments[0]->getResult(s).toInt();

            if constexpr (IsFloatType && !IsInterpolator)
            {
                return obj.getIndex(limit, delta);
            }

            jassertfalse;
            return VariableStorage();
        }
        case FunctionType::GetAlpha:
        {
            if constexpr (IsFloatType && !IsInterpolator)
                return obj.getAlpha(upperBounds);

            jassertfalse;
            return VariableStorage();
        }
        case FunctionType::GetFrom:
        {
            auto container = arguments[0]->getResult(s);
            void* src = nullptr;

            if (isDynAccess)
            {
                auto dv = Statement::OpFunctions::Data::readDynView(container);
                src = dv.data;
                upperBounds = dv.size;
            }
            else
            {
                src = container.getDataPointer();
            }

            int elementIndex;

            if constexpr (IsInterpolator)
            {
                auto numPerFrame = elementSize / sizeof(IType);

                if (numPerFrame == 1)
                {
                    dyn<IType> container(reinterpret_cast<IType*>(src), upperBounds);
                    return obj.getFrom(container);
                }
                else
                {
                    switch (numPerFrame)
                    {
                    case 2: 
                    {
                        using ST = span<IType, 2, 8>;
                        dyn<ST> container(reinterpret_cast<ST*>(src), upperBounds);
                        auto x = obj.getFrom(container);

                        auto dst = (void*)argContainer.returnData.begin();
                        memcpy(dst, &x, sizeof(x));
                        return VariableStorage(dst);
                    }
                    case 3:
                    case 4:
                    default:
                        jassertfalse;
                    }

                    jassertfalse;
                }

                
            }
            else if constexpr (IsFloatType)
                elementIndex = obj.getIndex(upperBounds);
            else
                elementIndex = obj.getLogicType().getWithDynamicLimit(value, (IType)upperBounds);

            src = reinterpret_cast<uint8*>(src) + elementIndex * elementSize;
            
            return VariableStorage(src);
        }

        default:
            jassertfalse;
        }

        return VariableStorage();
    }

    ResultCode perform(Scope* s, VariableStorage* returnValue) final override
    {
        *returnValue = performIndexOp(s);
        return ResultCode::ok;
    }

    const IndexType type;
    FunctionType functionType = FunctionType::Unknown;
    int upperBounds = 0;

    IndexType integerIndexType;
    IndexType floatIndexType;
    Types::ID floatTargetType = Types::ID::Dynamic;

    size_t elementSize;
    bool isDynAccess = false;

    Statement::OpFunctions::Inliners::ArgContainer argContainer;
};

struct unsafe : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::unsafe"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new unsafe(f, p); }

    unsafe(Statement::Factory& f, Statement& s) :
        executor(f, s, IndexType::unsafe, id.id)
    {};
};

struct wrapped : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::wrapped"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new wrapped(f, p); }

    wrapped(Statement::Factory& f, Statement& s):
      executor(f, s, IndexType::wrapped, id.id)
    {};
};

struct clamped : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::clamped"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new clamped(f, p); }

    clamped(Statement::Factory& f, Statement& s) :
        executor(f, s, IndexType::clamped, id.id)
    {};
};

struct unscaled : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::unscaled"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new unscaled(f, p); }

    unscaled(Statement::Factory& f, Statement& s) :
        executor(f, s, IndexType::unscaled, id.id)
    {};
};

struct normalised : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::normalised"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new normalised(f, p); }

    normalised(Statement::Factory& f, Statement& s) :
        executor(f, s, IndexType::normalised, id.id)
    {};
};

struct lerp : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::lerp"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new lerp(f, p); }

    lerp(Statement::Factory& f, Statement& s) :
        executor(f, s, IndexType::lerp, id.id)
    {};
};

struct hermite : public executor
{
    static NamespacedIdentifier getScopeId() { return NamespacedIdentifier::fromString("index::hermite"); }
    static Ptr create(Statement::Factory& f, Statement& p) { return new hermite(f, p); }

    hermite(Statement::Factory& f, Statement& s) :
        executor(f, s, IndexType::hermite, id.id)
    {};
};

} // index
} // shortcut

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
      returnFunctions(isReference ? OpFunctions::Data::getFunctionPtr(returnType) : OpFunctions::Data::Functions{}),
      shortcutFunction(shortcut::base::ShortcutFactory().create(f, *this))
    {}

    static Symbol resolveSymbol(Factory& f, const ValueTree& v, const Identifier symbolId)
    {
        auto symbolString = v[symbolId].toString();

        if (symbolString.startsWith("unresolved"))
        {
            auto parentFunc = valuetree::Helpers::findParentWithType(v, Identifier("InlinedFunction"));
            auto callerArgument = valuetree::Helpers::findParentWithType(v, Identifier("InlinedArgument"));

            // An argument initializer runs in its caller's scope, before this inline call binds its parameters.
            if (callerArgument.isValid() && callerArgument.getParent() == parentFunc)
                parentFunc = valuetree::Helpers::findParentWithType(parentFunc, Identifier("InlinedFunction"));

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

        performBody(s, &rv);

        if (readReturnedValue)
            return returnFunctions.first({ rv.getDataPointer(), 0 });

        return rv;
    }

    Scope::LValue& getLValue(Scope* s) override
    {
        if (!isReference)
            return Expression::getLValue(s);

        VariableStorage rv;
        performBody(s, &rv);
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

    void performBody(Scope* s, VariableStorage* rv)
    {
        if (shortcutFunction != nullptr)
            shortcutFunction->perform(s, rv);
        else
            blockData.perform(statementScope, rv);
    }

    NamespacedIdentifier scopeId;
    TypeInfo returnType;
    Statement::BlockExecutor blockData;
    const bool isReference;
    const bool readReturnedValue;
    const OpFunctions::Data::Functions returnFunctions;
    Scope::LValue returnAddress;

    shortcut::base::Ptr shortcutFunction;
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

namespace shortcut {

base::ShortcutFactory::ShortcutFactory()
{
    registerType<index::unsafe>();
    registerType<index::wrapped>();
    registerType<index::clamped>();
    registerType<index::unscaled>();
    registerType<index::normalised>();
    registerType<index::lerp>();
    registerType<index::hermite>();
}

base::base(Statement::Factory& f, Statement& parent_) :
    parent(parent_),
    id(Statement::Helpers::getNamespacedIdentifier(parent.astNode, AstIds::ScopeId))
{
    for (auto st : parent.childStatements)
    {
        if (auto il = st->getTyped<nodes::InlinedArgument>())
        {
            auto arg = (int)st->astNode[AstIds::Arg];
            auto e = Statement::Helpers::getChildExpression(st, 0);
            arguments[arg] = e;
        }
    }
}

}

} // namespace interpreter
} // namespace snex
namespace snex {
namespace interpreter {
namespace nodes {

struct InternalProperty : public Statement
{
	SNEX_INTERPRETER_NODE(InternalProperty, WantsNode::Yes);

	InternalProperty(Factory& f, const ValueTree& v):
	  Statement(f, v),
	  id(v["ID"].toString()),
	  value(v[AstIds::Value])
	{}

	void annotateUriLocationTree(UriLocationTree& t)
	{
		t.id = id;
		t.data->setProperty(AstIds::Value, value);
	}

	ResultCode perform(Scope*, VariableStorage*) override
	{
		return ResultCode::ok;
	}

	Identifier id;
	var value;
};

struct Immediate : public Expression
{
	SNEX_INTERPRETER_NODE(Immediate, WantsNode::No);

	Immediate(Factory& f, const ValueTree& v) :
		Expression(f, v),
		type(Helpers::getType(f, v)),
		value(type.getType(), v[AstIds::Value])
	{};

	VariableStorage getResult(Scope* s) override
	{
		return value;
	}

	TypeInfo getTypeInfo() const override
	{
		return type;
	}

	TypeInfo type;
	VariableStorage value;

};

struct VariableReference : public Expression
{
	SNEX_INTERPRETER_NODE(VariableReference, WantsNode::No);

	VariableReference(Factory& f, const ValueTree& v) :
		Expression(f, v),
		symbol(Helpers::getSymbol(f, v)),
		f(OpFunctions::Data::getFunctionPtr(symbol.typeInfo)),
		isBlock(OpFunctions::Data::isBlockType(symbol.typeInfo))
	{};

	Scope::LValue& getLValue(Scope* s) override
	{
		if (refIndex != -1)
			return s->getLValueReference(refIndex);

		return lValue;
	}

	void assign(Scope* s, const VariableStorage& value) override
	{
		if (symbol.resolved)
			f.second(getLValue(s), value);
	}

	VariableStorage getResult(Scope* s) override
	{
		VariableStorage v;

		if (symbol.resolved)
			v = f.first(getLValue(s));

		if(isBlock)
			v = OpFunctions::Data::toBlock(v);

		return v;
	}

	void resolveDataPointers(Scope* rootScope) override
	{
		auto scopeId = symbol.id.getParent();
		auto idWithinScope = symbol.id.id;

		if (auto s = rootScope->getScope(scopeId))
		{
			if (getTypeInfo().isRef())
			{
				refIndex = s->getLValueReferenceIndex(symbol.id.id);
				symbol.resolved = refIndex != -1;
			}
			else
			{
				if (auto entry = s->find(idWithinScope))
				{
					if (s->getAddress(idWithinScope, symbol.typeInfo.getRequiredByteSize()) != nullptr)
					{
						lValue = { s->getDataPointer(), entry->offset };
						symbol.resolved = true;
					}
				}
				
				refIndex = s->getLValueReferenceIndex(idWithinScope);

				if (refIndex != -1)
					symbol.resolved = true;
			}
		}
	}

	TypeInfo getTypeInfo() const override
	{
		return symbol.typeInfo;
	}

	int refIndex = -1;
	Scope::LValue lValue;
	jit::Symbol symbol;
	OpFunctions::Data::Functions f;

	const bool isBlock = false;
};

struct Assignment : public Statement,
					public DefinitionBase
{
	SNEX_INTERPRETER_NODE(Assignment, isDefinition ? WantsNode::Yes : WantsNode::No);

	Assignment(Factory& f, const ValueTree& v) :
		Statement(f, v),
		value(Helpers::getChildExpression(this, 0)),
		target(Helpers::getChildExpression(this, 1)),
		assignmentType(Helpers::getOpType(v, AstIds::AssignmentType)),
		isDefinition((bool)v[AstIds::First] && target->getTyped<VariableReference>()),
		f(OpFunctions::BinaryOp::getFunctionPtr(value->getTypeInfo(), assignmentType))
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		if (isRef && isDefinition)
		{
			auto& targetAddress = value->getLValue(s);
			target->getLValue(s) = targetAddress;
			return ResultCode::ok;
		}

		if (f == nullptr)
		{
			auto v = value->getResult(s);
			target->assign(s, v);
		}
		else
		{
			auto l = target->getResult(s);
			auto r = value->getResult(s);

			auto v = f(l, r);
			target->assign(s, v);
		}

		return ResultCode::ok;
	}

	void annotateUriLocationTree(UriLocationTree& t)
	{
		jassert(isDefinition);

		Helpers::annotateType(t, value->getTypeInfo());

		if (auto v = target->getTyped<VariableReference>())
			t.id = v->symbol.id.toString();
	}

	Result allocateInScope(Scope* s) override
	{
		setAllocated();

		if (!isDefinition)
			return Result::ok();

		if (auto v = target->getTyped<VariableReference>())
		{
			if (v->getTypeInfo().isRef())
			{
				isRef = true;
				return s->addReference(v->symbol);
			}
			else
			{
				if (v->symbol.id.getParent() == s->id &&
					s->find(v->symbol.id.id) == nullptr)
				{
					auto& type = v->symbol.typeInfo;
					return s->addSymbol(v->symbol, type.getRequiredByteSize(),
						type.getRequiredAlignment());
				}
			}
		}

		return Result::fail("not a definition");
	}

	OpFunctions::BinaryOp::Ops assignmentType;

	Expression::ExprPtr value;
	Expression::ExprPtr target;
	const bool isDefinition;
	bool isRef = false;

	OpFunctions::BinaryOp::Function f;
};

struct ComplexTypeDefinition : public Statement,
							   public DefinitionBase
{
	SNEX_INTERPRETER_NODE(ComplexTypeDefinition, WantsNode::Yes);

	ComplexTypeDefinition(Factory& f, const ValueTree& v) :
		Statement(f, v),
		type(Helpers::getType(f, v)),
		initData(Helpers::getInitData(v))
	{
		auto list = StringArray::fromTokens(v[AstIds::Ids].toString(), ",", "");
		list.removeEmptyStrings();

		for (auto l : list)
			symbols.add(NamespacedIdentifier::fromString(l));
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		auto numBytesRequired = type.getRequiredByteSize();
		jassert(initData.getSize() >= numBytesRequired);
		auto ptr = reinterpret_cast<uint8*>(resolvedScope->getDataPointer());

		for (const auto& offset : resolvedOffsets)
		{
			memcpy(ptr + offset, initData.getData(), numBytesRequired);
		}

		return ResultCode::ok;
	}

	Result allocateInScope(Scope* rootScope) override
	{
		setAllocated();

		auto parentId = symbols.getFirst().getParent();
		resolvedScope = rootScope->getScope(parentId);

		for (const auto& id : symbols)
		{
			jit::Symbol s;
			s.id = id;
			s.typeInfo = type;
			auto ok = resolvedScope->addSymbol(s, type.getRequiredByteSize(), type.getRequiredAlignment());

			if (!ok)
				return ok;
		}

		return Result::ok();
	}

	void resolveDataPointers(Scope* scopeToUse) override
	{
		jassert(resolvedScope != nullptr);
		auto start = reinterpret_cast<uint8*>(resolvedScope->getDataPointer());

		for (auto id : symbols)
		{
			auto data = reinterpret_cast<uint8*>(resolvedScope->getAddress(id.id, type.getRequiredByteSize()));
			resolvedOffsets.push_back(static_cast<size_t>(data - start));
		}
	}

	Scope::Ptr resolvedScope;
	std::vector<size_t> resolvedOffsets;

	TypeInfo type;
	Array<NamespacedIdentifier> symbols;
	
	MemoryBlock initData;
};

struct TemplatedFunction : public Statement
{
	SNEX_INTERPRETER_NODE(TemplatedFunction, WantsNode::No);

	TemplatedFunction(Factory& f, const ValueTree& v) :
		Statement(f, v)
	{}
};

struct ClassStatement : public Statement
{
	SNEX_INTERPRETER_NODE(ClassStatement, WantsNode::Yes);

	ClassStatement(Factory& f, const ValueTree& v) :
		Statement(f, v),
		classType(dynamic_cast<StructType*>(Helpers::getType(f, v).getComplexType().get()))
	{}

	StructType* classType;
};

struct Dot : public Expression
{
	SNEX_INTERPRETER_NODE(Dot, WantsNode::No);

	Dot(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  dotParent(Helpers::getChildExpression(this, 0)),
	  dotChild(Helpers::getChildExpression(this, 1)->getTyped<VariableReference>()),
	  type(Helpers::getType(f, v, AstIds::MemberType)),
	  df(OpFunctions::Data::getFunctionPtr(type))
	{
		jassert(dotChild != nullptr);
	}

	TypeInfo getTypeInfo() const override { return type; }

	VariableStorage getResult(Scope* s)
	{
		return df.first(getLValue(s));
	}

	Scope::LValue& getLValue(Scope* s) override
	{
		auto base = (void*)dotParent->getResult(s);
		auto offset = dotChild->getLValue(s).second;
		address = { base, offset };
		return address;
	}

	void assign(Scope* s, const VariableStorage& v) override
	{
		df.second(getLValue(s), v);
	}

	Scope::LValue address;
	ExprPtr dotParent;
	VariableReference* dotChild;
	TypeInfo type;
	OpFunctions::Data::Functions df;
};

struct ThisPointer : public Expression
{
	SNEX_INTERPRETER_NODE(ThisPointer, WantsNode::No);

	ThisPointer(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  type(Helpers::getType(f, v))
	{}

	jit::TypeInfo getTypeInfo() const override { return type; }

	VariableStorage getResult(Scope* s)
	{
		auto ptr = (void**)s->getParentScopeWithType(Scope::ScopeType::Function)->getDataPointer();
		return VariableStorage(*ptr);
	}

	Scope::Ptr resolvedScope = nullptr;
	TypeInfo type;
};

struct PointerAccess : public Expression
{
	SNEX_INTERPRETER_NODE(PointerAccess, WantsNode::No);

	PointerAccess(Factory& f, const ValueTree& v) :
		Expression(f, v)
	{}

	jit::TypeInfo getTypeInfo() const override
	{
		jassertfalse;
		return TypeInfo();
	}

	VariableStorage getResult(Scope* s) override
	{
		jassertfalse;
		return VariableStorage();
	}
};

} // namespace nodes
} // namespace interpreter
} // namespace snex

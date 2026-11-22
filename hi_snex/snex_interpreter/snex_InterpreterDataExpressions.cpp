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
		f(OpFunctions::Data::getFunctionPtr(symbol.typeInfo))
	{
		auto isMember = (v.getParent().isValid() &&
					     v.getParent().getType() == Identifier("Dot") &&
					     v.getParent().indexOf(v) == 1);

		if (isMember)
		{
			auto parentType = Helpers::getType(f, v.getParent(), AstIds::ObjectType);

			if(auto st = parentType.getTypedComplexType<StructType>())
			{
				memberOffset = { true, st->getMemberOffset(symbol.id.id) };
			}
		}
	};

	Scope::LValue& getLValue(Scope* s) override
	{
		return ref.getLValue();
	}

	void assign(Scope* s, const VariableStorage& value) override
	{
		jassert(!memberOffset.first);
		ref.write(value);
	}

	VariableStorage getResult(Scope* s) override
	{
		jassert(!memberOffset.first);
		return ref.read();
	}

	void resolveDataPointers(Scope* s) override
	{
		if (memberOffset.first)
		{
			ref.immediateLocation = { nullptr, memberOffset.second };
			symbol.resolved = true;
			return;
		}


		ref = s->resolveDataPointer(symbol.id);
		symbol.resolved = (bool)ref;

		if (!symbol.resolved)
		{
			String msg;
			msg << "error at resolving symbol: " + symbol.toString();
			DBG(msg);

			auto loc = Helpers::createUriLocation(this->locationBuilder, astNode);

			DBG(loc.toLocationString(UriLocation::UriFormat::Compact));

			jassertfalse;
			throwRuntimeError(msg);
		}
	}

	TypeInfo getTypeInfo() const override
	{
		return symbol.typeInfo;
	}

	Scope::ResolvedReference ref;
	jit::Symbol symbol;
	OpFunctions::Data::Functions f;

	std::pair<bool, size_t> memberOffset;
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
				return s->addSymbol(v->symbol);
			}
			else
			{
				if (v->symbol.id.getParent() == s->id &&
					s->find(v->symbol.id.id) == nullptr)
				{
					auto& type = v->symbol.typeInfo;
					return s->addSymbol(v->symbol);
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

		for (const auto& rr : refs)
		{
			memcpy(rr.getDataPointer(), initData.getData(), numBytesRequired);
		}

		return ResultCode::ok;
	}

	Result allocateInScope(Scope* s) override
	{
		setAllocated();

		auto parentId = symbols.getFirst().getParent();
		auto resolvedScope = s->getParentScopeWithId(parentId);

		for (const auto& id : symbols)
		{
			jit::Symbol s;
			s.id = id;
			s.typeInfo = type;
			auto ok = resolvedScope->addSymbol(s);

			if (!ok)
				return ok;
		}

		return Result::ok();
	}

	void resolveDataPointers(Scope* scopeToUse) override
	{
		for (auto id : symbols)
			refs.push_back(scopeToUse->resolveDataPointer(id));
	}
	std::vector<Scope::ResolvedReference> refs;

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

struct ClassStatement : public Statement,
						public DefinitionBase
{
	SNEX_INTERPRETER_NODE(ClassStatement, WantsNode::Yes);

	ClassStatement(Factory& f, const ValueTree& v) :
		Statement(f, v),
		classType(dynamic_cast<StructType*>(Helpers::getType(f, v).getComplexType().get()))
	{}

	Scope::Ptr createScope(Scope* parentScope)
	{
		return new Scope(Scope::ScopeType::Class, parentScope, parentScope->engine, classType->id);
	}

	Result allocateInScope(Scope* scopeToUse) override
	{
		setAllocated();
		return Result::ok();
	}

	FunctionData* getFunction(const NamespacedIdentifier& fid)
	{
		auto st = childStatements[0];

		if (fid.getParent() == classType->id)
		{
			for (auto cs : st->childStatements)
			{
				if (auto f = cs->getTyped<Function>())
				{
					if (f->fData.id == fid)
						return &f->fData;
				}
			}
		}
		else
		{
			if (classType->id.isParentOf(fid))
			{
				for (auto cs : st->childStatements)
				{
					if (auto subClass = cs->getTyped<ClassStatement>())
					{
						if (auto f = subClass->getFunction(fid))
							return f;
					}
				}
			}
		}

		return nullptr;
	}

	StructType* classType;
};

struct Dot : public Expression
{
	SNEX_INTERPRETER_NODE(Dot, WantsNode::No);

	Dot(Factory& f, const ValueTree& v) :
	  Expression(f, v),
	  dotParent(Helpers::getChildExpression(this, 0)),
	  dotChild(Helpers::getChildExpression(this, 1)),
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
	ExprPtr dotChild;
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

	void resolveDataPointers(Scope* s) override
	{
		if (auto inlineScope = s->getParentScopeWithType(Scope::ScopeType::InlinedFunction))
		{
			auto thisId = inlineScope->id.getChildId("this");
			address = inlineScope->resolveDataPointer(thisId);
			if (address)
				return;
		}

		auto functionScope = s->getParentScopeWithType(Scope::ScopeType::Function);
		auto thisId = functionScope->id.getChildId("this");
		address = functionScope->resolveDataPointer(thisId);
	}

	VariableStorage getResult(Scope*)
	{
		return address.read();
	}

	Scope::LValue& getLValue(Scope*) override
	{
		return address.getLValue();
	}

	Scope::ResolvedReference address;
	TypeInfo type;
};

struct PointerAccess : public Expression
{
	SNEX_INTERPRETER_NODE(PointerAccess, WantsNode::No);

	PointerAccess(Factory& f, const ValueTree& v) :
		Expression(f, v),
		target(Helpers::getChildExpression(this, 0)),
		type(target->getTypeInfo().withModifiers(false, true, false))
	{}

	jit::TypeInfo getTypeInfo() const override
	{
		return type;
	}

	VariableStorage getResult(Scope* s) override
	{
		return target->getResult(s);
	}

	Scope::LValue& getLValue(Scope* s) override
	{
		address = { (void*)target->getResult(s), 0 };
		return address;
	}

	Expression::ExprPtr target;
	TypeInfo type;
	Scope::LValue address;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex

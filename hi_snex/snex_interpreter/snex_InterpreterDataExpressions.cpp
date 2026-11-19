namespace snex {
namespace interpreter {
namespace nodes {

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
		f(OpFunctions::createDataFunctions(symbol.typeInfo))
	{};

	void assign(Scope* s, const VariableStorage& value) override
	{
		if (resolvedScope != nullptr && f.second != nullptr)
			f.second(resolvedScope->getDataPointer(), resolvedOffset, value);
	}

	VariableStorage getResult(Scope* s) override
	{
		if (resolvedScope != nullptr && f.first != nullptr)
			return f.first(resolvedScope->getDataPointer(), resolvedOffset);

		jassertfalse;
		return {};
	}

	void resolveDataPointers(Scope* rootScope) override
	{
		auto scopeId = symbol.id.getParent();
		auto idWithinScope = symbol.id.id;

		if (auto s = rootScope->getScope(scopeId))
		{
			if (auto entry = s->find(idWithinScope))
			{
				if (s->getAddress(idWithinScope, symbol.typeInfo.getRequiredByteSize()) != nullptr)
				{
					resolvedScope = s;
					resolvedOffset = entry->offset;
					symbol.resolved = true;
				}
			}
		}
	}

	TypeInfo getTypeInfo() const override
	{
		return symbol.typeInfo;
	}

	Scope::Ptr resolvedScope;
	size_t resolvedOffset = 0;
	jit::Symbol symbol;
	OpFunctions::DataFunctions f;
};

struct Assignment : public DefinitionBase
{
	SNEX_INTERPRETER_NODE(Assignment, isDefinition ? WantsNode::Yes : WantsNode::No);

	Assignment(Factory& f, const ValueTree& v) :
		DefinitionBase(f, v),
		value(Helpers::getChildExpression(this, 0)),
		target(Helpers::getChildExpression(this, 1)),
		isDefinition((bool)v[AstIds::First] && target->getTyped<VariableReference>())
	{};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		auto v = value->getResult(s);
		target->assign(s, v);
		return ResultCode::ok;
	}

	void annotateUriLocationTree(UriLocationTree& t)
	{
		jassert(isDefinition);

		Helpers::annotateType(t, value->getTypeInfo());

		if (auto v = target->getTyped<VariableReference>())
			t.id = v->symbol.id.toString();
	}

	Result allocateInScope(Scope* rootScope) override
	{
		jassert(isDefinition);

		if (auto v = target->getTyped<VariableReference>())
		{
			if (v->symbol.id.getParent() == rootScope->id &&
				rootScope->find(v->symbol.id.id) == nullptr)
			{
				auto& type = v->symbol.typeInfo;
				return rootScope->addSymbol(v->symbol, type.getRequiredByteSize(),
					type.getRequiredAlignment());
			}
		}

		return Result::fail("not a definition");
	}

	

	Expression::ExprPtr value;
	Expression::ExprPtr target;

	const bool isDefinition;
};

struct ComplexTypeDefinition : public DefinitionBase
{
	SNEX_INTERPRETER_NODE(ComplexTypeDefinition, WantsNode::Yes);

	ComplexTypeDefinition(Factory& f, const ValueTree& v) :
		DefinitionBase(f, v),
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

} // namespace nodes
} // namespace interpreter
} // namespace snex

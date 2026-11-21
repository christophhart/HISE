namespace snex {
namespace interpreter {

Scope::Scope(ScopeType st, Scope* parent_, Engine* e, const NamespacedIdentifier& id_) :
  scopeType(st),
  parent(parent_),
  engine(e),
  id(id_)
{
	if (parent != nullptr)
		parent->children.add(this);
}

Result Scope::addSymbol(const jit::Symbol& symbol)
{
	if (dataStart != nullptr)
		return Result::fail("Scope storage is already finalised");

	if (symbol.id.getParent() != id)
		return Result::fail("Invalid symbol");
		

	if (symbol.typeInfo.isRef())
	{
		Reference nr;
		nr.isExternal = true;
		nr.id = symbol.id.id;
		nr.type = symbol.typeInfo;

		references.push_back(nr);
		return Result::ok();
	}
	else
	{
		size_t slotSize = symbol.typeInfo.getRequiredByteSizeNonZero();
		size_t alignment = symbol.typeInfo.getRequiredAlignmentNonZero();

		if (entries.find(symbol.id.id) != entries.end())
			return Result::fail("Duplicate symbol");

		const auto remainder = totalSize % alignment;
		const auto padding = remainder == 0 ? 0 : alignment - remainder;

		if (padding > std::numeric_limits<size_t>::max() - totalSize ||
			slotSize > std::numeric_limits<size_t>::max() - totalSize - padding)
			return Result::fail("Scope storage size overflow");

		Entry entry;
		entry.symbol = symbol;
		entry.offset = totalSize + padding;
		entry.size = slotSize;
		entries.emplace(symbol.id.id, entry);
		totalSize = entry.offset + slotSize;
		maxAlignment = std::max(maxAlignment, alignment);
		return Result::ok();
	}
}

Scope::ResolvedReference Scope::resolveDataPointer(const NamespacedIdentifier& symbolId)
{
	ResolvedReference rr;

	auto scopeId = symbolId.getParent();

	if (scopeId != id)
	{
		if(auto ps = getParentScopeWithId(scopeId))
			return ps->resolveDataPointer(symbolId);

		if (auto otherScope = getRootScope()->getChildScope(scopeId))
			return otherScope->resolveDataPointer(symbolId);

		jassertfalse;
		return rr;
	}

	int idx = 0;

	for (auto& r : references)
	{
		if (r.id == symbolId.id)
		{
			if (r.isExternal)
			{
				rr.s = this;
				rr.referenceIndex = idx;
				rr.type = r.type;
			}
			else
			{
				jassert(r.address.first != nullptr);
				rr.immediateLocation = r.address;
				rr.type = r.type;
			}
			
			return rr;
		}
			
		idx++;
	}

	return rr;
}

Result Scope::finalise()
{
	if (dataStart != nullptr)
		return Result::fail("Scope storage is already finalised");

	if (totalSize > std::numeric_limits<size_t>::max() - maxAlignment)
		return Result::fail("Scope storage size overflow");

	scopeData.calloc(totalSize + maxAlignment);
	auto address = reinterpret_cast<std::uintptr_t>(scopeData.get());
	auto remainder = address % maxAlignment;
	dataStart = scopeData.get() + (remainder == 0 ? 0 : maxAlignment - remainder);

	for (const auto& e : entries)
	{
		Reference nr;
		nr.address = { dataStart, e.second.offset };
		nr.isExternal = false;
		nr.type = e.second.symbol.typeInfo;
		nr.id = e.first;

		references.push_back(nr);
	}

	return Result::ok();
}

void Scope::clear()
{
	if (dataStart != nullptr)
		std::memset(dataStart, 0, totalSize);
}


const Scope::Entry* Scope::find(const Identifier& symbolId) const
{
	auto it = entries.find(symbolId);
	return it == entries.end() ? nullptr : &it->second;
}

#if 0
void* Scope::getAddress(const Identifier& symbolId, size_t bytesNeeded) const
{
	if (auto entry = find(symbolId))
		if (dataStart != nullptr && bytesNeeded <= entry->size)
			return dataStart + entry->offset;

	return nullptr;
}
#endif

Array<jit::Symbol> Scope::getSymbols() const
{
	Array<jit::Symbol> symbols;

	for (const auto& entry : entries)
		symbols.add(entry.second.symbol);

	return symbols;
}

Scope* Scope::getParentScopeWithId(const jit::NamespacedIdentifier& scopeId)
{
	if (id == scopeId)
		return this;

	if (parent != nullptr)
		return parent->getParentScopeWithId(scopeId);

	return nullptr;
}

Scope* Scope::getChildScope(const jit::NamespacedIdentifier& scopeId)
{
	if (id == scopeId)
		return this;

	for (auto c : children)
	{
		if (auto match = c->getChildScope(scopeId))
			return match;
	}

	return nullptr;
}

Scope::LValue& Scope::ResolvedReference::getLValue() 
{ 
	if (referenceIndex != -1)
	{
		jassert(isPositiveAndBelow(referenceIndex, s->references.size()));
		return s->references[referenceIndex].address;
	}

	return immediateLocation;
}

Scope::LValue Scope::ResolvedReference::getLValueCopy() const
{
	if (referenceIndex != -1)
	{
		jassert(isPositiveAndBelow(referenceIndex, s->references.size()));
		return s->references[referenceIndex].address;
	}

	return immediateLocation;
}

void* Scope::ResolvedReference::getDataPointer() const
{
	auto lv = getLValueCopy();
	jassert(lv.first != nullptr);
	return reinterpret_cast<uint8*>(lv.first) + lv.second;
}

VariableStorage Scope::ResolvedReference::read() const
{
	auto ptr = getDataPointer();

	switch (type.getType())
	{
	case Types::ID::Integer: return VariableStorage(*static_cast<int*>(ptr));
	case Types::ID::Float:   return VariableStorage(*static_cast<float*>(ptr));
	case Types::ID::Double:  return VariableStorage(*static_cast<double*>(ptr));
	case Types::ID::Pointer: {
							 auto v = VariableStorage(ptr);
							 return isBlockType(type) ? toBlock(v) : v; }
	default:				 return VariableStorage();
	}
}

void Scope::ResolvedReference::write(const VariableStorage& v)
{
	auto& lv = getLValue();
	jassert(lv.first != nullptr);

	if (isBlockType(type))
		ComplexType::writeNativeMemberType(lv.first, lv.second, toBlock(v));
	else
		ComplexType::writeNativeMemberType(lv.first, lv.second, v);
}

void Scope::ResolvedReference::assign(const LValue& lValue)
{
	getLValue() = lValue;
}

bool Scope::ResolvedReference::isBlockType(const jit::TypeInfo& t)
{
	if (t.isComplexType() && !t.isRef())
	{
		if (auto d = dynamic_cast<DynType*>(t.getComplexType().get()))
		{
			if (d->elementType.getType() == Types::ID::Float)
				return true;
		}
	}

	return false;
}

// Convert a pointer to a block type
VariableStorage Scope::ResolvedReference::toBlock(const VariableStorage& pointer)
{
	jassert(pointer.getType() == Types::ID::Pointer);
	auto blockPtr = reinterpret_cast<block*>(pointer.getDataPointer());
	jassert(blockPtr->unused == Types::ID::Block);
	return VariableStorage(*blockPtr);
}

} // namespace interpreter
} // namespace snex

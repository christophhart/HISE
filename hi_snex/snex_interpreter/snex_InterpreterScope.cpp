namespace snex {
namespace interpreter {

Scope::Scope(Scope* parent_, Engine* e, const NamespacedIdentifier& id_) :
  parent(parent_),
  engine(e),
  id(id_)
{
	if (parent != nullptr)
		parent->children.add(this);
}

Result Scope::addSymbol(const jit::Symbol& symbol, size_t slotSize, size_t alignment)
{
	if (dataStart != nullptr)
		return Result::fail("Scope storage is already finalised");

	if (symbol.id.getParent() != id || entries.find(symbol.id.id) != entries.end())
		return Result::fail("Invalid or duplicate scope symbol: " + symbol.id.toString());

	if (slotSize == 0 || alignment == 0)
		return Result::fail("Invalid scope slot size or alignment");

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

void* Scope::getAddress(const Identifier& symbolId, size_t bytesNeeded) const
{
	if (auto entry = find(symbolId))
		if (dataStart != nullptr && bytesNeeded <= entry->size)
			return dataStart + entry->offset;

	return nullptr;
}

Array<jit::Symbol> Scope::getSymbols() const
{
	Array<jit::Symbol> symbols;

	for (const auto& entry : entries)
		symbols.add(entry.second.symbol);

	return symbols;
}

Scope* Scope::getScope(const jit::NamespacedIdentifier& scopeId)
{
	if (id == scopeId)
		return this;

	for (auto c : children)
	{
		if (auto match = c->getScope(scopeId))
			return match;
	}

	return nullptr;
}

} // namespace interpreter
} // namespace snex

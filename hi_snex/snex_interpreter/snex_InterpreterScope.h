#pragma once

namespace snex {
namespace interpreter {

struct Scope : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<Scope>;
	using List = ReferenceCountedArray<Scope>;
	using WeakPtr = WeakReference<Scope>;

	Scope(Scope* parent_, Engine* e, const NamespacedIdentifier& id_);

	struct Entry
	{
		jit::Symbol symbol;
		size_t offset = 0;
		size_t size = 0;
	};

	Result addSymbol(const jit::Symbol& symbol, size_t slotSize, size_t alignment);
	Result finalise();

	void clear();
	void* getDataPointer() const { return dataStart; }
	size_t getDataSize() const { return totalSize; }

	const Entry* find(const Identifier& symbolId) const;
	void* getAddress(const Identifier& symbolId, size_t bytesNeeded) const;
	Array<jit::Symbol> getSymbols() const;
	Scope* getScope(const jit::NamespacedIdentifier& scopeId);

	List children;
	WeakPtr parent = nullptr;
	Engine* engine = nullptr;
	jit::NamespacedIdentifier id;

	HeapBlock<uint8> scopeData;
	std::map<Identifier, Entry> entries;
	uint8* dataStart = nullptr;
	size_t totalSize = 0;
	size_t maxAlignment = 1;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Scope);
};

} // namespace interpreter
} // namespace snex
